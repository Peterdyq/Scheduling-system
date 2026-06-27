# Scheduling System

A small course-scheduling demo that combines a Flask backend, a browser UI, and a C++ scheduling executable.

## Requirements

- Python 3
- Flask
- A C++ compiler such as `g++`
- Windows can use the committed `output/schedule.exe` directly

## Run

Start the Flask server from the project root:

```powershell
venv\Scripts\python.exe connector.py
```

Open:

```text
http://127.0.0.1:5000/
```

Live Server is no longer required. The Flask server serves the HTML, JavaScript, CSS, and API from the same port.

## Input Rules

Each course must include:

- `name`: non-empty course name, max 50 characters
- `num`: non-empty course number, max 30 characters
- `day`: integer from `1` to `7`
- `starttime`: strict `HH:MM`, for example `08:00`
- `endtime`: strict `HH:MM`, and later than `starttime`

The frontend validates these rules before adding a course. The backend validates them again before calling C++.

## Rebuild C++

Windows:

```powershell
g++ schedule.cpp -std=c++17 -O2 -o output\schedule.exe
```

macOS/Linux:

```bash
g++ schedule.cpp -std=c++17 -O2 -o output/schedule
```

The backend looks for `output/schedule.exe` on Windows and `output/schedule` on other systems. If the executable is missing, the API response includes the compile command.

## Output

The C++ program returns structured JSON. The backend forwards it as:

```json
{
  "status": "success",
  "message": "排课完成",
  "schedule": {
    "has_conflict": false,
    "courses": [],
    "conflicts": []
  }
}
```

The browser renders `courses` as a schedule table and `conflicts` as conflict details.

## Local Browser Cache

The page stores the current course list and last scheduling result in `localStorage`, so accidental refreshes do not lose data. Use `clear courses` and `clear output` to clear saved browser data.
