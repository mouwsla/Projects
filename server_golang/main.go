package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	_ "modernc.org/sqlite"
)

// функция открытия баз данных
func initDB() (*sql.DB, *sql.DB, error) {
	// Пути к картам
	pathLow := os.Getenv("MBTILES_LOW_PATH")
	pathHigh := os.Getenv("MBTILES_HIGH_PATH")

	if pathLow == "" {
		pathLow = "maps/russia_raster_z0_z6.mbtiles"
	}

	if pathHigh == "" {
		pathHigh = "maps/russia_raster_z5_z12.mbtiles"
	}

	// Открываем первую базу (z0-z6)
	dbLow, err := sql.Open("sqlite", "file:"+pathLow+"?mode=ro")
	if err != nil {
		return nil, nil, fmt.Errorf("Ошибка открытия первой базы: %w", err)
	}

	if err := dbLow.Ping(); err != nil {
		dbLow.Close()
		return nil, nil, fmt.Errorf("Первая база недоступна: %w", err)
	}

	// Открываем вторую базу (z7-z12)
	dbHigh, err := sql.Open("sqlite", "file:"+pathHigh+"?mode=ro")
	if err != nil {
		dbLow.Close()
		return nil, nil, fmt.Errorf("Ошибка открытия второй базы: %w", err)
	}

	if err := dbHigh.Ping(); err != nil {
		dbHigh.Close()
		dbLow.Close()
		return nil, nil, fmt.Errorf("Вторая база недоступна: %w", err)
	}
	// Сохраняем до 4 свободных соединений для повторного использования
	dbLow.SetMaxIdleConns(4)
	dbHigh.SetMaxIdleConns(4)

	return dbLow, dbHigh, nil
}

// функция чтения метаднных баз данных
func readMetadataFromSql(db *sql.DB) (map[string]string, error) {
	rows, err := db.Query("SELECT name, value FROM metadata")
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	metadata := make(map[string]string)
	for rows.Next() {
		var key, value string
		if err := rows.Scan(&key, &value); err != nil {
			return nil, err
		}
		metadata[key] = value
	}
	if err := rows.Err(); err != nil {
		return nil, err
	}
	return metadata, nil

}

// Функция правильного завершения сервера
func gracefulShutdown(server *http.Server) error {
	// Созадем контекст закрытия
	ctx, stop := signal.NotifyContext(
		context.Background(), // Корневой чистый контекст
		os.Interrupt,         // Прерывание из терминала (Ctrl+C / SIGINT)
		syscall.SIGTERM,      // Сигнал завершения от ОС (например, от Docker/Kubernetes)
	)

	defer stop()
	// Создаем канал общения
	chanError := make(chan error, 1)
	go func() {
		chanError <- server.ListenAndServe()
	}()
	log.Println("Запуск Map Server на :8080")

	// обработка сигналов
	select {
	case <-ctx.Done(): // Получен SIGINT или SIGTERM
		log.Println("Получен сигнал остановки")
	case err := <-chanError: // Получаем результат завершения HTTP-сервера
		if err != nil && !errors.Is(err, http.ErrServerClosed) {
			return fmt.Errorf("Ошибка HTTP-сервера: %w", err)
		}
		return nil
	}
	// Прекращаем перехват сигналов ОС
	stop()
	// Даём серверу 8 секунд на завершение запросов
	shutdownCtx, cancel := context.WithTimeout(
		context.Background(),
		8*time.Second, // даем 8 секунд
	)
	defer cancel()
	shutdownErr := server.Shutdown(shutdownCtx)

	// Корректно останавливаем сервер
	if shutdownErr != nil {
		log.Println("Ошибка остановки сервера:", shutdownErr)

		if err := server.Close(); err != nil {
			log.Println("Ошибка закрытия соединений:", err)
		}
	}

	if err := <-chanError; err != nil && !errors.Is(err, http.ErrServerClosed) {
		return fmt.Errorf("HTTP-сервер завершился с ошибкой: %w", err)
	}

	if shutdownErr != nil {
		return fmt.Errorf("Не удалось корректно остановить сервер: %w", shutdownErr)
	}

	log.Println("Server successfully shutdown")
	return nil

}

