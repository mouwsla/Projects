package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"server_go_redis_rabbit/internal/taskstate"

	amqp "github.com/rabbitmq/amqp091-go"
	"github.com/redis/go-redis/v9"
)

const (
	maxRetries          = int32(3)
	retryPublishTimeout = 5 * time.Second
)

func processTask(task Task) error {
	if task.Type == "fail" {
		return fmt.Errorf("simulated task failure")
	}

	time.Sleep(2 * time.Second)

	return nil
}

func getRetryCount(headers amqp.Table) int32 {
	value, exists := headers["x-retry-count"]
	if !exists {
		return 0
	}

	switch count := value.(type) {
	case int8:
		return int32(count)
	case int16:
		return int32(count)
	case int32:
		return count
	case int64:
		return int32(count)
	case int:
		return int32(count)
	default:
		return 0
	}
}

func copyHeaders(source amqp.Table) amqp.Table {
	result := amqp.Table{}

	for key, value := range source {
		result[key] = value
	}

	return result
}

func publishMessage(
	channel *amqp.Channel,
	exchange string,
	routingKey string,
	message amqp.Delivery,
	headers amqp.Table,
) error {
	ctx, cancel := context.WithTimeout(
		context.Background(),
		retryPublishTimeout,
	)
	defer cancel()

	confirmation, err :=
		channel.PublishWithDeferredConfirmWithContext(
			ctx,
			exchange,
			routingKey,
			false,
			false,
			amqp.Publishing{
				Headers:      headers,
				ContentType:  "application/json",
				DeliveryMode: amqp.Persistent,
				Timestamp:    time.Now(),
				Body:         message.Body,
			},
		)
	if err != nil {
		return fmt.Errorf("publish message: %w", err)
	}

	acknowledged, err := confirmation.WaitContext(ctx)
	if err != nil {
		return fmt.Errorf(
			"wait for publisher confirmation: %w",
			err,
		)
	}

	if !acknowledged {
		return fmt.Errorf("RabbitMQ rejected message")
	}

	return nil
}

func requeueMessage(message amqp.Delivery) {
	if err := message.Nack(false, true); err != nil {
		log.Printf("failed to requeue message: %v", err)
	}

	time.Sleep(time.Second)
}

func acknowledgeMessage(message amqp.Delivery) {
	if err := message.Ack(false); err != nil {
		log.Printf("failed to acknowledge message: %v", err)
	}
}

func handleTaskFailure(
	client *redis.Client,
	channel *amqp.Channel,
	message amqp.Delivery,
	task Task,
	owner string,
	processingError error,
) {
	retries := getRetryCount(message.Headers)
	headers := copyHeaders(message.Headers)

	headers["x-last-error"] = processingError.Error()
	headers["x-task-id"] = task.ID

	exchange := ""
	routingKey := retryQueueName
	nextStatus := "retrying"

	if retries < maxRetries {
		headers["x-retry-count"] = retries + 1
	} else {
		exchange = deadLetterExchange
		routingKey = deadQueueName
		nextStatus = "failed"
	}

	// 1. Сначала публикуем сообщение.
	err := publishMessage(
		channel,
		exchange,
		routingKey,
		message,
		headers,
	)

	if err != nil {
		log.Printf("publish failed task: %v", err)
		releaseTaskLease(client, task.ID, owner)
		requeueMessage(message)
		return
	}

	// 2. После подтверждения RabbitMQ
	// обновляем состояние Redis.
	ctx, cancel := context.WithTimeout(
		context.Background(),
		5*time.Second,
	)

	err = taskstate.Finish(
		ctx,
		client,
		task.ID,
		owner,
		nextStatus,
	)
	cancel()

	if err != nil {
		log.Printf("save task status: %v", err)
		releaseTaskLease(client, task.ID, owner)
		requeueMessage(message)
		return
	}

	// 3. Только теперь подтверждаем
	// исходное сообщение.
	acknowledgeMessage(message)
}
