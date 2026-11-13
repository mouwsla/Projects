package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	"server_go_redis_rabbit/internal/taskstate"

	amqp "github.com/rabbitmq/amqp091-go"
)

type Task struct {
	ID    string `json:"id"`
	Type  string `json:"type"`
	Email string `json:"email"`
}

func main() {
	redisClient, _, err := taskstate.OpenRedis()
	if err != nil {
		log.Fatal(err)
	}

	defer func() {
		if err := redisClient.Close(); err != nil {
			log.Printf("failed to close Redis: %v", err)
		}
	}()

	rabbitURL := os.Getenv("RABBITMQ_URL")
	if rabbitURL == "" {
		log.Fatal("RABBITMQ_URL is not set")
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
		log.Fatalf("failed to connect to RabbitMQ: %v", err)
	}
	defer connection.Close()

	channel, err := connection.Channel()
	if err != nil {
		log.Fatalf("failed to open RabbitMQ channel: %v", err)
	}
	defer channel.Close()

	queue, err := declareTaskTopology(channel)
	if err != nil {
		log.Fatalf(
			"failed to declare RabbitMQ topology: %v",
			err,
		)
	}
	if err = channel.Confirm(false); err != nil {
		log.Fatalf(
			"failed to enable publisher confirms: %v",
			err,
		)
	}

	err = channel.Qos(
		1,
		0,
		false,
	)
	if err != nil {
		log.Fatalf("failed to configure QoS: %v", err)
	}
	message, err := channel.Consume(
		queue.Name,
		"",
		false,
		false,
		false,
		false,
		nil,
	)
	if err != nil {
		log.Fatalf("failed to start consumer: %v", err)
	}

	log.Printf("Worker is waiting for messages from queue %q", queue.Name)

	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	defer stop()

	for {
		select {
		case <-ctx.Done():
			log.Println("Worker shutting down...")
			return

		case delivery, ok := <-message:
			if !ok {
				log.Println("RabbitMQ channel closed")
				return
			}

			if ctx.Err() != nil {
				requeueMessage(delivery)
				return
			}

			handleDelivery(
				redisClient,
				channel,
				delivery,
			)
		}
	}

}
