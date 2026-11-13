package main

import (
	"fmt"

	amqp "github.com/rabbitmq/amqp091-go"
)

const (
	taskQueueName      = "tasks"
	retryQueueName     = "tasks.retry"
	deadQueueName      = "tasks.dead"
	deadLetterExchange = "tasks.dlx"
	retryDelayMillis   = int32(5000)
)

func declareTaskTopology(
	channel *amqp.Channel,
) (
	amqp.Queue, error,
) {
	taskQueue, err := channel.QueueDeclare(
		taskQueueName,
		true,
		false,
		false,
		false,
		nil,
	)
	if err != nil {
		return amqp.Queue{}, fmt.Errorf(
			"declare task queue: %w",
			err,
		)
	}
	err = channel.ExchangeDeclare(
		deadLetterExchange,
		"direct",
		true,
		false,
		false,
		false,
		nil,
	)
	if err != nil {
		return amqp.Queue{}, fmt.Errorf(
			"declare dead-letter exchange: %w",
			err,
		)
	}
	deadQueue, err := channel.QueueDeclare(
		deadQueueName,
		true,
		false,
		false,
		false,
		nil,
	)
	if err != nil {
		return amqp.Queue{}, fmt.Errorf(
			"declare dead queue: %w",
			err,
		)
	}
	err = channel.QueueBind(
		deadQueue.Name,
		deadQueueName,
		deadLetterExchange,
		false,
		nil,
	)
	if err != nil {
		return amqp.Queue{}, fmt.Errorf(
			"bind dead queue: %w",
			err,
		)
	}
	_, err = channel.QueueDeclare(
		retryQueueName,
		true,
		false,
		false,
		false,
		amqp.Table{
			"x-message-ttl":             retryDelayMillis,
			"x-dead-letter-exchange":    "",
			"x-dead-letter-routing-key": taskQueueName,
		},
	)
	if err != nil {
		return amqp.Queue{}, fmt.Errorf(
			"declare retry queue: %w",
			err,
		)
	}

	return taskQueue, nil

}
