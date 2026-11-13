package main

import (
	"context"
	"fmt"
	"time"

	amqp "github.com/rabbitmq/amqp091-go"
)

const publisherConfirmTimeout = 5 * time.Second

func (app *Application) publishTask(
	ctx context.Context,
	body []byte,
) error {
	app.rabbitMutex.Lock()
	defer app.rabbitMutex.Unlock()

	confirmation, err := app.rabbitChannel.PublishWithDeferredConfirmWithContext(
		ctx,
		"",
		app.taskQueueName,
		false,
		false,
		amqp.Publishing{
			ContentType:  "application/json",
			DeliveryMode: amqp.Persistent,
			Timestamp:    time.Now(),
			Body:         body,
		},
	)
	if err != nil {
		return fmt.Errorf("publish task: %w", err)
	}
	confirmContext, cancel := context.WithTimeout(
		ctx,
		publisherConfirmTimeout,
	)
	defer cancel()
	acknowledged, err := confirmation.WaitContext(confirmContext)
	if err != nil {
		return fmt.Errorf("wait for publisher confirmation: %w", err)
	}

	if !acknowledged {
		return fmt.Errorf("RabbitMQ rejected the published task")
	}

	return nil

}
