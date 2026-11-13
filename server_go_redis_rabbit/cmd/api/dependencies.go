package main

import (
	"fmt"
	"os"
	"time"

	amqp "github.com/rabbitmq/amqp091-go"
)

// Rabbit init
func initRabbitMQ() (
	*amqp.Connection,
	*amqp.Channel,
	amqp.Queue,
	error,
) {
	var emptyQueue amqp.Queue

	rabbitURL := os.Getenv("RABBITMQ_URL")
	if rabbitURL == "" {
		return nil, nil, emptyQueue, fmt.Errorf(
			"RABBITMQ_URL is not set",
		)
	}

	connection, err := amqp.DialConfig(
		rabbitURL,
		amqp.Config{
			Heartbeat: 10 * time.Second,
			Recovery: &amqp.Recovery{
				ReconnectionConfig: &amqp.ReconnectionConfig{
					MaxRetryCount: 20,
					RetryInterval: 3 * time.Second,
				},
				TopologyRecoveryMode: amqp.TopologyRecoveryAllEnabled,
			},
		},
	)
	if err != nil {
		return nil, nil, emptyQueue, fmt.Errorf(
			"failed to connect to RabbitMQ: %w",
			err,
		)
	}

	channel, err := connection.Channel()
	if err != nil {
		_ = connection.Close()

		return nil, nil, emptyQueue, fmt.Errorf(
			"failed to open RabbitMQ channel: %w",
			err,
		)
	}
	if err := channel.Confirm(false); err != nil {
		_ = channel.Close()
		_ = connection.Close()
		return nil, nil, emptyQueue, fmt.Errorf(
			"failed to enable publisher confirms: %w",
			err,
		)
	}

	queue, err := channel.QueueDeclare(
		"tasks",
		true,
		false,
		false,
		false,
		nil,
	)
	if err != nil {
		_ = channel.Close()
		_ = connection.Close()

		return nil, nil, emptyQueue, fmt.Errorf(
			"failed to declare queue: %w",
			err,
		)
	}

	return connection, channel, queue, nil
}
