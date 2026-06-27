const SCHEDULE_OUTPUT_STORAGE_KEY = 'scheduling_system_last_output';
const DEFAULT_OUTPUT_TEXT = '\u7b49\u5f85\u63d0\u4ea4\u6570\u636e...';
const API_BASE_URL = window.location.port === '5000' ? '' : 'http://127.0.0.1:5000';

function escapeOutputHtml(value) {
    return String(value)
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#039;');
}

function saveOutput(payload) {
    localStorage.setItem(SCHEDULE_OUTPUT_STORAGE_KEY, JSON.stringify(payload));
}

function setOutputText(outputBox, text, color) {
    outputBox.innerText = text;
    outputBox.style.color = color;
    saveOutput({ type: 'text', text, color });
}

function setOutputHtml(outputBox, html, color) {
    outputBox.innerHTML = html;
    outputBox.style.color = color;
    saveOutput({ type: 'html', html, color });
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

        const output = JSON.parse(savedOutput);
        outputBox.style.color = output.color || 'black';
        if (output.type === 'html') {
            outputBox.innerHTML = output.html;
        } else {
            outputBox.innerText = output.text;
        }
        if (typeof addStorageNotice === 'function') {
            addStorageNotice('\u5df2\u6062\u590d\u4e0a\u6b21\u6392\u8bfe\u53cd\u9988\uff1b\u53ef\u70b9\u51fb clear output \u6e05\u7a7a\u3002');
        }
    } catch (error) {
        console.warn('\u8bfb\u53d6\u5df2\u4fdd\u5b58\u6392\u8bfe\u53cd\u9988\u5931\u8d25\uff1a', error);
    }
}

function clearOutput() {
    const outputBox = document.getElementById('outputContent');
    localStorage.removeItem(SCHEDULE_OUTPUT_STORAGE_KEY);

    if (outputBox) {
        outputBox.innerText = DEFAULT_OUTPUT_TEXT;
        outputBox.style.color = 'black';
    }

    const notice = document.getElementById('storageNotice');
    if (notice) {
        notice.innerText = notice.innerText
            .replace('\u5df2\u6062\u590d\u4e0a\u6b21\u6392\u8bfe\u53cd\u9988\uff1b\u53ef\u70b9\u51fb clear output \u6e05\u7a7a\u3002', '')
            .trim();
    }
}

function renderCourseRows(courses) {
    return courses.map((course) => `
        <tr>
            <td>${escapeOutputHtml(course.day)}</td>
            <td>${escapeOutputHtml(course.name)}</td>
            <td>${escapeOutputHtml(course.num)}</td>
            <td>${escapeOutputHtml(course.starttime)} - ${escapeOutputHtml(course.endtime)}</td>
        </tr>`).join('');
}

function renderScheduleResult(schedule) {
    const courses = Array.isArray(schedule.courses) ? schedule.courses : [];
    const conflicts = Array.isArray(schedule.conflicts) ? schedule.conflicts : [];

    let html = `<p>${schedule.has_conflict ? '\u68c0\u6d4b\u5230\u8bfe\u7a0b\u51b2\u7a81' : '\u672a\u68c0\u6d4b\u5230\u8bfe\u7a0b\u51b2\u7a81'}</p>`;
    html += `
        <h4>\u8bfe\u7a0b\u8868</h4>
        <table border="1" style="width: 100%; text-align: left; border-collapse: collapse;">
            <thead>
                <tr>
                    <th>\u5468\u51e0</th>
                    <th>\u8bfe\u7a0b\u540d\u79f0</th>
                    <th>\u7f16\u53f7</th>
                    <th>\u65f6\u95f4</th>
                </tr>
            </thead>
            <tbody>${renderCourseRows(courses)}</tbody>
        </table>`;

    if (conflicts.length > 0) {
        html += '<h4>\u51b2\u7a81\u8be6\u60c5</h4>';
        conflicts.forEach((conflict, index) => {
            html += `
                <div class="conflict-block">
                    <strong>\u51b2\u7a81 ${index + 1}：</strong>
                    \u661f\u671f${escapeOutputHtml(conflict.day)}
                    ${escapeOutputHtml(conflict.starttime)} - ${escapeOutputHtml(conflict.endtime)}
                    <table border="1" style="width: 100%; text-align: left; border-collapse: collapse; margin-top: 6px;">
                        <thead>
                            <tr>
                                <th>\u8bfe\u7a0b\u540d\u79f0</th>
                                <th>\u7f16\u53f7</th>
                                <th>\u65f6\u95f4</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${(conflict.courses || []).map((course) => `
                                <tr>
                                    <td>${escapeOutputHtml(course.name)}</td>
                                    <td>${escapeOutputHtml(course.num)}</td>
                                    <td>${escapeOutputHtml(course.starttime)} - ${escapeOutputHtml(course.endtime)}</td>
                                </tr>`).join('')}
                        </tbody>
                    </table>
                </div>`;
        });
    }

    return html;
}

async function uploadData(event) {
    if (event && typeof event.preventDefault === 'function') {
        event.preventDefault();
    }

    const outputBox = document.getElementById('outputContent');
    const submitBtn = document.getElementById('submitBtn');

    if (!Array.isArray(datalist) || datalist.length === 0) {
        setOutputText(outputBox, '\u8bf7\u5148\u6dfb\u52a0\u81f3\u5c11\u4e00\u95e8\u8bfe\u7a0b', 'red');
        return;
    }

    const payload = JSON.stringify(datalist);
    outputBox.innerText = '\u6b63\u5728\u6392\u8bfe\uff0c\u8bf7\u7a0d\u540e...';
    outputBox.style.color = 'blue';

    if (submitBtn) {
        submitBtn.disabled = true;
    }

    try {
        const response = await fetch(API_BASE_URL + '/api/make_schedule', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: payload,
        });

        const rawText = await response.text();
        console.log('\u670d\u52a1\u5668\u539f\u59cb\u8fd4\u56de\uff1a', rawText);

        let result;
        try {
            result = JSON.parse(rawText);
        } catch (e) {
            setOutputText(outputBox, '\u540e\u7aef\u8fd4\u56de\u7684\u4e0d\u662f JSON:\n' + rawText, 'red');
            return;
        }

        if (response.ok) {
            setOutputHtml(outputBox, renderScheduleResult(result.schedule || {}), 'black');
        } else {
            setOutputText(outputBox, '\u670d\u52a1\u5668\u9519\u8bef\uff1a' + (result.message || rawText), 'red');
        }
    } catch (error) {
        setOutputText(outputBox, '\u8bf7\u6c42\u5931\u8d25\uff1a' + error.message, 'red');
    } finally {
        if (submitBtn) {
            submitBtn.disabled = false;
        }
    }
}

restoreOutput();
