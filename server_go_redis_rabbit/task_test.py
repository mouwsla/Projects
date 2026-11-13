
import argparse
import json
import time
import urllib.request

from collections import Counter
from concurrent.futures import ThreadPoolExecutor

BASE_URL = "http://localhost"


def request(path, payload=None):
    data = None

    if payload is not None:
        data = json.dumps(payload).encode("utf-8")

    req = urllib.request.Request(
        BASE_URL + path,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST" if payload is not None else "GET",
    )

    with urllib.request.urlopen(req, timeout=10) as response:
        return json.load(response)


def create_task(index, task_type):
    return request("/tasks", {
        "type": task_type,
        "email": f"test{index}@example.com",
    })


def get_status(task_id):
    response = request(f"/tasks/{task_id}")
    return response["status"]


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--type",
        choices=["email", "fail"],
        default="email",
    )
    parser.add_argument("--count", type=int, default=1)
    parser.add_argument("--parallel", type=int, default=10)
    parser.add_argument("--interval", type=float, default=0.5)
    parser.add_argument("--timeout", type=float, default=180)

    args = parser.parse_args()

    if args.count < 1 or args.parallel < 1 or args.interval <= 0:
        parser.error("count, parallel и interval должны быть > 0")

    start = time.monotonic()

    print(f"Создаём задач: {args.count}")
    print(f"Тип: {args.type}")
    print(f"Параллельных HTTP-запросов: {args.parallel}\n")

    with ThreadPoolExecutor(
        max_workers=args.parallel,
    ) as executor:

        created = list(executor.map(
            lambda i: create_task(i, args.type),
            range(args.count),
        ))

        ids = [task["id"] for task in created]

        print(f"API принял {len(ids)} задач")
        print(f"Время отправки: {time.monotonic() - start:.2f} с\n")

        previous = {}
        last_summary = None
        states = {}

        if args.count == 1:
            previous[ids[0]] = created[0]["status"]
            print(
                f"{time.strftime('%H:%M:%S')} "
                f"{ids[0][:12]} {created[0]['status']}"
            )

        while time.monotonic() - start < args.timeout:

            results = list(executor.map(get_status, ids))
            states = dict(zip(ids, results))

            if args.count == 1:
                task_id = ids[0]
                state = states[task_id]

                if state != previous.get(task_id):
                    print(
                        f"{time.strftime('%H:%M:%S')} "
                        f"{task_id[:12]} {state}"
                    )
                    previous[task_id] = state

            else:
                counts = Counter(states.values())

                summary = tuple(
                    counts.get(s, 0)
                    for s in (
                        "queued",
                        "processing",
                        "retrying",
                        "completed",
                        "failed",
                    )
                )

                if summary != last_summary:
                    elapsed = time.monotonic() - start

                    print(
                        f"[{elapsed:6.1f}s] "
                        f"queued={summary[0]} "
                        f"processing={summary[1]} "
                        f"retrying={summary[2]} "
                        f"completed={summary[3]} "
                        f"failed={summary[4]}"
                    )

                    last_summary = summary

            if all(
                s in ("completed", "failed")
                for s in states.values()
            ):
                break

            time.sleep(args.interval)

    elapsed = time.monotonic() - start
    counts = Counter(states.values())

    print("\n--- Результат ---")
    print(f"Общее время: {elapsed:.2f} с")
    print(f"Completed: {counts['completed']}")
    print(f"Failed: {counts['failed']}")

    finished = counts["completed"] + counts["failed"]

    print(f"Завершено: {finished}/{len(ids)}")
    print(f"Скорость: {finished / elapsed:.2f} задач/с")

    if finished != len(ids):
        print("Не все задачи достигли финального статуса.")


if __name__ == "__main__":
    main()
