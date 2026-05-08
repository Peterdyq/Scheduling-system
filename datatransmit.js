async function uploadData() {
    if (event) event.preventDefault();
    const payload = JSON.stringify(datalist);
    const outputBox = document.getElementById('outputContent');

    outputBox.innerText = "正在排课，请稍后...";
    outputBox.style.color = "blue";

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
            outputBox.innerText = "后端返回的不是 JSON:\n" + rawText;
            outputBox.style.color = "red";
            return;
        }

        if (response.ok) {
            outputBox.innerText = result.cpp_output || "C++ 程序无输出";
            outputBox.style.color = "black";
        } else {
            outputBox.innerText = "服务器错误：" + (result.message || rawText);
            outputBox.style.color = "red";
        }
    } catch (error) {
        outputBox.innerText = "请求失败：" + error.message;
        outputBox.style.color = "red";
    }
}