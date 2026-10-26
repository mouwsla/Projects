# C, C++ и Go — коллекция проектов

14 самостоятельных проектов: библиотеки на C и C++, терминальные игры, визуализация, HTTP-сервисы на Go и погодное приложение с картой на Qt.

В репозитории собраны учебные задания и собственные эксперименты с памятью, алгоритмами, конкурентностью, сетевыми запросами и графическими интерфейсами. У каждого проекта свои исходники и способ сборки.

[Каталог](#каталог-проектов) · [Зависимости](#зависимости) · [Быстрый старт](#быстрый-старт) · [Библиотеки и игры](#сборка-библиотек-и-игр) · [Тестирование](#тестирование)

## Каталог проектов

| Проект | Что внутри | Стек |
| --- | --- | --- |
| [Map Server](server_golang/) | HTTP-сервер PNG-тайлов из двух MBTiles-баз, метаданные карт и graceful shutdown | Go, `net/http`, SQLite, Docker |
| [Weather Server](server_golang_weather/) | HTTP API: поиск города и текущая температура, влажность, ветер и давление через Open-Meteo | Go, `net/http`, JSON |
| [Очередь задач](Golang_queue_with_workers/) | Буферизированная очередь, пул воркеров, повторные попытки с задержкой и случайным разбросом | Go, goroutines, channels, `sync` |
| [AviWx](weather_prog/) | Анализ погодных рисков по сценарию маршрута, Qt-интерфейс, прогноз Open-Meteo и SXF-карты | C++20, CMake, Qt6, GDAL |
| [3D Cube](3D_Cube/) | Вращающийся ASCII-куб с перспективой, освещением, цветами и ручным управлением | C++17, POSIX terminal |
| [s21_string](C2_s21_stringplus-1-develop/src/) | Собственные функции работы со строками и памятью, часть возможностей `sprintf`, вставка и обрезка строк | C11, Check |
| [Simple Bash Utils](C3_SimpleBashUtils-2-develop/src/) | Реализации `cat` и `grep` с набором флагов и тестами сравнения вывода | C11, POSIX, regex |
| [s21_decimal](C5_s21_decimal.ID_353529-Team_TL_tobymayr.572ea178_7cff_4323-3-develop/src/) | Десятичный тип: арифметика, сравнения, преобразования и округление | C11, Check |
| [s21_matrix](C6_s21_matrix.ID_353530-1-develop/src/) | Операции над матрицами: арифметика, транспонирование, определитель и обратная матрица | C11, Check |
| [s21_matrixplus](CPP1_s21_matrixplus.ID_353533-1-develop/src/) | Класс `S21Matrix`, конструкторы, перегрузка операторов и матричные вычисления | C++20, GoogleTest |
| [BrickGame / Tetris](C7_BrickGame_v1.0.ID_1449897-1-develop/src/) | Тетрис с конечным автоматом состояний, семью фигурами, очками и рекордом | C11, ncurses, Check |
| [Pong](P01D06-1-develop/src/) | Игра для двух игроков в пошаговом и интерактивном режимах | C, ncurses |
| [Game of Life](P02D13-1-develop/src/) | Клеточный автомат на тороидальном поле, пять стартовых конфигураций и управление скоростью | C, ncurses |
| [Graph](P03D20-1-develop/src/) | Разбор выражений через обратную польскую нотацию и построение графика в терминале | C, math |

В каталогах учебных проектов сохранены исходные названия, условия заданий в `README*.md`, материалы и примеры. Реализация обычно находится в `src/`.

## Зависимости

Устанавливать весь стек сразу не требуется — зависимости выбираются под конкретный проект.

| Для чего | Что потребуется |
| --- | --- |
| C/C++ проекты | GCC или Clang; `make` для проектов с Makefile |
| 3D Cube | Компилятор C++17 и терминал с POSIX API |
| s21_matrixplus и AviWx | Компилятор C++20 |
| Pong, Game of Life, Tetris | ncurses; Makefile Тетриса также линкует Check при обычной сборке |
| Тесты библиотек на C | Check; в Linux Makefile также используют Subunit и системные библиотеки |
| Тесты s21_matrixplus | GoogleTest |
| AviWx: консоль и тесты | CMake 3.20+; при сборке через Makefile — `make` |
| AviWx: интерфейс и SXF-карты | Qt6 Widgets + Network для GUI, GDAL для чтения SXF |
| Go-сервисы | Go; в `go.mod` [Map Server](server_golang/go.mod) и [Weather Server](server_golang_weather/go.mod) указана версия `1.27.3` |
| Map Server в контейнере | Docker с Compose; два файла карт в формате MBTiles |
| Дополнительные проверки | `clang-format`, `cppcheck`, `gcovr` или `lcov`/`genhtml` — в зависимости от Makefile |

Терминальные приложения ориентированы на macOS/Linux. Для внешних C/C++ библиотек нужны заголовки и библиотеки, доступные компилятору и линковщику.

## Быстрый старт

Команды ниже выполняются **из корня репозитория**. Команды в скобках меняют рабочую папку только на время запуска.

### Map Server

Сервер читает картографические тайлы из SQLite-контейнеров MBTiles. Перед запуском положите базы карт в каталог `server_golang/maps/`:

```text
server_golang/maps/russia_raster_z0_z6.mbtiles
server_golang/maps/russia_raster_z5_z12.mbtiles
```

Запуск на хосте:

```sh
(cd server_golang && go run .)
```

Или в Docker Compose:

```sh
docker compose -f server_golang/compose.yaml up --build
```

Примеры запросов:

```sh
curl http://localhost:8080/health
curl http://localhost:8080/metadata
curl --output tile.png http://localhost:8080/tiles/3/4/2.png
```

| Запрос | Результат |
| --- | --- |
| `GET /health` | `200 OK`, тело `OK` |
| `GET /metadata` | Метаданные обеих MBTiles-баз в JSON |
| `GET /tiles/{z}/{x}/{y}.png` | PNG-тайл в XYZ-координатах для масштабов `0`–`12` |
| Неверные координаты | `400 Bad Request` |
| Неподдерживаемый масштаб или отсутствующий тайл | `404 Not Found` |

Пути к базам можно переопределить переменными `MBTILES_LOW_PATH` и `MBTILES_HIGH_PATH`. При остановке по `Ctrl+C`, `SIGINT` или `SIGTERM` сервер даёт активным запросам до 8 секунд на завершение.

### Weather Server

Сервис принимает название города, получает координаты и возвращает текущую погоду текстом.

```sh
(cd server_golang_weather && go run .)
```

В другом терминале:

```sh
curl http://localhost:8080/health
curl --get --data-urlencode 'city=Москва' http://localhost:8080/weather
```

| Запрос | Результат |
| --- | --- |
| `GET /health` | `200 OK`, тело `OK` |
| `GET /weather?city=...` | Город, температура в °C, влажность в %, ветер в м/с, давление в гПа |
| Пустой `city` | `400 Bad Request` |
| Город не найден | `404 Not Found` |
| Ошибка получения погодных данных | `502 Bad Gateway` |

Для получения погоды нужен доступ к Open-Meteo. Таймаут каждого исходящего HTTP-запроса — 10 секунд.

### Очередь задач с воркерами

У проекта пока нет `go.mod`, поэтому он запускается непосредственно через файл:

```sh
QUEUE_SIZE=64 WORKERS=4 go run Golang_queue_with_workers/main.go
```

Отправка задачи из другого терминала:

```sh
curl http://localhost:8080/healthz
curl -X POST http://localhost:8080/enqueue \
  -H 'Content-Type: application/json' \
  -d '{"id":"task-001","payload":"demo","max_retries":3}'
```

`QUEUE_SIZE` задаёт размер буфера, `WORKERS` — число воркеров; значения по умолчанию — `64` и `4`. Принятая задача получает ответ `202 Accepted`, заполненная очередь — `503 Service Unavailable`.

Сейчас обработка задачи имитируется задержкой и случайным успехом или ошибкой. Состояния хранятся в памяти, результаты выполнения выводятся в лог. Подробности — в [README очереди](Golang_queue_with_workers/README.md).

Все три Go-сервиса используют порт `8080`: запускайте их по очереди. Остановка — `Ctrl+C`.

### AviWx — погода и маршрут

Консольный отчёт по демонстрационному сценарию:

```sh
(cd weather_prog && make run)
```

Сборка GUI и запуск с картой масштаба 1:1 000 000:

```sh
(
  cd weather_prog
  make app
  ./build/make/aviwx_app --map-scale 1m examples/demo_scenario.json
)
```

Оценка риска рассчитывается по маршруту, погодным опасностям и условиям аэродромов из JSON-сценария. Прогноз Open-Meteo загружается отдельно для отображения на карте и временной шкале; он пока не пересчитывает оценку риска из сценария.

Консольное ядро собирается без Qt6 и GDAL. Для GUI требуется Qt6, для SXF-карт — дополнительно GDAL. Карты находятся в `Maps/` и `Maps_1M/`, пример входных данных — в [demo_scenario.json](weather_prog/examples/demo_scenario.json).

Дополнительные параметры сборки, выбор карт и экспорт скриншота описаны в [README AviWx](weather_prog/README.md).

### 3D Cube

```sh
make -C 3D_Cube run
```

| Клавиши | Действие |
| --- | --- |
| `C` | Включить или выключить цвета граней |
| `V` | Переключить автоматическое и ручное вращение |
| `WASD` / стрелки | Вращать куб в ручном режиме |
| `Ctrl+C` | Завершить анимацию |

Подробности — в [README 3D Cube](3D_Cube/README.md).

## Сборка библиотек и игр

### Библиотеки C/C++

Для сборки каждой библиотеки отдельно:

```sh
make -C C2_s21_stringplus-1-develop/src s21_string.a
make -C C5_s21_decimal.ID_353529-Team_TL_tobymayr.572ea178_7cff_4323-3-develop/src s21_decimal.a
make -C C6_s21_matrix.ID_353530-1-develop/src s21_matrix.a
make -C CPP1_s21_matrixplus.ID_353533-1-develop/src s21_matrix_oop.a
```

Архивы `.a` создаются в соответствующих каталогах `src/`. Публичные функции и класс матрицы объявлены в заголовках рядом с исходниками.

### cat и grep

```sh
make -C C3_SimpleBashUtils-2-develop/src/cat
make -C C3_SimpleBashUtils-2-develop/src/grep

./C3_SimpleBashUtils-2-develop/src/cat/s21_cat -n README.md
./C3_SimpleBashUtils-2-develop/src/grep/s21_grep -n Go README.md
```

В `cat` реализованы флаги `-b`, `-e`, `-n`, `-s`, `-t`, `-v`, `-E`, `-T`; в `grep` — отдельные флаги `-e`, `-i`, `-v`, `-c`, `-l`, `-n`.

### Tetris

```sh
(
  cd C7_BrickGame_v1.0.ID_1449897-1-develop/src
  make
  cd build
  ./Tetris
)
```

Управление: `s` — начать игру, стрелки влево/вправо — перемещение, вниз — опускание, вверх — поворот, `p` — пауза. Остальные сведения находятся в [документации игры](C7_BrickGame_v1.0.ID_1449897-1-develop/src/Documentation.html).

### Pong, Game of Life и Graph

Примеры прямой сборки компилятором; исполняемые файлы помещаются в `build/` в корне репозитория:

```sh
mkdir -p build

cc -std=c11 P01D06-1-develop/src/pong.c -o build/pong
cc -std=gnu11 P01D06-1-develop/src/pong_interactive.c -lncurses -o build/pong_interactive
cc -std=gnu11 P02D13-1-develop/src/game_of_life.c -lncurses -o build/game_of_life
cc -std=gnu11 P03D20-1-develop/src/main.c P03D20-1-develop/src/graph.c -lm -o build/graph
```

Запускайте нужную программу отдельно:

```sh
./build/pong
./build/pong_interactive
./build/game_of_life < P02D13-1-develop/src/start-fields/pulsar.txt
./build/graph
```

В Pong игроки используют `A/Z` и `K/M`. В Game of Life клавиши `+` и `-` меняют скорость, `q` завершает игру. Graph запрашивает выражение, например `sin(x)`; унарный минус задаётся символом `~`.

## Тестирование

| Проекты | Где находятся проверки | Запуск |
| --- | --- | --- |
| Map Server | `main_test.go`, пакет `testing`, временные SQLite-базы | `go test ./...` из `server_golang/` |
| s21_string, s21_decimal, s21_matrix | `src/tests/`, библиотека Check | `make test` из соответствующего `src/` |
| s21_matrixplus | `src/tests/`, GoogleTest | `make test` из `src/` |
| Tetris | `src/test/`, библиотека Check | `make test` из `src/` |
| cat и grep | Shell-скрипты в `src/cat/tests/` и `src/grep/tests/` | Из папки `tests/`, предварительно поместив в неё собранный `s21_cat` или `s21_grep` |
| AviWx | `tests/risk_engine_tests.cpp` | Команда ниже |

Тесты AviWx используют `assert`, поэтому для них выбирается Debug-сборка:

```sh
make -C weather_prog test BUILD_DIR=build/debug BUILD_TYPE=Debug
```

В Makefile библиотек и Тетриса предусмотрена цель `gcov_report`. Она требует дополнительных инструментов покрытия; состав отчёта зависит от конкретного проекта. Цели форматирования также различаются: часть из них меняет исходные файлы.

У Weather Server, очереди задач, 3D Cube, Pong, Game of Life и Graph отдельных автоматических тестов в текущем дереве нет.

В `benchmark_test.go` Map Server есть последовательные и параллельные HTTP-бенчмарки. Они требуют реальные MBTiles-базы и запускаются командой:

```sh
(cd server_golang && go test -bench=. -benchmem)
```

---

Состав проектов и команды сверены с исходниками и конфигурацией сборки 26 октября 2026 года.
