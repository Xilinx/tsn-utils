import json
import time
from datetime import timedelta

def load_json(file_path):
    timeout = timedelta(seconds=1)
    sleep_time = timedelta(milliseconds=1)
    time_elapsed = timedelta(milliseconds=0)
    while time_elapsed < timeout:
        try:
            with open(file_path) as data_file:
                data = json.load(data_file)
        except json.decoder.JSONDecodeError:
            time.sleep(sleep_time.total_seconds())
            time_elapsed += sleep_time
        else:
            return data
    print(f"Time out while loading json data from '{file_path}'")
    return None
