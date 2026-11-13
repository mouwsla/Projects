package main

import (
	"context"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"server_go_redis_rabbit/internal/taskstate"
	"time"

	"github.com/redis/go-redis/v9"
)

func (app *Application) healthHandler(
	w http.ResponseWriter,
	r *http.Request,
) {
	if err := app.redisClient.Ping(r.Context()).Err(); err != nil {
		http.Error(w, "Redis is unavailable", http.StatusServiceUnavailable)
		return
	}
	if app.rabbitConnection.IsClosed() ||
		app.rabbitChannel.IsClosed() {
		http.Error(w, "RabbitMQ is unavailable", http.StatusServiceUnavailable)
		return
	}

	response := HealthResponse{
		Status:   "ok",
		Redis:    "ok",
		RabbitMQ: "ok",
	}
	writeJSON(w, http.StatusOK, response)
}

func (app *Application) visitsHandler(
	w http.ResponseWriter,
	r *http.Request,
) {
	visits, err := app.redisClient.Incr(r.Context(), "visits").Result()
	if err != nil {
		log.Printf("failed to increment visits: %v", err)
		http.Error(w, "Internal server error", http.StatusInternalServerError)
		return
	}
	response := VisitsResponse{
		Visits: visits,
	}
	writeJSON(w, http.StatusOK, response)

}

func (app *Application) createTaskHandler(
	w http.ResponseWriter,
	r *http.Request,
) {
	var request TaskRequest

	r.Body = http.MaxBytesReader(w, r.Body, 4096)
	decoder := json.NewDecoder(r.Body)
	decoder.DisallowUnknownFields()

	if err := decoder.Decode(&request); err != nil {
		http.Error(
			w,
			"Invalid JSON body",
			http.StatusBadRequest,
		)
		return
	}
	var extra any
	if err := decoder.Decode(&extra); err != io.EOF {
		http.Error(w, "Expected a single JSON object",
			http.StatusBadRequest)
		return
	}
	if err := taskstate.Validate(
		request.Type,
		request.Email,
	); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	log.Printf(
		"Received task: type=%q email=%q",
		request.Type,
		request.Email,
	)

	if err := taskstate.Validate(
		request.Type,
		request.Email,
	); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	taskID, err := newTaskID()
	if err != nil {
		log.Printf("failed to create task ID: %v", err)
		http.Error(
			w,
			"Internal server error",
			http.StatusInternalServerError,
		)
		return
	}

	message := TaskMessage{
		ID:    taskID,
		Type:  request.Type,
		Email: request.Email,
	}

	body, err := json.Marshal(message)
	if err != nil {
		log.Printf("failed to encode task: %v", err)
		http.Error(
			w,
			"Internal server error",
			http.StatusInternalServerError,
		)
		return
	}

	// if err := app.setTaskStatus(
	// 	taskID,
	// 	"queued",
	// ); err != nil {
	// 	log.Printf("failed to save task status: %v", err)
	// 	http.Error(
	// 		w,
	// 		"Failed to save task",
	// 		http.StatusServiceUnavailable,
	// 	)
	// 	return
	// }
	if err := taskstate.Create(
		context.Background(),
		app.redisClient,
		taskID,
	); err != nil {
		log.Printf("failed to create task: %v", err)
		http.Error(w, "Failed to save task",
			http.StatusServiceUnavailable)
		return
	}

	ctx, cancel := context.WithTimeout(
		r.Context(),
		5*time.Second,
	)
	defer cancel()

	if err := app.publishTask(ctx, body); err != nil {
		log.Printf("failed to publish task: %v", err)

		statusCtx, cancel := context.WithTimeout(
			context.Background(),
			5*time.Second,
		)
		status, statusErr := taskstate.MarkPubIsUnknow(
			statusCtx,
			app.redisClient,
			taskID,
		)
		cancel()
		if statusErr != nil {
			log.Printf("failed to update status: %v",
				statusErr)
			status = "unknown"
		}
		writeJSON(w, http.StatusServiceUnavailable,
			TaskResponse{
				ID:     taskID,
				Status: status,
				Queue:  app.taskQueueName,
			},
		)
		return
	}

	// 	if statusErr := app.setTaskStatus(
	// 		taskID,
	// 		"failed",
	// 	); statusErr != nil {
	// 		log.Printf(
	// 			"failed to save failed status: %v",
	// 			statusErr,
	// 		)
	// 	}

	// 	response := TaskResponse{
	// 		ID:     taskID,
	// 		Status: "failed",
	// 		Queue:  app.taskQueueName,
	// 	}

	// 	writeJSON(
	// 		w,
	// 		http.StatusServiceUnavailable,
	// 		response,
	// 	)
	// 	return
	// }

	response := TaskResponse{
		ID:     taskID,
		Status: "queued",
		Queue:  app.taskQueueName,
	}

	writeJSON(
		w,
		http.StatusAccepted,
		response,
	)
}
func (app *Application) taskStatusHandler(
	w http.ResponseWriter,
	r *http.Request,
) {
	taskID := r.PathValue("id")

	status, err := app.redisClient.Get(
		r.Context(),
		// taskKey(taskID),
		taskstate.Key(taskID),
	).Result()

	if err == redis.Nil {
		http.Error(w, "Task not found", http.StatusNotFound)
		return
	}

	if err != nil {
		log.Printf("failed to get task status: %v", err)
		http.Error(w, "Internal server error", http.StatusInternalServerError)
		return
	}

	response := TaskStatusResponse{
		ID:     taskID,
		Status: status,
	}

	writeJSON(w, http.StatusOK, response)
}
