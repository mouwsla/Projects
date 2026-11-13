package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"time"

	amqp "github.com/rabbitmq/amqp091-go"
	"github.com/redis/go-redis/v9"

	"server_go_redis_rabbit/internal/taskstate"
)

func moveToDead(
	channel *amqp.Channel,
	delivery amqp.Delivery,
	reason string,
) {
	headers := copyHeaders(delivery.Headers)
	headers["x-last-error"] = reason

	err := publishMessage(
		channel,
		deadLetterExchange,
		deadQueueName,
		delivery,
		headers,
	)

	if err != nil {
		log.Printf("failed to send to DLQ: %v", err)
		requeueMessage(delivery)
		return
	}

	acknowledgeMessage(delivery)
}

func releaseTaskLease(
	client *redis.Client,
	id, owner string,
) {
	ctx, cancel := context.WithTimeout(
		context.Background(), 5*time.Second,
	)
	defer cancel()

	if err := taskstate.Release(
		ctx, client, id, owner,
	); err != nil {
		log.Printf("release lease: %v", err)
	}
}

func handleDelivery(
	client *redis.Client,
	channel *amqp.Channel,
	delivery amqp.Delivery,
) {
	var task Task

	if err := json.Unmarshal(
		delivery.Body, &task,
	); err != nil {
		moveToDead(channel, delivery,
			fmt.Sprintf("invalid JSON: %v", err))
		return
	}

	if task.ID == "" {
		moveToDead(channel, delivery, "missing task ID")
		return
	}

	if err := taskstate.Validate(
		task.Type, task.Email,
	); err != nil {
		moveToDead(channel, delivery, err.Error())
		return
	}

	owner, err := taskstate.NewID()
	if err != nil {
		requeueMessage(delivery)
		return
	}

	ctx, cancel := context.WithTimeout(
		context.Background(), 5*time.Second,
	)
	claim, err := taskstate.Claim(
		ctx, client, task.ID, owner,
	)
	cancel()

	if err != nil {
		log.Printf("claim failed: %v", err)
		requeueMessage(delivery)
		return
	}

	switch claim {
	case 0:
		// Финальный статус: повтор не нужен.
		acknowledgeMessage(delivery)
		return

	case 2:
		// Другой Worker уже обрабатывает задачу.
		requeueMessage(delivery)
		return

	case -1:
		moveToDead(channel, delivery,
			"task not found in Redis")
		return

	case 1:
		// Блокировка успешно получена.

	default:
		requeueMessage(delivery)
		return
	}

	log.Printf("Processing task %s", task.ID)

	if err := processTask(task); err != nil {
		handleTaskFailure(
			client,
			channel,
			delivery,
			task,
			owner,
			err,
		)
		return
	}

	ctx, cancel = context.WithTimeout(
		context.Background(), 5*time.Second,
	)

	err = taskstate.Finish(
		ctx, client, task.ID, owner, "completed",
	)
	cancel()

	if err != nil {
		log.Printf("finish task failed: %v", err)
		releaseTaskLease(client, task.ID, owner)
		requeueMessage(delivery)
		return
	}

	acknowledgeMessage(delivery)
}
