const COURSE_LIST_STORAGE_KEY = 'scheduling_system_course_list';
const TIME_PATTERN = /^([01]\d|2[0-3]):([0-5]\d)$/;

let datalist = loadCourseList();
let editingIndex = null;

function addStorageNotice(message) {
    const notice = document.getElementById('storageNotice');
    if (!notice) {
        return;
    }

    const existing = notice.innerText.trim();
    notice.innerText = existing ? `${existing} ${message}` : message;
}

function loadCourseList() {
    try {
        const savedCourses = localStorage.getItem(COURSE_LIST_STORAGE_KEY);
        return savedCourses ? JSON.parse(savedCourses) : [];
    } catch (error) {
        console.warn('\u8bfb\u53d6\u5df2\u4fdd\u5b58\u8bfe\u7a0b\u5931\u8d25\uff1a', error);
        return [];
    }
}

function saveCourseList() {
    localStorage.setItem(COURSE_LIST_STORAGE_KEY, JSON.stringify(datalist));
}

function getCourseFormData() {
    return {
        name: document.getElementById('coursename').value.trim(),
        num: document.getElementById('coursenum').value.trim(),
        day: Number(document.getElementById('day').value),
        starttime: document.getElementById('starttime').value.trim(),
        endtime: document.getElementById('endtime').value.trim(),
    };
}

function parseTimeToMinutes(value) {
    if (!TIME_PATTERN.test(value)) {
        return null;
    }

    const [hours, minutes] = value.split(':').map(Number);
    return hours * 60 + minutes;
}

function validateCourse(course) {
    if (!course.name) {
        return '\u8bfe\u7a0b\u540d\u79f0\u4e0d\u80fd\u4e3a\u7a7a';
    }
    if (!course.num) {
        return '\u8bfe\u7a0b\u7f16\u53f7\u4e0d\u80fd\u4e3a\u7a7a';
    }
    if (!Number.isInteger(course.day) || course.day < 1 || course.day > 7) {
        return '\u5468\u51e0\u5fc5\u987b\u662f 1-7 \u7684\u6574\u6570';
    }

    const startMinutes = parseTimeToMinutes(course.starttime);
    const endMinutes = parseTimeToMinutes(course.endtime);
    if (startMinutes === null) {
        return '\u5f00\u59cb\u65f6\u95f4\u5fc5\u987b\u662f HH:MM \u683c\u5f0f\uff0c\u4f8b\u5982 08:00';
    }
    if (endMinutes === null) {
        return '\u7ed3\u675f\u65f6\u95f4\u5fc5\u987b\u662f HH:MM \u683c\u5f0f\uff0c\u4f8b\u5982 09:30';
    }
    if (startMinutes >= endMinutes) {
        return '\u5f00\u59cb\u65f6\u95f4\u5fc5\u987b\u65e9\u4e8e\u7ed3\u675f\u65f6\u95f4';
    }

    return '';
}

function setFormError(message) {
    document.getElementById('formError').innerText = message;
}

function clearCourseForm() {
    document.getElementById('coursename').value = '';
    document.getElementById('coursenum').value = '';
    document.getElementById('day').value = '';
    document.getElementById('starttime').value = '';
    document.getElementById('endtime').value = '';
}

function fillCourseForm(course) {
    document.getElementById('coursename').value = course.name;
    document.getElementById('coursenum').value = course.num;
    document.getElementById('day').value = course.day;
    document.getElementById('starttime').value = course.starttime;
    document.getElementById('endtime').value = course.endtime;
}

function updateAddButtonText() {
    document.getElementById('addBtn').innerText = editingIndex === null ? 'add' : 'save';
}

function clickevent() {
    const newcourse = getCourseFormData();
    const validationError = validateCourse(newcourse);
    if (validationError) {
        setFormError(validationError);
        return;
    }

    if (editingIndex === null) {
        datalist.push(newcourse);
        alert('\u5df2\u6dfb\u52a0\u8bfe\u7a0b');
    } else {
        datalist[editingIndex] = newcourse;
        editingIndex = null;
        updateAddButtonText();
        alert('\u5df2\u4fdd\u5b58\u8bfe\u7a0b');
    }

    setFormError('');
    clearCourseForm();
    saveCourseList();
    renderCourseList();
}

function editCourse(index) {
    editingIndex = index;
    fillCourseForm(datalist[index]);
    setFormError('');
    updateAddButtonText();
}

function deleteCourse(index) {
    datalist.splice(index, 1);
    if (editingIndex === index) {
        editingIndex = null;
        clearCourseForm();
        updateAddButtonText();
    } else if (editingIndex !== null && editingIndex > index) {
        editingIndex -= 1;
    }

    saveCourseList();
    renderCourseList();
}

function clearCourseList() {
    datalist = [];
    editingIndex = null;
    localStorage.removeItem(COURSE_LIST_STORAGE_KEY);
    clearCourseForm();
    setFormError('');
    const notice = document.getElementById('storageNotice');
    if (notice) {
        notice.innerText = '';
    }
    updateAddButtonText();
    renderCourseList();
}

function escapeHtml(value) {
    return String(value)
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#039;');
}

function renderCourseList() {
    const displayArea = document.getElementById('courseListDisplay');

    if (datalist.length === 0) {
        displayArea.innerHTML = '<p>\u6682\u65e0\u8bfe\u7a0b</p>';
        return;
    }

    let html = `
        <table border="1" style="width: 100%; text-align: left; border-collapse: collapse;">
            <thead>
                <tr style="background-color: #f2f2f2;">
                    <th>\u8bfe\u7a0b\u540d\u79f0</th>
                    <th>\u7f16\u53f7</th>
                    <th>\u5468\u51e0</th>
                    <th>\u65f6\u95f4</th>
                    <th>\u64cd\u4f5c</th>
                </tr>
            </thead>
            <tbody>`;

    datalist.forEach((course, index) => {
        html += `
            <tr>
                <td>${escapeHtml(course.name)}</td>
                <td>${escapeHtml(course.num)}</td>
                <td>\u661f\u671f${escapeHtml(course.day)}</td>
                <td>${escapeHtml(course.starttime)} - ${escapeHtml(course.endtime)}</td>
                <td>
                    <button type="button" onclick="editCourse(${index})">edit</button>
                    <button type="button" onclick="deleteCourse(${index})">delete</button>
                </td>
            </tr>`;
    });

    html += '</tbody></table>';
    displayArea.innerHTML = html;
}

updateAddButtonText();
renderCourseList();
if (datalist.length > 0) {
    addStorageNotice('\u5df2\u6062\u590d\u4e0a\u6b21\u4fdd\u5b58\u7684\u8bfe\u7a0b\uff1b\u53ef\u70b9\u51fb clear courses \u6e05\u7a7a\u3002');
}
