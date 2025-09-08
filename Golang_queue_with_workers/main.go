package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"math/rand"
	"net/http"
	"os"
	"os/signal"
	"strconv"
	"sync"
	"syscall"
	"time"
)

type Json_job struct {
	Id          string `json:"id"`
	Payload     string `json:"payload"`
	Max_retries int    `json:"max_retries"`
}

type Json_job_status struct {
	Attempts int
	Status   string
	mu       sync.Mutex
}

var chan_job chan Json_job
var job_status sync.Map

func initQueueSize() {
	QUEUE_SIZE := 64

	str_from_term := os.Getenv("QUEUE_SIZE")
	if str_from_term != "" {
		count, err := strconv.Atoi(str_from_term)
		if err == nil {
			QUEUE_SIZE = count
		}
	}

	chan_job = make(chan Json_job, QUEUE_SIZE)
}
func initWorkersSize() int {
	WORKERS := 4

	str_from_term := os.Getenv("WORKERS")
	if str_from_term != "" {
		count, err := strconv.Atoi(str_from_term)
		if err == nil {
			WORKERS = count
		}
	}

	return WORKERS
}

func worker(id int, job_chan <-chan Json_job, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := range job_chan {
		processJob(id, i)
	}
}

func Backoff_with_jitter(retries int) time.Duration {
	baseDelay := 100 * time.Microsecond
	maxDelay := 5 * time.Second

	exp := baseDelay * time.Duration(math.Pow(2, float64(retries)))
	if exp > maxDelay {
		exp = maxDelay
	}
	jitter := time.Duration(rand.Int63n(int64(exp)))
	return jitter
}

func processJob(worker_id int, job_chan Json_job) {
	statusAny, _ := job_status.Load(job_chan.Id)
	status, ok := statusAny.(*Json_job_status)
	if !ok {
		status = &Json_job_status{}
		job_status.Store(job_chan.Id, status)
	}
	status.mu.Lock()
	status.Status = "running"
	status.mu.Unlock()

	for {
		workTime := time.Duration(rand.Intn(400)+100) * time.Millisecond
		time.Sleep(workTime)
		if rand.Float32() < 0.2 {
			status.mu.Lock()
			status.Attempts++
			if status.Attempts > job_chan.Max_retries {
				status.Status = "failed"
				log.Printf("[Worker: %d] job %s Failed after %d attempts", worker_id, job_chan.Id, status.Attempts)
				status.mu.Unlock()
				return
			}
			backoff := Backoff_with_jitter(status.Attempts)
			log.Printf("[Worker: %d] job %s Failed, retrying in %v [Attempt: %d]", worker_id, job_chan.Id, backoff, status.Attempts)
			status.mu.Unlock()
			time.Sleep(backoff)
			continue
		}
		status.mu.Lock()
		status.Attempts++
		status.Status = "done"
		log.Printf("[Worker: %d] job %s Done after %d attempts", worker_id, job_chan.Id, status.Attempts)
		status.mu.Unlock()
		return
	}

}

func queueHandler(w http.ResponseWriter, r *http.Request) {
	var job Json_job

	err := json.NewDecoder(r.Body).Decode(&job)
	if err != nil {
		http.Error(w, "Invalid Json", http.StatusBadRequest)
		log.Println(http.StatusBadRequest)
		return
	}
	status := &Json_job_status{Attempts: 0, Status: "queued"}
	job_status.Store(job.Id, status)

	select {
	case chan_job <- job:
		w.WriteHeader(http.StatusAccepted)
		fmt.Fprintf(w, "job %s enqueued\n", job.Id)
	default:
		http.Error(w, "queue is full", http.StatusServiceUnavailable)
	}
}

func healthzHandler(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("OK\n"))
}

func main() {
	initQueueSize()
	workers := initWorkersSize()
	fmt.Printf("Workers: %d, Queues: %d\n", workers, cap(chan_job))
	var wg sync.WaitGroup
	for i := 1; i <= workers; i++ {
		wg.Add(1)
		go worker(i, chan_job, &wg)
	}
	mux := http.NewServeMux()
	mux.HandleFunc("/enqueue", queueHandler)
	mux.HandleFunc("/healthz", healthzHandler)

	server := &http.Server{Addr: ":8080", Handler: mux}
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, os.Interrupt, syscall.SIGTERM)

	go func() {
		<-quit
		log.Println("Shutting down server ...")
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		err := server.Shutdown(ctx)
		if err != nil {
			log.Fatalf("Server forsed to shut down: %v", err)
		}
		close(chan_job)
		wg.Wait()
	}()

	fmt.Println("localhost :8080 is started")
	err := server.ListenAndServe()
	if err != nil && err != http.ErrServerClosed {
		log.Fatalf("Could`t listen: %v\n", err)
	}
	log.Println("Server exited gracefully")
}
