from flask import Flask, request, jsonify
import json
import os
import subprocess
import tempfile

app = Flask(__name__)


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "POST, OPTIONS"
    return response

# 定义一个路由，专门用来接收前端传来的排课数据
# methods=['POST'] 表示这个接口只接受“提交数据”的操作
@app.route('/api/make_schedule', methods=['POST'])
def make_schedule():
    # --- 1. 接收数据 ---
    # 从网页请求中提取 JSON 数据，并将其转换为 Python 的列表或字典
    # 假设网页传过来的是一个包含课程对象的数组
    incoming_data = request.get_json() 

    # 如果没收到数据，直接返回错误信息给网页
    if not incoming_data:
        return jsonify({"status": "error", "message": "请求体为空"}), 400

    # --- 2. 整合数据 ---
    # 我们可以给原始数据套上一层“信封”，方便 C++ 读取更多元信息
    processed_payload = {
        "config": {
            "version": "2.0",
            "author": "User"
        },
        "course_list": incoming_data  # 网页传来的核心课程数据放在这里
    }

    # --- 3. 存为临时文件 ---
    # 不写入项目目录，避免 VS Code Live Server 检测到文件变化后自动刷新页面。
    input_filename = os.path.join(tempfile.gettempdir(), 'scheduling_system_data_to_cpp.json')
    
    try:
        # 使用 utf-8 编码写入，确保课程名中的中文不会乱码
        with open(input_filename, 'w', encoding='utf-8') as f:
            # indent=4 让生成的 JSON 文件有缩进，方便你打开文件肉眼检查
            json.dump(processed_payload, f, ensure_ascii=False, indent=4)
        
        # --- 4. 调用 C++ 程序 ---
        # 假设你的 C++ 编译后叫 schedule.exe
        # 我们把文件名作为命令行参数传给它：schedule.exe data_to_cpp.json
        cpp_executable = os.path.join(".", "output", "schedule.exe")
        
        if not os.path.exists(cpp_executable):
            return jsonify({"status": "error", "message": "找不到 C++ 可执行文件"}), 500

        # subprocess.run 会阻塞运行，直到 C++ 程序执行完毕
        # capture_output=True 会抓取 C++ 里的 cout 打印的内容
        result = subprocess.run(
            [cpp_executable, input_filename], 
            capture_output=True, 
            text=True, 
            encoding='utf-8'
        )

        

        # --- 5. 返回结果 ---
        # result.stdout 就是 C++ 程序在黑窗口打印出来的所有文字
        print("C++ Output:", result.stdout)
        return jsonify({
            "status": "success",
            "message": "排课完成",
            "cpp_output": result.stdout  # 把 C++ 的计算结果直接传回给网页
        })

    except Exception as e:
        # 如果中间任何一步报错（比如文件写失败），捕获异常并返回
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    # 启动 Flask 服务
    app.run(debug=True, port=5000, use_reloader=False)
