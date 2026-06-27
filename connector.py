from flask import Flask, jsonify, request, send_from_directory
import json
import os
import re
import subprocess
import tempfile

app = Flask(__name__)

TIME_PATTERN = re.compile(r"^([01]\d|2[0-3]):([0-5]\d)$")
REQUIRED_COURSE_FIELDS = ("name", "num", "day", "starttime", "endtime")


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "POST, OPTIONS"
    return response


@app.route("/")
def index():
    return send_from_directory(".", "form_submission.html")


@app.route("/addcourse.js")
def addcourse_js():
    return send_from_directory(".", "addcourse.js")


@app.route("/datatransmit.js")
def datatransmit_js():
    return send_from_directory(".", "datatransmit.js")


@app.route("/style.css")
def style_css():
    return send_from_directory(".", "style.css")


def get_cpp_executable():
    filename = "schedule.exe" if os.name == "nt" else "schedule"
    return os.path.join(".", "output", filename)


def get_compile_command():
    output_path = "output\\schedule.exe" if os.name == "nt" else "output/schedule"
    return f"g++ schedule.cpp -std=c++17 -O2 -o {output_path}"


def parse_time_to_minutes(value):
    if not isinstance(value, str):
        return None

    match = TIME_PATTERN.fullmatch(value.strip())
    if not match:
        return None

    hours = int(match.group(1))
    minutes = int(match.group(2))
    return hours * 60 + minutes


def validate_courses(data):
    if data is None:
        return None, "请求体必须是合法的 JSON"

    if not isinstance(data, list):
        return None, "请求体最外层必须是课程数组"

    if len(data) == 0:
        return None, "课程列表不能为空"

    validated_courses = []
    for index, course in enumerate(data, start=1):
        if not isinstance(course, dict):
            return None, f"第 {index} 门课程必须是对象"

        for field in REQUIRED_COURSE_FIELDS:
            if field not in course:
                return None, f"第 {index} 门课程缺少 {field}"

        name = course["name"]
        if not isinstance(name, str) or not name.strip():
            return None, f"第 {index} 门课程的 name 不能为空"
        name = name.strip()
        if len(name) > 50:
            return None, f"第 {index} 门课程的 name 不能超过 50 个字符"

        course_num = course["num"]
        if isinstance(course_num, bool) or course_num is None:
            return None, f"第 {index} 门课程的 num 不能为空"
        if not isinstance(course_num, (str, int, float)):
            return None, f"第 {index} 门课程的 num 必须是字符串或数字"
        course_num = str(course_num).strip()
        if not course_num:
            return None, f"第 {index} 门课程的 num 不能为空"
        if len(course_num) > 30:
            return None, f"第 {index} 门课程的 num 不能超过 30 个字符"

        day = course["day"]
        if isinstance(day, bool) or not isinstance(day, int):
            return None, f"第 {index} 门课程的 day 必须是 1-7 的整数"
        if day < 1 or day > 7:
            return None, f"第 {index} 门课程的 day 必须在 1-7 之间"

        starttime = course["starttime"]
        endtime = course["endtime"]
        start_minutes = parse_time_to_minutes(starttime)
        end_minutes = parse_time_to_minutes(endtime)
        if start_minutes is None:
            return None, f"第 {index} 门课程的 starttime 必须是 HH:MM 格式"
        if end_minutes is None:
            return None, f"第 {index} 门课程的 endtime 必须是 HH:MM 格式"
        if start_minutes >= end_minutes:
            return None, f"第 {index} 门课程的开始时间必须早于结束时间"

        validated_courses.append({
            "name": name,
            "num": course_num,
            "day": day,
            "starttime": starttime.strip(),
            "endtime": endtime.strip(),
        })

    return validated_courses, None


@app.route("/api/make_schedule", methods=["POST"])
def make_schedule():
    incoming_data = request.get_json(silent=True)
    validated_courses, validation_error = validate_courses(incoming_data)
    if validation_error:
        return jsonify({"status": "error", "message": validation_error}), 400

    processed_payload = {
        "config": {
            "version": "2.0",
            "author": "User",
        },
        "course_list": validated_courses,
    }

    input_filename = None

    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            suffix=".json",
            prefix="scheduling_system_",
            delete=False,
        ) as f:
            input_filename = f.name
            json.dump(processed_payload, f, ensure_ascii=False, indent=4)

        cpp_executable = get_cpp_executable()
        if not os.path.exists(cpp_executable):
            return jsonify({
                "status": "error",
                "message": "找不到 C++ 可执行文件",
                "compile_command": get_compile_command(),
            }), 500

        result = subprocess.run(
            [cpp_executable, input_filename],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        if result.returncode != 0:
            return jsonify({
                "status": "error",
                "message": "C++ 程序执行失败",
                "cpp_stdout": result.stdout,
                "cpp_stderr": result.stderr,
                "returncode": result.returncode,
            }), 500

        try:
            schedule_result = json.loads(result.stdout)
        except json.JSONDecodeError:
            return jsonify({
                "status": "error",
                "message": "C++ 程序返回的不是合法 JSON",
                "cpp_stdout": result.stdout,
            }), 500

        return jsonify({
            "status": "success",
            "message": "排课完成",
            "schedule": schedule_result,
        })

    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

    finally:
        if input_filename and os.path.exists(input_filename):
            os.remove(input_filename)


if __name__ == "__main__":
    app.run(debug=True, port=5000, use_reloader=False)
