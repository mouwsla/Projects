package main

import (
	"bytes"
	"database/sql"
	"encoding/json"
	"image"
	"image/color"
	"image/png"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"reflect"
	"testing"
)

// Создаём временную SQLite-базу для тестов.
func newTestDB(t *testing.T) *sql.DB {
	t.Helper()

	path := filepath.Join(t.TempDir(), "test.mbtiles")

	db, err := sql.Open("sqlite", path)
	if err != nil {
		t.Fatal("Не удалось создать тестовую базу:", err)
	}

	t.Cleanup(func() {
		db.Close()
	})

	queries := []string{
		`CREATE TABLE tiles (
			zoom_level INTEGER,
			tile_column INTEGER,
			tile_row INTEGER,
			tile_data BLOB
		)`,
		`CREATE TABLE metadata (
			name TEXT,
			value TEXT
		)`,
	}

	for _, query := range queries {
		if _, err := db.Exec(query); err != nil {
			t.Fatal("Ошибка создания таблицы:", err)
		}
	}

	return db
}

// Отправляем HTTP-запрос напрямую в маршрутизатор.
func testRequest(mux *http.ServeMux, path string) *httptest.ResponseRecorder {
	req := httptest.NewRequest(http.MethodGet, path, nil)
	rec := httptest.NewRecorder()

	mux.ServeHTTP(rec, req)

	return rec
}

// Создаём настоящий PNG размером 1x1 пиксель.
func makeTestPNG(t *testing.T, c color.RGBA) []byte {
	t.Helper()

	img := image.NewRGBA(image.Rect(0, 0, 1, 1))
	img.SetRGBA(0, 0, c)

	var buf bytes.Buffer

	if err := png.Encode(&buf, img); err != nil {
		t.Fatal("Ошибка создания PNG:", err)
	}

	return buf.Bytes()
}

// Проверяем доступность сервера.
func TestHealth(t *testing.T) {
	mux := createRoutes(nil, nil, nil, nil)
	rec := testRequest(mux, "/health")

	if rec.Code != http.StatusOK {
		t.Fatalf("Ожидали 200, получили %d", rec.Code)
	}

	if rec.Body.String() != "OK\n" {
		t.Fatalf("Неверный ответ: %q", rec.Body.String())
	}
}

// Проверяем получение метаданных в JSON.
func TestMetadata(t *testing.T) {
	low := map[string]string{
		"name":    "Test Low",
		"minzoom": "0",
		"maxzoom": "6",
	}

	high := map[string]string{
		"name":    "Test High",
		"minzoom": "5",
		"maxzoom": "12",
	}

	mux := createRoutes(nil, nil, low, high)
	rec := testRequest(mux, "/metadata")

	if rec.Code != http.StatusOK {
		t.Fatalf("Ожидали 200, получили %d", rec.Code)
	}

	if got := rec.Header().Get("Content-Type"); got != "application/json" {
		t.Fatalf("Неверный Content-Type: %q", got)
	}

	var result map[string]map[string]string

	if err := json.Unmarshal(rec.Body.Bytes(), &result); err != nil {
		t.Fatalf("Некорректный JSON: %v", err)
	}

	expected := map[string]map[string]string{
		"lowDataBase":  low,
		"highDataBase": high,
	}

	if !reflect.DeepEqual(result, expected) {
		t.Fatalf("Неверные метаданные: %v", result)
	}
}

// Проверяем чтение метаданных непосредственно из SQLite.
func TestReadMetadataFromSQL(t *testing.T) {
	db := newTestDB(t)

	_, err := db.Exec(
		"INSERT INTO metadata (name, value) VALUES (?, ?)",
		"name", "Test Map",
	)
	if err != nil {
		t.Fatal(err)
	}

	metadata, err := readMetadataFromSql(db)
	if err != nil {
		t.Fatal("Ошибка чтения метаданных:", err)
	}

	if metadata["name"] != "Test Map" {
		t.Fatalf("Неверное название карты: %q", metadata["name"])
	}
}

