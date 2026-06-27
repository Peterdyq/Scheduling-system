from flask import Flask, jsonify, request, send_from_directory
from sqlalchemy import select
from sqlalchemy.exc import SQLAlchemyError
import json
import os
import re
import subprocess
import tempfile

from db import (
    Course,
    SchedulePlan,
    get_session,
    is_database_configured,
    plan_to_detail,
    plan_to_summary,
)

app = Flask(__name__)

TIME_PATTERN = re.compile(r"^([01]\d|2[0-3]):([0-5]\d)$")
REQUIRED_COURSE_FIELDS = ("name", "num", "day", "starttime", "endtime")


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS"
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


def validate_courses(data, allow_empty=False):
    if data is None:
        return None, "请求体必须是合法的 JSON"

    if not isinstance(data, list):
        return None, "请求体最外层必须是课程数组"

    if not allow_empty and len(data) == 0:
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


def validate_plan_payload(payload, require_name=False):
    if payload is None or not isinstance(payload, dict):
        return None, "请求体必须是对象"

    name = payload.get("name", "")
    if require_name and (not isinstance(name, str) or not name.strip()):
        return None, "方案名称不能为空"
    if name and (not isinstance(name, str) or len(name.strip()) > 120):
        return None, "方案名称必须是 120 个字符以内的字符串"

    description = payload.get("description", "")
    if description is None:
        description = ""
    if not isinstance(description, str):
        return None, "方案说明必须是字符串"

    courses = payload.get("courses")
    validated_courses = None
    if courses is not None:
        validated_courses, error = validate_courses(courses, allow_empty=True)
        if error:
            return None, error

    return {
        "name": name.strip() if isinstance(name, str) else "",
        "description": description.strip(),
        "courses": validated_courses,
    }, None


def run_scheduler(validated_courses):
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
            return None, ({
                "status": "error",
                "message": "找不到 C++ 可执行文件",
                "compile_command": get_compile_command(),
            }, 500)

        result = subprocess.run(
            [cpp_executable, input_filename],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        if result.returncode != 0:
            return None, ({
                "status": "error",
                "message": "C++ 程序执行失败",
                "cpp_stdout": result.stdout,
                "cpp_stderr": result.stderr,
                "returncode": result.returncode,
            }, 500)

        try:
            return json.loads(result.stdout), None
        except json.JSONDecodeError:
            return None, ({
                "status": "error",
                "message": "C++ 程序返回的不是合法 JSON",
                "cpp_stdout": result.stdout,
            }, 500)

    finally:
        if input_filename and os.path.exists(input_filename):
            os.remove(input_filename)


def database_error_response(error):
    return jsonify({
        "status": "error",
        "message": "数据库操作失败",
        "detail": str(error),
    }), 500


@app.route("/api/db/status", methods=["GET"])
def db_status():
    if not is_database_configured():
        return jsonify({
            "status": "not_configured",
            "message": "DATABASE_URL 未配置",
        }), 503

    try:
        with get_session() as session:
            session.execute(select(SchedulePlan.id).limit(1))
        return jsonify({"status": "ok"})
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans", methods=["GET"])
def list_plans():
    try:
        with get_session() as session:
            plans = session.scalars(select(SchedulePlan).order_by(SchedulePlan.updated_at.desc())).all()
            return jsonify({"status": "success", "plans": [plan_to_summary(plan) for plan in plans]})
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans", methods=["POST"])
def create_plan():
    payload = request.get_json(silent=True)
    data, error = validate_plan_payload(payload, require_name=True)
    if error:
        return jsonify({"status": "error", "message": error}), 400

    try:
        with get_session() as session:
            plan = SchedulePlan(name=data["name"], description=data["description"])
            for course in data["courses"] or []:
                plan.courses.append(Course(**course))
            session.add(plan)
            session.commit()
            return jsonify({"status": "success", "plan": plan_to_detail(plan)}), 201
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans/<int:plan_id>", methods=["GET"])
def get_plan(plan_id):
    try:
        with get_session() as session:
            plan = session.get(SchedulePlan, plan_id)
            if not plan:
                return jsonify({"status": "error", "message": "找不到课程表方案"}), 404
            return jsonify({"status": "success", "plan": plan_to_detail(plan)})
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans/<int:plan_id>", methods=["PUT"])
def update_plan(plan_id):
    payload = request.get_json(silent=True)
    data, error = validate_plan_payload(payload, require_name=False)
    if error:
        return jsonify({"status": "error", "message": error}), 400

    try:
        with get_session() as session:
            plan = session.get(SchedulePlan, plan_id)
            if not plan:
                return jsonify({"status": "error", "message": "找不到课程表方案"}), 404

            if data["name"]:
                plan.name = data["name"]
            plan.description = data["description"]

            if data["courses"] is not None:
                plan.courses.clear()
                for course in data["courses"]:
                    plan.courses.append(Course(**course))

            session.commit()
            return jsonify({"status": "success", "plan": plan_to_detail(plan)})
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans/<int:plan_id>", methods=["DELETE"])
def delete_plan(plan_id):
    try:
        with get_session() as session:
            plan = session.get(SchedulePlan, plan_id)
            if not plan:
                return jsonify({"status": "error", "message": "找不到课程表方案"}), 404
            session.delete(plan)
            session.commit()
            return jsonify({"status": "success"})
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)


@app.route("/api/plans/<int:plan_id>/run", methods=["POST"])
def run_plan(plan_id):
    try:
        with get_session() as session:
            plan = session.get(SchedulePlan, plan_id)
            if not plan:
                return jsonify({"status": "error", "message": "找不到课程表方案"}), 404

            courses = [
                {
                    "name": course.name,
                    "num": course.num,
                    "day": course.day,
                    "starttime": course.starttime,
                    "endtime": course.endtime,
                }
                for course in plan.courses
            ]
    except RuntimeError as error:
        return jsonify({"status": "error", "message": str(error)}), 503
    except SQLAlchemyError as error:
        return database_error_response(error)

    if not courses:
        return jsonify({"status": "error", "message": "课程列表不能为空"}), 400

    schedule_result, run_error = run_scheduler(courses)
    if run_error:
        body, status_code = run_error
        return jsonify(body), status_code

    return jsonify({
        "status": "success",
        "message": "排课完成",
        "schedule": schedule_result,
    })


@app.route("/api/make_schedule", methods=["POST"])
def make_schedule():
    incoming_data = request.get_json(silent=True)
    validated_courses, validation_error = validate_courses(incoming_data)
    if validation_error:
        return jsonify({"status": "error", "message": validation_error}), 400

    schedule_result, run_error = run_scheduler(validated_courses)
    if run_error:
        body, status_code = run_error
        return jsonify(body), status_code

    return jsonify({
        "status": "success",
        "message": "排课完成",
        "schedule": schedule_result,
    })


if __name__ == "__main__":
    app.run(debug=True, port=5000, use_reloader=False)
