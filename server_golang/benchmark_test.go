package main

import (
	"database/sql"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"sync/atomic"
	"testing"
	"time"
)

// Выбираем до limit существующих тайлов и переводим TMS-Y в XYZ-Y.
func benchmarkHTTPPaths(b *testing.B, db *sql.DB, z, limit int) []string {
	b.Helper()

	rows, err := db.Query(
		`SELECT tile_column, tile_row
         FROM tiles
         WHERE zoom_level = ?
         LIMIT ?`,
		z, limit,
	)
	if err != nil {
		b.Fatalf("Ошибка поиска тайлов z=%d: %v", z, err)
	}
	defer rows.Close()

	paths := make([]string, 0, limit)
	maxCoord := (1 << uint(z)) - 1

	for rows.Next() {
		var x, tmsY int
		if err := rows.Scan(&x, &tmsY); err != nil {
			b.Fatalf("Ошибка чтения координат: %v", err)
		}
		y := maxCoord - tmsY
		paths = append(paths, fmt.Sprintf("/tiles/%d/%d/%d.png", z, x, y))
	}
	if err := rows.Err(); err != nil {
		b.Fatalf("Ошибка чтения списка тайлов: %v", err)
	}
	if len(paths) == 0 {
		b.Fatalf("В базе нет тайлов для z=%d", z)
	}
	return paths
}

// Выполняем реальный HTTP GET, полностью читаем и закрываем ответ.
func benchmarkHTTPGet(client *http.Client, url string) error {
	resp, err := client.Get(url)
	if err != nil {
		return err
	}

	_, readErr := io.Copy(io.Discard, resp.Body)
	closeErr := resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("HTTP %d: %s", resp.StatusCode, url)
	}
	if readErr != nil {
		return fmt.Errorf("ошибка чтения ответа: %w", readErr)
	}
	if closeErr != nil {
		return fmt.Errorf("ошибка закрытия ответа: %w", closeErr)
	}
	return nil
}

// Измеряем последовательные либо параллельные HTTP-запросы.
func benchmarkHTTPRequests(
	b *testing.B,
	client *http.Client,
	baseURL string,
	paths []string,
	parallel bool,
) {
	b.Helper()

	urls := make([]string, len(paths))
	for i, path := range paths {
		urls[i] = baseURL + path
	}

	b.ResetTimer()

	if !parallel {
		for i := 0; i < b.N; i++ {
			if err := benchmarkHTTPGet(client, urls[i%len(urls)]); err != nil {
				b.Fatal(err)
			}
		}
		return
	}

	// Каждая горутина начинает с другого тайла и затем перебирает остальные.
	var workerID atomic.Uint64
	b.RunParallel(func(pb *testing.PB) {
		index := int(workerID.Add(1) - 1)
		for pb.Next() {
			if err := benchmarkHTTPGet(client, urls[index%len(urls)]); err != nil {
				b.Error(err)
				return
			}
			index++
		}
	})
}

func BenchmarkHTTP(b *testing.B) {
	dbLow, dbHigh, err := initDB()
	if err != nil {
		b.Fatalf("Не удалось открыть MBTiles: %v", err)
	}
	defer dbLow.Close()
	defer dbHigh.Close()

	mux := createRoutes(dbLow, dbHigh, nil, nil)
	ts := httptest.NewServer(mux)
	defer ts.Close()

	// Увеличиваем пул свободных соединений для параллельного теста.
	transport := http.DefaultTransport.(*http.Transport).Clone()
	transport.MaxIdleConns = 128
	transport.MaxIdleConnsPerHost = 128
	transport.MaxConnsPerHost = 128
	defer transport.CloseIdleConnections()

	client := &http.Client{
		Transport: transport,
		Timeout:   10 * time.Second,
	}

	cases := []struct {
		name string
		db   *sql.DB
		z    int
	}{
		{"Low_z3", dbLow, 3},
		{"High_z7", dbHigh, 7},
	}

	for _, tc := range cases {
		paths := benchmarkHTTPPaths(b, tc.db, tc.z, 32)

		// Проверяем выбранные URL до начала измерений.
		for _, path := range paths {
			if err := benchmarkHTTPGet(client, ts.URL+path); err != nil {
				b.Fatalf("Проверка тайла %s: %v", path, err)
			}
		}

		b.Run(tc.name+"/Single/Sequential", func(b *testing.B) {
			benchmarkHTTPRequests(b, client, ts.URL, paths[:1], false)
		})
		b.Run(tc.name+"/Single/Parallel", func(b *testing.B) {
			benchmarkHTTPRequests(b, client, ts.URL, paths[:1], true)
		})
		b.Run(tc.name+"/Mixed/Sequential", func(b *testing.B) {
			benchmarkHTTPRequests(b, client, ts.URL, paths, false)
		})
		b.Run(tc.name+"/Mixed/Parallel", func(b *testing.B) {
			benchmarkHTTPRequests(b, client, ts.URL, paths, true)
		})
	}
}
