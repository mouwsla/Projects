package main

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"log"
	"time"
)

const taskStatusTTL = 24 * time.Hour

func taskKey(id string) string {
	return "task:" + id
}

func newTaskID() (string, error) {
	data := make([]byte, 16)
	if _, err := rand.Read(data); err != nil {
		return "", fmt.Errorf("generate task ID: %w", err)
	}
	return hex.EncodeToString(data), nil
}

func (app *Application) setTaskStatus(
	taskID string,
	status string,
) error {
	ctx, cancel := context.WithTimeout(
		context.Background(),
		5*time.Second,
	)
	defer cancel()
	err := app.redisClient.Set(
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
	log.Printf(
		"Task %s status changed to %s",
		taskID,
		status,
	)

	return nil

}
