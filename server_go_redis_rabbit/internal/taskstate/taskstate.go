package taskstate

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"errors"
	"fmt"
	"net/mail"
	"os"
	"time"

	"github.com/redis/go-redis/v9"
)

var unknownScript = redis.NewScript(`
local state = redis.call('GET', KEYS[1])
if state == 'queued' then
    redis.call('SET', KEYS[1],
        'publish_unknown', 'EX', ARGV[1])
    return 'publish_unknown'
end
return state or 'missing'
`)

var claimScript = redis.NewScript(`
local state = redis.call('GET', KEYS[1])

if not state then return -1 end

if state == 'completed' or state == 'failed' then
    return 0
end

if not redis.call('SET', KEYS[2],
    ARGV[1], 'NX', 'PX', ARGV[2]) then
    return 2
end

redis.call('SET', KEYS[1],
    'processing', 'EX', ARGV[3])

return 1
`)

// Статус меняет только владелец блокировки.
var finishScript = redis.NewScript(`
if redis.call('GET', KEYS[2]) ~= ARGV[1] then
    return 0
end

local state = redis.call('GET', KEYS[1])

if state == 'completed' or state == 'failed' then
    redis.call('DEL', KEYS[2])
    return 0
end

redis.call('SET', KEYS[1],
    ARGV[2], 'EX', ARGV[3])

redis.call('DEL', KEYS[2])
return 1
`)

// Удаляем только собственную блокировку.
var releaseScript = redis.NewScript(`
if redis.call('GET', KEYS[1]) == ARGV[1] then
    return redis.call('DEL', KEYS[1])
end
return 0
`)

const (
	StatusTTL = 24 * time.Hour
	LeaseTTL  = 24 * time.Second
)

func Key(id string) string {
	return "task:" + id
}

func lockKey(id string) string {
	return "task:lock:" + id
}

func OpenRedis() (
	*redis.Client,
	string,
	error,
) {
	redisAddress := os.Getenv("REDIS_ADDR")
	if redisAddress == "" {
		redisAddress = "localhost:6379"
	}

	redisClient := redis.NewClient(&redis.Options{
		Addr: redisAddress,
	})

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := redisClient.Ping(ctx).Err(); err != nil {
		_ = redisClient.Close()

		return nil, "", fmt.Errorf(
			"failed to connect to Redis: %w",
			err,
		)
	}
	return redisClient, redisAddress, nil

}

func NewID() (string, error) {
	buf := make([]byte, 16)
	if _, err := rand.Read(buf); err != nil {
		return "", err
	}
	return hex.EncodeToString(buf), nil
}

func Validate(kind, email string) error {
	if kind != "email" && kind != "fail" {
		return errors.New("type must be email or fail")
	}
	if len(email) == 0 || len(email) > 254 {
		return errors.New("invalid email length")
	}
	addr, err := mail.ParseAddress(email)
	if addr.Address != email || err != nil {
		return errors.New("invalid email address")
	}
	return nil
}

func Create(
	ctx context.Context,
	client *redis.Client,
	id string,
) error {
	ok, err := client.SetNX(
		ctx,
		Key(id),
		"queued",
		StatusTTL,
	).Result()
	if err != nil {
		return err
	}
	if !ok {
		return errors.New("task ID already exists")
	}
	return nil
}

func MarkPubIsUnknow(
	ctx context.Context,
	client *redis.Client,
	id string,
) (string, error) {

	return unknownScript.Run(
		ctx,
		client,
		[]string{Key(id)},
		int64(StatusTTL.Seconds()),
	).Text()
}

func Claim(
	ctx context.Context,
	client *redis.Client,
	id, owner string,
) (int64, error) {
	return claimScript.Run(
		ctx,
		client,
		[]string{Key(id), lockKey(id)},
		owner,
		int64(LeaseTTL.Milliseconds()),
		int64(StatusTTL.Seconds()),
	).Int64()
}

func Finish(
	ctx context.Context,
	client *redis.Client,
	id, owner, status string,
) error {
	if status != "completed" &&
		status != "retrying" &&
		status != "failed" {
		return fmt.Errorf("invalid status: %s", status)
	}
	result, err := finishScript.Run(
		ctx,
		client,
		[]string{Key(id), lockKey(id)},
		owner,
		status,
		int64(StatusTTL.Seconds()),
	).Int64()
	if err != nil {
		return err
	}
	if result != 1 {
		return errors.New("task lease expired or lost")
	}
	return nil
}

func Release(
	ctx context.Context,
	client *redis.Client,
	id, owner string,
) error {
	return releaseScript.Run(
		ctx,
		client,
		[]string{lockKey(id)},
		owner,
	).Err()
}