// Создаём маршрутизатор и регистрируем HTTP-обработчики
func createRoutes(dbLow, dbHigh *sql.DB, lowMetadata, highMetadata map[string]string) *http.ServeMux {
	// Создаём HTTP-маршрутизатор
	mux := http.NewServeMux()

	// Проверка состояния сервера
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "OK")
	})

	// Возвращаем ранее загруженные метаданные обеих карт в JSON
	mux.HandleFunc("GET /metadata", func(w http.ResponseWriter, r *http.Request) {
		allMetadata := map[string]map[string]string{
			"lowDataBase":  lowMetadata,
			"highDataBase": highMetadata,
		}
		w.Header().Set("Content-Type", "application/json")
		encoder := json.NewEncoder(w)

		encoder.SetIndent("", "    ")
		if err := encoder.Encode(allMetadata); err != nil {
			log.Println("Ошибка отправки JSON:", err)
		}
	})

	// Получение картографического тайла
	mux.HandleFunc("GET /tiles/{z}/{x}/{tile}", func(w http.ResponseWriter, r *http.Request) {

		// Получаем координаты из URL
		z, errZ := strconv.Atoi(r.PathValue("z"))
		x, errX := strconv.Atoi(r.PathValue("x"))

		filename := r.PathValue("tile")

		if !strings.HasSuffix(filename, ".png") {
			http.Error(w, "Неверный формат тайла", http.StatusBadRequest)
			return
		}

		y, errY := strconv.Atoi(strings.TrimSuffix(filename, ".png"))

		// Проверяем корректность координат
		if errZ != nil || errX != nil || errY != nil || z < 0 {
			http.Error(w, "Неверные координаты", http.StatusBadRequest)
			return
		}

		// Проверяем поддерживаемый масштаб
		if z > 12 {
			http.Error(w, "Масштаб не поддерживается", http.StatusNotFound)
			return
		}

		// Максимальная координата X и Y для масштаба Z (эквивалент 2^Z - 1)
		maxCoord := (1 << uint(z)) - 1

		if x < 0 || x > maxCoord || y < 0 || y > maxCoord {
			http.Error(w, "Координаты вне диапазона", http.StatusBadRequest)
			return
		}

		// Выбираем базу по масштабу
		var selectedDB *sql.DB

		switch {
		case z <= 6:
			selectedDB = dbLow

		case z <= 12:
			selectedDB = dbHigh
		}

		// Преобразуем XYZ в TMS
		tmsY := maxCoord - y

		// Получаем PNG из SQLite
		var tile []byte

		err := selectedDB.QueryRowContext(
			r.Context(),
			`SELECT tile_data
			 FROM tiles
			 WHERE zoom_level = ?
			   AND tile_column = ?
			   AND tile_row = ?`,
			z, x, tmsY,
		).Scan(&tile)

		// Тайл отсутствует
		if errors.Is(err, sql.ErrNoRows) {
			http.Error(w, "Тайл не найден", http.StatusNotFound)
			return
		}

		// Ошибка базы данных
		if err != nil {
			log.Println("Ошибка SQLite:", err)
			http.Error(w, "Внутренняя ошибка сервера", http.StatusInternalServerError)
			return
		}

		// Отправляем изображение клиенту
		w.Header().Set("Content-Type", "image/png")
		w.Header().Set("Cache-Control", "public, max-age=86400")

		if _, err := w.Write(tile); err != nil {
			log.Println("Ошибка отправки тайла:", err)
		}
	})
	return mux
}

func runServer() error {
	// Вызываем функцию и получаем готовые коннекты
	dbLow, dbHigh, err := initDB()

	if err != nil {
		return err
	}

	// Закрываем базы при выходе из runServer()
	defer dbLow.Close()
	defer dbHigh.Close()

	log.Println("Обе базы данных успешно открыты")
	lowMetadata, err := readMetadataFromSql(dbLow)
	if err != nil {
		return fmt.Errorf("Ошибка чтения метаданных первой базы: %w", err)
	}
	highMetadata, err := readMetadataFromSql(dbHigh)
	if err != nil {
		return fmt.Errorf("Ошибка чтения метаданных второй базы: %w", err)
	}

	mux := createRoutes(dbLow, dbHigh, lowMetadata, highMetadata)
	// Запуск HTTP-сервера
	server := &http.Server{
		Addr:              ":8080",          // Адрес и порт HTTP-сервера
		Handler:           mux,              // Маршрутизатор HTTP-запросов
		ReadHeaderTimeout: 5 * time.Second,  // Время на чтение заголовков запроса
		WriteTimeout:      15 * time.Second, // Ограничение времени записи ответа
		IdleTimeout:       60 * time.Second, // Ожидание следующего запроса в keep-alive соединении
	}

	return gracefulShutdown(server)
}

// Основная главная функция
func main() {
	if err := runServer(); err != nil {
		log.Fatal("Ошибка приложения: ", err)
	}
}
