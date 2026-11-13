package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"
	"server_go_redis_rabbit/internal/taskstate"
	"syscall"
	"time"
)

func main() {

	redisClient, redisAddress, err := taskstate.OpenRedis()
	if err != nil {
		log.Fatal(err)
	}

	defer func() {
		if err := redisClient.Close(); err != nil {
			log.Printf("failed to close Redis: %v", err)
		}
	}()
	rabbitConnection, rabbitChannel, queue, err := initRabbitMQ()
	if err != nil {
		log.Fatal(err)
	}

	defer rabbitConnection.Close()
	defer rabbitChannel.Close()

	app := &Application{
		redisClient:      redisClient,
		rabbitConnection: rabbitConnection,
		rabbitChannel:    rabbitChannel,
		taskQueueName:    queue.Name,
	}

	mux := http.NewServeMux()

	mux.HandleFunc("GET /health", app.healthHandler)
	mux.HandleFunc("GET /visits", app.visitsHandler)
	mux.HandleFunc("POST /tasks", app.createTaskHandler)
	mux.HandleFunc("GET /tasks/{id}", app.taskStatusHandler)

	server := &http.Server{
		Addr:              ":8080",
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
	}

	log.Println("Server is listening on http://localhost:8080")
	log.Printf("Connected to Redis at %s", redisAddress)
	log.Println("Connected to RabbitMQ")

	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	defer stop()

	serverErrors := make(chan error, 1)

	go func() {
		serverErrors <- server.ListenAndServe()
	}()

	select {
	case <-ctx.Done():
		log.Println("Shutting down API...")

		shutdownCtx, cancel := context.WithTimeout(
			context.Background(),
			15*time.Second,
		)
		defer cancel()

		if err := server.Shutdown(shutdownCtx); err != nil {
			log.Printf("HTTP shutdown error: %v", err)
		}

		if err := <-serverErrors; err != nil &&
			!errors.Is(err, http.ErrServerClosed) {
			log.Printf("HTTP server stopped: %v", err)
		}

	case err := <-serverErrors:
		if err != nil &&
			!errors.Is(err, http.ErrServerClosed) {
			log.Printf("HTTP server error: %v", err)
		}
	}

}
