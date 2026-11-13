package main

import (
	"sync"

	amqp "github.com/rabbitmq/amqp091-go"
	"github.com/redis/go-redis/v9"
)

type Application struct {
	redisClient      *redis.Client
	rabbitConnection *amqp.Connection
	rabbitChannel    *amqp.Channel
	rabbitMutex      sync.Mutex
	taskQueueName    string
}

type HealthResponse struct {
	Status   string `json:"status"`
	Redis    string `json:"redis"`
	RabbitMQ string `json:"rabbitmq"`
}

type TaskRequest struct {
	Type  string `json:"type"`
	Email string `json:"email"`
}

type TaskMessage struct {
	ID    string `json:"id"`
	Type  string `json:"type"`
	Email string `json:"email"`
}

type TaskResponse struct {
	ID     string `json:"id"`
	Status string `json:"status"`
	Queue  string `json:"queue"`
}

type VisitsResponse struct {
	Visits int64 `json:"visits"`
}

type TaskStatusResponse struct {
	ID     string `json:"id"`
	Status string `json:"status"`
}
