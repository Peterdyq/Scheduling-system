#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

class schedule;
class course;
class timeslot;
struct event;
struct printItem;

/*
最基本的时间块
包含时间点的基本起始点，以整型数据存储
*/
class timeslot
{
    friend bool check_overlap(schedule &s);
    friend class course;
    friend ostream &operator<<(ostream &os, timeslot &ts);
    friend ostream &operator<<(ostream &os, course &c);

private:
    int day;
    int startmin;
    int endmin;

public:
    timeslot(int day, int startmin, int endmin) : day(day), startmin(startmin), endmin(endmin)
    {
    }
    bool operator<(const timeslot &t) const
    {
        if (this->day != t.day)
        {
            return this->day < t.day;
        }
        return this->startmin < t.startmin;
    }
};

/*
课程对象
包含时间块列表，课程名字以及课程编号
*/
class course
{
    friend class schedule;
    friend schedule *init_from_terminal();
    friend schedule *init_from_file(string file_route);
    friend bool check_overlap(schedule &s);
    friend ostream &operator<<(ostream &os, vector<course> v);
    friend ostream &operator<<(ostream &os, course &c);
    friend course &search_with_name(schedule &s, string tarname);
    friend void erase_with_name(schedule &s, string tarname);
    friend course &search_with_name(schedule &s, int tarnum);
    friend void erase_with_num(schedule &s, int tarnum);
    friend void sort(schedule &s, vector<printItem> &v);

private:
    vector<timeslot> ts;
    string name;
    int num;

public:
    course(vector<timeslot> ts, string name, int num) : ts(ts), name(name), num(num)
    {
    }

    bool operator==(const course &other) const
    {
        return this->num == other.num; // 只要编号一样，就认为是同一门课
    }
};

/*
课程表对象
包含一个课程的vector容器
*/
class schedule
{
    friend schedule *init_from_terminal();
    friend schedule *init_from_file(string file_route);
    friend bool check_overlap(schedule &s);
    friend course &search_with_name(schedule &s, string name);
    friend void erase_with_name(schedule &s, string tarname);
    friend course &search_with_name(schedule &s, int tarnum);
    friend void erase_with_num(schedule &s, int tarnum);
    friend void sort(schedule &s, vector<printItem> &v);
    friend schedule *init_from_json(schedule *s, const string &filename);

private:
    vector<course> sch;

public:
    schedule()
    {
    }
};

// 优化查找算法的“事件”结构体（时间轴扫描）
struct event
{
    int starttime;
    int endtime;
};

// 打印对象
struct printItem
{
    timeslot ts;
    string name;
    bool operator<(const printItem &p2) const
    {
        return this->ts < p2.ts;
    }
};

// 从终端初始化
schedule *init_from_terminal()
{
    map<string, bool> m;
    schedule *s = new schedule();
    while (cin)
    {
        int day, startmin, endmin;
        string name;
        int num;
        cout << "please enter the name and the class number" << endl;
        cin >> name >> num;
        cout << "please enter the day, start time and endtime" << endl;
        cin >> day >> startmin >> endmin;
        if (!m[name])
        {
            m[name] = true;
            vector<timeslot> vslot(1, timeslot(day, startmin, endmin));
            course c(vslot, name, num);
            s->sch.push_back(c);
        }
        else
        {
            for (vector<course>::iterator it = s->sch.begin(); it != s->sch.end(); it++)
            {
                if (it->name == name)
                {
                    it->ts.push_back({day, startmin, endmin});
                }
            }
        }
    }
    return s;
}

// 从文本文件初始化
schedule *init_from_file(string file_route)
{
    ifstream ifs;
    ifs.open(file_route, ios::in);
    if (!ifs.is_open())
    {
        cout << "Unable to open file" << endl;
        return NULL;
    }
    vector<string> vstring;
    string input;
    while (getline(ifs, input))
    {
        vstring.push_back(input);
    }
    vstring.shrink_to_fit();
    schedule *s = new schedule();
    // 手动处理字符串部分
    for (vector<string>::iterator it = vstring.begin(); it != vstring.end(); it++)
    {
        size_t last_pos = (*it).find_last_not_of("\n\t\r");
        (*it).erase(last_pos + 1);
        int day, startmin, endmin;
        string name;
        int num;
        int count = 1;
        int lastidx = 0;
        for (int i = 0; i < (*it).length(); i++)
        {
            if ((*it)[i] == ' ' || i == (*it).length() - 1)
            {
                switch (count++)
                {
                case 1:
                    name = (*it).substr(lastidx, i - lastidx);
                    break;
                case 2:
                    num = stoi((*it).substr(lastidx, i - lastidx));
                    break;
                case 3:
                    day = stoi((*it).substr(lastidx, i - lastidx));
                    break;
                case 4:
                    startmin = stoi((*it).substr(lastidx, i - lastidx));
                    break;
                case 5:
                    endmin = stoi((*it).substr(lastidx, i - lastidx + 1));
                    break;
                default:
                    break;
                }
                lastidx = i + 1;
            }
        }
        vector<timeslot> vslot(1, timeslot(day, startmin, endmin));
        course c(vslot, name, num);
        s->sch.push_back(c);
    }
    return s;
}

