package main

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/redis/go-redis/v9"
)

const taskStatusTTL = 24 * time.Hour

// Redis init
func initRedis() (
	*redis.Client,
	error,
) {
	redisAddress := os.Getenv("REDIS_ADDR")
	if redisAddress == "" {
		redisAddress = "localhost:6379"
	}

	redisClient := redis.NewClient(&redis.Options{
		Addr: redisAddress,
	})

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := redisClient.Ping(ctx).Err(); err != nil {
		_ = redisClient.Close()

		return nil, fmt.Errorf(
			"failed to connect to Redis: %w",
			err,
		)
	}
	return redisClient, nil
}

func taskKey(taskID string) string {
	return "task:" + taskID
}

func setTaskStatus(
	reddisClient *redis.Client,
	taskID string,
	status string,
) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	err := reddisClient.Set(
		ctx,
		taskKey(taskID),
		status,
		taskStatusTTL,
	).Err()
	if err != nil {
		return fmt.Errorf(
			"set task %s status to %s: %w",
			taskID,
			status,
			err,
		)
	}
	return nil
}
