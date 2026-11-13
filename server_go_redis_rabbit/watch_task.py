
import json
import sys
import time
import urllib.request
from datetime import datetime

API = "http://localhost"

# Получаем JSON, который вернул curl.
response = sys.stdin.read()

print(response, flush=True)

try:
    task_id = json.loads(response)["id"]
except (ValueError, KeyError):
    print("Не удалось получить ID задачи")
    sys.exit(1)

print("\nОтслеживание задачи:\n")

previous_status = None

while True:
    try:
        with urllib.request.urlopen(
            f"{API}/tasks/{task_id}", timeout=5
        ) as res:
            data = json.load(res)

        status = data["status"]

        if status != previous_status:
            current_time = datetime.now().strftime("%H:%M:%S")

            print(
                f"[{current_time}] {status}",
                flush=True,
            )

            previous_status = status

        if status in ("completed", "failed"):
            print("\nОбработка завершена.")
            break

        time.sleep(0.3)

    except KeyboardInterrupt:
        print("\nОтслеживание остановлено.")
        break

    except Exception as error:
        print(f"Ошибка: {error}")
        time.sleep(1)