// 从json文件初始化
schedule *init_from_json(schedule *sche, const string &filename)
{
    ifstream ifs;
    schedule *s;
    if (sche == NULL)
    {
        s = new schedule();
    }
    else
    {
        s = sche;
    }
    ifs.open(filename, ios::in);
    if (!ifs.is_open())
    {
        cout << "Unable to open file" << endl;
        return NULL;
    }
    json j;
    ifs >> j;
    auto course_list = j["course_list"];
    for (auto item : course_list)
    {
        string name = item["name"];
        int num = stoi(item["num"].get<string>());
        int day = item["day"];
        string starttime1 = item["starttime"].get<string>().substr(0, 2);
        string starttime2 = item["starttime"].get<string>().substr(3, 2);
        string endtime1 = item["endtime"].get<string>().substr(0, 2);
        string endtime2 = item["endtime"].get<string>().substr(3, 2);
        int starttime = stoi(starttime1) * 60 + stoi(starttime2);
        int endtime = stoi(endtime1) * 60 + stoi(endtime2);
        timeslot ts(day, starttime, endtime);
        vector<timeslot> v;
        v.push_back(ts);
        course c(v, name, num);
        s->sch.push_back(c);
    }
    cout << "Course initialized/added" << endl;
    return s;
}

// 检查是否有重复课程
bool check_overlap(schedule &s)
{
    // Key: 绝对分钟数 (day * 1440 + min)
    // Value: 变化量 (+1 表示开始, -1 表示结束)
    int eventcount = 0;
    multimap<int, pair<int, course>> timeline; // 记录开始和结束
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        for (vector<timeslot>::iterator it2 = (it->ts).begin(); it2 != (it->ts).end(); it2++)
        {
            int starttime = (*it2).startmin + (*it2).day * 1440;
            int endtime = it2->endmin + (it2->day) * 1440;
            timeline.insert({starttime, {1, *it}});
            timeline.insert({endtime, {-1, *it}});
        }
    }
    vector<course> activeclasses;
    // pair的第一个int代表当前课程是开始还是结束
    // map的第一个int代表课程时间
    for (multimap<int, pair<int, course>>::iterator mit = timeline.begin(); mit != timeline.end();)
    {
        auto range = timeline.equal_range(mit->first);
        // 循环同一个key的列表
        for (auto keyit = range.first; keyit != range.second; keyit++)
        {

            if (keyit->second.first == -1)
            {
                eventcount += keyit->second.first;
                auto it = find(activeclasses.begin(), activeclasses.end(), keyit->second.second);
                if (it != activeclasses.end())
                {
                    activeclasses.erase(it);
                }
            }
        }
        for (auto keyit = range.first; keyit != range.second; keyit++)
        {

            if (keyit->second.first == 1)
            {
                activeclasses.push_back(keyit->second.second);
                eventcount += keyit->second.first;
            }
            if (eventcount > 1)
            {
                cout << "Course overlap detected" << endl;
                cout << "Overlapping course: " << endl;
                for (course c : activeclasses)
                {
                    cout << c.name << endl;
                    cout << c.num << endl;
                    int abs_starttime = mit->first;
                    int abs_endtime = mit->first + (c.ts[0].endmin - c.ts[0].startmin);
                    cout << "starttime: " << (abs_starttime % 1440) / 60 << " : " << abs_starttime % 60 << endl;
                    cout << "endtime: " << (abs_endtime % 1440) / 60 << " : " << abs_endtime % 60 << endl;
                }
                return true;
            }
        }
        mit = range.second;
    }
    cout << "There is no course overlap" << endl;
    return false;
}

ostream &operator<<(ostream &os, timeslot &ts)
{
    os << "startmin: " << ts.startmin << " endmin: " << ts.endmin;
    return os;
}

ostream &operator<<(ostream &os, vector<course> v)
{
    for (vector<course>::iterator it = v.begin(); it != v.end(); it++)
    {
        os << "classname: " << it->name << " " << "class number" << it->num << endl;
    }
    return os;
}
ostream &operator<<(ostream &os, course &c)
{
    for (vector<timeslot>::iterator it = c.ts.begin(); it != c.ts.end(); it++)
    {
        cout << "Day: " << it->day << " Startmin: " << it->startmin << " Endmin: " << it->endmin << endl;
    }
    return os;
}
course &search_with_name(schedule &s, string tarname)
{
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        if (it->name == tarname)
        {
            cout << &(*it) << endl;
            return *it;
        }
    }
}

// 查找课程
course &search_with_name(schedule &s, int tarnum)
{
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        if (it->num == tarnum)
        {
            cout << &(*it) << endl;
            return *it;
        }
    }
}

// 删除课程
void erase_with_name(schedule &s, string tarname)
{
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        if (it->name == tarname)
        {
            s.sch.erase(it);
            return;
        }
    }
}

void erase_with_num(schedule &s, int tarnum)
{
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        if (it->num == tarnum)
        {
            s.sch.erase(it);
            return;
        }
    }
}

/*
课程排序函数
按照宏观的自然时间轴
*/
void sort(schedule &s, vector<printItem> &pi)
{
    pi.reserve(s.sch.size());
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        for (vector<timeslot>::iterator it2 = it->ts.begin(); it2 != it->ts.end(); it2++)
        {
            pi.push_back({*it2, it->name});
        }
    }
    std::sort(pi.begin(), pi.end());
}

// 打印课程表
void printschedule(schedule &s)
{
    vector<printItem> v;
    sort(s, v);
    for (vector<printItem>::iterator it = v.begin(); it != v.end(); it++)
    {
        cout << "classname: " << it->name << " " << it->ts << endl;
    }
    cout.flush();
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Error: please provide the json file path" << endl;
        return 1;
    }

    const string json_path = argv[1];
    schedule *my_schedule = new schedule();

    try
    {
        my_schedule = init_from_json(my_schedule, json_path);

        check_overlap(*my_schedule);
        printschedule(*my_schedule);
    }
    catch (const std::exception &e)
    {
        cerr << "解析出错: " << e.what() << endl;
        return 1;
    }
    cout.flush();
    return 0;
}