// Проверяем некорректные запросы тайлов.
func TestInvalidTiles(t *testing.T) {
	mux := createRoutes(nil, nil, nil, nil)

	tests := []struct {
		name   string
		path   string
		status int
		body   string
	}{
		{
			name:   "Неподдерживаемый масштаб",
			path:   "/tiles/15/0/0.png",
			status: http.StatusNotFound,
			body:   "Масштаб не поддерживается\n",
		},
		{
			name:   "Координата X вне диапазона",
			path:   "/tiles/3/100/2.png",
			status: http.StatusBadRequest,
			body:   "Координаты вне диапазона\n",
		},
		{
			name:   "Отрицательная координата X",
			path:   "/tiles/3/-1/0.png",
			status: http.StatusBadRequest,
			body:   "Координаты вне диапазона\n",
		},
		{
			name:   "Нечисловой масштаб",
			path:   "/tiles/abc/0/0.png",
			status: http.StatusBadRequest,
			body:   "Неверные координаты\n",
		},
		{
			name:   "Нечисловая координата Y",
			path:   "/tiles/3/0/abc.png",
			status: http.StatusBadRequest,
			body:   "Неверные координаты\n",
		},
		{
			name:   "Неверный формат изображения",
			path:   "/tiles/3/0/0.jpg",
			status: http.StatusBadRequest,
			body:   "Неверный формат тайла\n",
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			rec := testRequest(mux, tt.path)

			if rec.Code != tt.status {
				t.Fatalf("Ожидали %d, получили %d", tt.status, rec.Code)
			}

			if rec.Body.String() != tt.body {
				t.Fatalf("Неверный ответ: %q", rec.Body.String())
			}
		})
	}
}

// Проверяем получение PNG из обеих SQLite-баз.
func TestTilesPNG(t *testing.T) {
	dbLow := newTestDB(t)
	dbHigh := newTestDB(t)

	redPNG := makeTestPNG(t, color.RGBA{R: 255, A: 255})
	greenPNG := makeTestPNG(t, color.RGBA{G: 255, A: 255})

	// Масштаб z=3: XYZ y=2 преобразуется в TMS y=5.
	_, err := dbLow.Exec(
		`INSERT INTO tiles
		 (zoom_level, tile_column, tile_row, tile_data)
		 VALUES (?, ?, ?, ?)`,
		3, 2, 5, redPNG,
	)
	if err != nil {
		t.Fatal("Ошибка добавления тайла в первую базу:", err)
	}

	// Масштаб z=7: XYZ y=4 преобразуется в TMS y=123.
	_, err = dbHigh.Exec(
		`INSERT INTO tiles
		 (zoom_level, tile_column, tile_row, tile_data)
		 VALUES (?, ?, ?, ?)`,
		7, 3, 123, greenPNG,
	)
	if err != nil {
		t.Fatal("Ошибка добавления тайла во вторую базу:", err)
	}

	mux := createRoutes(dbLow, dbHigh, nil, nil)

	tests := []struct {
		name string
		path string
		png  []byte
	}{
		{"Первая база", "/tiles/3/2/2.png", redPNG},
		{"Вторая база", "/tiles/7/3/4.png", greenPNG},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			rec := testRequest(mux, tt.path)

			if rec.Code != http.StatusOK {
				t.Fatalf("Ожидали 200, получили %d", rec.Code)
			}

			if got := rec.Header().Get("Content-Type"); got != "image/png" {
				t.Fatalf("Неверный Content-Type: %q", got)
			}

			if !bytes.Equal(rec.Body.Bytes(), tt.png) {
				t.Fatal("Полученный PNG не совпадает с оригиналом")
			}
		})
	}
}

// Проверяем запрос отсутствующего тайла.
func TestTileNotFound(t *testing.T) {
	dbLow := newTestDB(t)
	dbHigh := newTestDB(t)

	mux := createRoutes(dbLow, dbHigh, nil, nil)
	rec := testRequest(mux, "/tiles/3/2/2.png")

	if rec.Code != http.StatusNotFound {
		t.Fatalf("Ожидали 404, получили %d", rec.Code)
	}

	if rec.Body.String() != "Тайл не найден\n" {
		t.Fatalf("Неверный ответ: %q", rec.Body.String())
	}
}

// Проверяем обработку ошибки SQLite.
func TestTileDatabaseError(t *testing.T) {
	dbLow := newTestDB(t)
	dbHigh := newTestDB(t)

	mux := createRoutes(dbLow, dbHigh, nil, nil)

	// Имитируем потерю подключения к первой базе.
	if err := dbLow.Close(); err != nil {
		t.Fatal(err)
	}

	rec := testRequest(mux, "/tiles/3/2/2.png")

	if rec.Code != http.StatusInternalServerError {
		t.Fatalf("Ожидали 500, получили %d", rec.Code)
	}
}
