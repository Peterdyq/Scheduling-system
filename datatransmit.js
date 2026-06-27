const SCHEDULE_OUTPUT_STORAGE_KEY = 'scheduling_system_last_output';

function setOutput(outputBox, text, color) {
    outputBox.innerText = text;
    outputBox.style.color = color;
    localStorage.setItem(SCHEDULE_OUTPUT_STORAGE_KEY, JSON.stringify({ text, color }));
}

function restoreOutput() {
    const outputBox = document.getElementById('outputContent');
    if (!outputBox) {
        return;
    }

    try {
        const savedOutput = localStorage.getItem(SCHEDULE_OUTPUT_STORAGE_KEY);
        if (!savedOutput) {
            return;
        }

        const { text, color } = JSON.parse(savedOutput);
        outputBox.innerText = text;
        outputBox.style.color = color || 'black';
    } catch (error) {
        console.warn('读取已保存排课反馈失败：', error);
    }
}

async function uploadData(event) {
    if (event && typeof event.preventDefault === 'function') {
        event.preventDefault();
    }

    const payload = JSON.stringify(datalist);
    const outputBox = document.getElementById('outputContent');
    const submitBtn = document.getElementById('submitBtn');

    outputBox.innerText = "正在排课，请稍后...";
    outputBox.style.color = "blue";
    if (submitBtn) {
        submitBtn.disabled = true;
    }

    try {
        const response = await fetch('http://127.0.0.1:5000/api/make_schedule', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: payload
        });

        const rawText = await response.text();
        console.log("服务器原始返回：", rawText);

        let result;
        try {
            result = JSON.parse(rawText);
        } catch (e) {
            setOutput(outputBox, "后端返回的不是 JSON:\n" + rawText, "red");
            return;
        }

        if (response.ok) {
            setOutput(outputBox, result.cpp_output || "C++ 程序无输出", "black");
        } else {
            setOutput(outputBox, "服务器错误：" + (result.message || rawText), "red");
        }
    } catch (error) {
        setOutput(outputBox, "请求失败：" + error.message, "red");
    } finally {
        if (submitBtn) {
            submitBtn.disabled = false;
        }
    }
}

restoreOutput();
