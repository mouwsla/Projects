# Go Task Queue: Redis + RabbitMQ

Небольшой асинхронный сервис обработки задач на Go. HTTP API принимает задачу, сохраняет её состояние в Redis и публикует сообщение в RabbitMQ. Worker обрабатывает очередь, повторяет неудачные задачи с задержкой и отправляет окончательно упавшие сообщения в dead-letter queue.

## Что входит в проект

- `api` — HTTP-сервис на порту `8080` внутри Docker-сети;
- `worker` — потребитель RabbitMQ с ручными подтверждениями сообщений;
- `redis` — хранение статусов задач с TTL 24 часа;
- `rabbitmq` — основная очередь, retry-очередь с задержкой 5 секунд и DLQ;
- `caddy` — reverse proxy, публикующий API на `http://localhost`;
- `task_test.py` и `watch_task.py` — тестовые сценарии без внешних Python-зависимостей.

```text
клиент → Caddy → API → RabbitMQ → Worker
                  ↓                 ↓
                  └──── Redis ──────┘
```

## Требования

- Docker Engine с Docker Compose v2;
- для локальной сборки без Docker — Go `1.27.3`;
- Python 3 нужен только для тестовых скриптов.

## Быстрый старт

```bash
git clone <URL-репозитория>
cd server_go_redis_rabbit
cp .env.example .env
docker compose up --build
```

После запуска доступны:

- API: `http://localhost`;
- health check: `http://localhost/health`;
- RabbitMQ Management UI: `http://localhost:15672`.

Логин и пароль RabbitMQ задаются в `.env`. Локальный `.env` игнорируется Git; в репозитории хранится только `.env.example`. Перед публикацией или общим развёртыванием замените демонстрационный пароль `change_me`.

Остановка сервисов:

```bash
docker compose down
```

Чтобы вместе с контейнерами удалить локальные тома Redis, RabbitMQ и Caddy:

```bash
docker compose down -v
```

## API

### Проверка состояния

```bash
curl http://localhost/health
```

Пример ответа:

```json
{"status":"ok","redis":"ok","rabbitmq":"ok"}
```

### Счётчик обращений

```bash
curl http://localhost/visits
```

### Создание задачи

Поддерживаются два демонстрационных типа:

- `email` — успешно завершается примерно через 2 секунды;
- `fail` — имитирует ошибку, проходит повторные попытки и попадает в DLQ.

```bash
curl -sS \
  -H 'Content-Type: application/json' \
  -d '{"type":"email","email":"user@example.com"}' \
  http://localhost/tasks
```

API отвечает кодом `202 Accepted`:

```json
{"id":"<task-id>","status":"queued","queue":"tasks"}
```

### Получение статуса

```bash
curl http://localhost/tasks/<task-id>
```

Возможные статусы: `queued`, `processing`, `retrying`, `completed`, `failed` и `publish_unknown`.

Создать задачу и наблюдать за ней одной командой:

```bash
curl -sS \
  -H 'Content-Type: application/json' \
  -d '{"type":"email","email":"user@example.com"}' \
  http://localhost/tasks | python3 watch_task.py
```

## Нагрузочный и отказоустойчивый сценарии

Одна успешная задача:

```bash
python3 task_test.py
```

Сто задач, до двадцати параллельных HTTP-запросов:

```bash
python3 task_test.py --count 100 --parallel 20
```

Проверка retry и DLQ:

```bash
python3 task_test.py --type fail --timeout 60
```

Просмотреть очередь `tasks.dead` можно в RabbitMQ Management UI.

## Проверка исходного кода

```bash
go test ./...
go vet ./...
docker compose config --quiet
```

## Конфигурация

| Переменная | Где используется | Значение в Docker Compose |
|---|---|---|
| `RABBITMQ_USER` | учётная запись RabbitMQ | берётся из `.env` |
| `RABBITMQ_PASSWORD` | пароль RabbitMQ | берётся из `.env` |
| `RABBITMQ_URL` | подключение API и worker | формируется в `compose.yaml` |
| `REDIS_ADDR` | подключение API и worker | `redis:6379` |

## Структура проекта

```text
server_go_redis_rabbit/
├── cmd/api/             # HTTP API и публикация задач
├── cmd/worker/          # consumer, retry и DLQ
├── internal/taskstate/  # атомарные операции со статусами и lease в Redis
├── compose.yaml
├── Dockerfile
├── Caddyfile
├── .env.example
├── task_test.py
└── watch_task.py
```

Сборочные результаты, локальные данные Docker, логи, профили, Python-кэши, настройки IDE и секреты исключены через `.gitignore` и `.dockerignore`.
