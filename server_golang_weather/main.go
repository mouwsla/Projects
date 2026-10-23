package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"strings"
	"time"
)

// structs for getting `name` `lan` `lot`
type Location struct {
	Name      string  `json:"name"`
	Latitude  float64 `json:"latitude"`
	Longitude float64 `json:"longitude"`
}
type GeocodingResponse struct {
	Result []Location `json:"results"`
}

// structs for getting temperature
type Params struct {
	Temp      *float64 `json:"temperature_2m"`
	Humidity  *float64 `json:"relative_humidity_2m"`
	WindSpeed *float64 `json:"wind_speed_10m"`
	Pressure  *float64 `json:"surface_pressure"`
}
type GeocodingParams struct {
	Result_param Params `json:"current"`
}

var errCityNotFound = errors.New("город не найден")

func findLocation(client *http.Client, city string) (Location, error) {
	// URL for get lan lot from name-city
	apiURL := "https://geocoding-api.open-meteo.com/v1/search?name=" + url.QueryEscape(city) + "&count=1&language=ru"
	log.Println("Адрес запроса:", apiURL)

	// getting body json from apiURL
	resp, err := client.Get(apiURL)
	// check
	if err != nil {
		log.Println("Ошибка запроса к Open-Meteo:", err)
		return Location{}, fmt.Errorf("запрос координат: %w", err)
	}
	// defer close GATE
	defer resp.Body.Close()

	// check
	if resp.StatusCode != http.StatusOK {
		log.Println("Неожиданный статус Open-Meteo:", resp.StatusCode)
		return Location{}, fmt.Errorf("API координат вернул статус %d", resp.StatusCode)
	}

	// reading json struct
	body, err := io.ReadAll(resp.Body)
	// check
	if err != nil {
		log.Println("Неудалось прочиать даныне", err)
		return Location{}, fmt.Errorf("чтение ответа с координатами: %w", err)
	}
	// trimming our json for components of params `name` `lan` `lot`
	var data GeocodingResponse
	err = json.Unmarshal(body, &data)

	// check
	if err != nil {
		log.Println("Нету данных/Данные пустые", err)
		return Location{}, fmt.Errorf("разбор ответа с координатами: %w", err)
	}
	if len(data.Result) == 0 {
		return Location{}, fmt.Errorf(
			"город %q: %w",
			city,
			errCityNotFound,
		)
	}
	return data.Result[0], nil

}
func getWeather(client *http.Client, location Location) (Params, error) {
	// Получение погоды
	weatherURL := fmt.Sprintf(
		"https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current=temperature_2m,relative_humidity_2m,wind_speed_10m,surface_pressure&wind_speed_unit=ms",
		location.Latitude,
		location.Longitude,
	)

	// getting body json from weatherURL
	weatherResp, err := client.Get(weatherURL)
	// check
	if err != nil {
		log.Println("Ошибка запроса к Open-Meteo:", err)
		return Params{}, fmt.Errorf("запрос погоды: %w", err)
	}
	defer weatherResp.Body.Close()
	// check
	if weatherResp.StatusCode != http.StatusOK {
		log.Println("Неожиданный статус Open-Meteo:", weatherResp.StatusCode)
		return Params{}, fmt.Errorf("API погоды вернул статус %d", weatherResp.StatusCode)
	}

	// reading json struct
	weatherBody, err := io.ReadAll(weatherResp.Body)
	// check
	if err != nil {
		log.Println("Неудалось прочиать даныне", err)
		return Params{}, fmt.Errorf("чтение ответа с погодой: %w", err)
	}

	// trimming our json for components of params `temp`
	var data_params GeocodingParams
	err = json.Unmarshal(weatherBody, &data_params)

	// check
	if err != nil {
		log.Println("Нету данных/Данные пустые", err)
		return Params{}, fmt.Errorf("разбор ответа с погодой: %w", err)
	}
	if data_params.Result_param.Temp == nil ||
		data_params.Result_param.Humidity == nil ||
		data_params.Result_param.WindSpeed == nil ||
		data_params.Result_param.Pressure == nil {

		log.Println("В ответе API отсутствуют погодные данные")
		return Params{}, errors.New("в ответе API отсутствуют погодные данные")
	}
	return data_params.Result_param, nil
}

func main() {

	// Создаём HTTP-маршрутизатор
	mux := http.NewServeMux()
	client := &http.Client{
		Timeout: 10 * time.Second,
	}

	// Проверка состояния сервера
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "OK")
	})

	// получение запроса
	mux.HandleFunc("GET /weather", func(w http.ResponseWriter, r *http.Request) {
		city := r.URL.Query().Get("city")
		city = strings.TrimSpace(city)
		// check
		if city == "" {
			log.Println("Пустой параметр")
			http.Error(w, "Параметр city обязателен", http.StatusBadRequest)
			return
		}

		// GET temp from `lan` `lot` param from URL
		location, err := findLocation(client, city)
		if err != nil {
			log.Println("Ошибка findLocation:", err)

			if errors.Is(err, errCityNotFound) {
				http.Error(w, "Город не найден", http.StatusNotFound)
				return
			}

			http.Error(
				w,
				"Не удалось получить данные о местоположении",
				http.StatusBadGateway,
			)
			return
		}
		weather, err := getWeather(client, location)
		if err != nil {
			log.Println("Ошибка getWeather:", err)
			http.Error(w, "Не удалось получить метеоданные", http.StatusBadGateway)
			return
		}

		fmt.Fprintf(w, "City: %s\nTemperature: %.1f°C\nHumidity: %.0f%%\nWind: %.1f м/с\nPressure: %.1f гПа\n",
			location.Name,
			*weather.Temp,
			*weather.Humidity,
			*weather.WindSpeed,
			*weather.Pressure)

	})

	// Запуск HTTP-сервера
	log.Println("Weather Server запущен на :8080")
	log.Fatal(http.ListenAndServe(":8080", mux))
}
