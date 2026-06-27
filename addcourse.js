
const courseform = document.querySelector("#courseform")
const COURSE_LIST_STORAGE_KEY = 'scheduling_system_course_list';
let datalist = loadCourseList();

function loadCourseList() {
    try {
        const savedCourses = localStorage.getItem(COURSE_LIST_STORAGE_KEY);
        return savedCourses ? JSON.parse(savedCourses) : [];
    } catch (error) {
        console.warn('读取已保存课程失败：', error);
        return [];
    }
}

function saveCourseList() {
    localStorage.setItem(COURSE_LIST_STORAGE_KEY, JSON.stringify(datalist));
}

function clickevent() {

    const coursename = document.getElementById('coursename').value;
    const coursenum = document.getElementById('coursenum').value;
    const day = document.getElementById('day').value;
    const starttime = document.getElementById('starttime').value;
    const endtime = document.getElementById('endtime').value;
    const newcourse = {
        "name": coursename,
        "num": coursenum,
        "day": parseInt(day),
        "starttime": starttime,
        "endtime": endtime
    };
    datalist.push(newcourse);
    saveCourseList();
    alert('已添加课程');
    renderCourseList();
}

function renderCourseList() {
    const displayArea = document.getElementById('courseListDisplay');

    if (datalist.length === 0) {
        displayArea.innerHTML = '<p>暂无课程</p>';
        return;
    }

    // 使用表格展示会更整齐
    let html = `
        <table border="1" style="width: 100%; text-align: left; border-collapse: collapse;">
            <thead>
                <tr style="background-color: #f2f2f2;">
                    <th>课程名称</th>
                    <th>编号</th>
                    <th>周几</th>
                    <th>时间</th>
                </tr>
            </thead>
            <tbody>`;

    datalist.forEach((course) => {
        html += `
            <tr>
                <td>${course.name}</td>
                <td>${course.num}</td>
                <td>星期${course.day}</td>
                <td>${course.starttime} - ${course.endtime}</td>
            </tr>`;
    });

    html += `</tbody></table>`;
    displayArea.innerHTML = html;
}

renderCourseList();
