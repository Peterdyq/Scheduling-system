#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
using namespace std;

class schedule;
class course;
class timeslot;
struct event;
struct printItem;

class timeslot
{
    friend void check_overlap(schedule &s);
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

class course
{
    friend class schedule;
    friend schedule *init_from_terminal();
    friend schedule *init_from_file(string file_route);
    friend void check_overlap(schedule &s);
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
};
class schedule
{
    friend schedule *init_from_terminal();
    friend schedule *init_from_file(string file_route);
    friend void check_overlap(schedule &s);
    friend course &search_with_name(schedule &s, string name);
    friend void erase_with_name(schedule &s, string tarname);
    friend course &search_with_name(schedule &s, int tarnum);
    friend void erase_with_num(schedule &s, int tarnum);
    friend void sort(schedule &s, vector<printItem> &v);

private:
    vector<course> sch;

public:
    schedule()
    {
    }
};

struct event
{
    int starttime;
    int endtime;
};

struct printItem
{
    timeslot ts;
    string name;
    bool operator<(const printItem &p2) const
    {
        return this->ts < p2.ts;
    }
};

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

void check_overlap(schedule &s)
{
    // Key: 绝对分钟数 (day * 1440 + min)
    // Value: 变化量 (+1 表示开始, -1 表示结束)
    int eventcount = 0;
    map<int, pair<int, vector<course>>> timeline; // 记录开始和结束
    for (vector<course>::iterator it = s.sch.begin(); it != s.sch.end(); it++)
    {
        for (vector<timeslot>::iterator it2 = (it->ts).begin(); it2 != (it->ts).end(); it2++)
        {
            int starttime = (*it2).startmin + (*it2).day * 1440;
            int endtime = it2->endmin + (it2->day) * 1440;
            timeline[starttime].first++;
            timeline[endtime].first--;
            timeline[starttime].second.push_back(*it);
        }
    }
    vector<course> formerclasses;
    for (map<int, pair<int, vector<course>>>::iterator mit = timeline.begin(); mit != timeline.end(); mit++)
    {
        eventcount += mit->second.first;
        if (eventcount > 1)
        {
            cout << "Course overlap detected" << endl;
            cout << "Overlapping course: " << formerclasses << ", " << mit->second.second << endl;
            return;
        }
        if (mit->second.first >= 1)
        {
            formerclasses = mit->second.second;
        }
    }
    cout << "There is no course overlap" << endl;
    return;
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

void printschedule(schedule &s)
{
    vector<printItem> v;
    sort(s, v);
    for (vector<printItem>::iterator it = v.begin(); it != v.end(); it++)
    {
        cout << "classname: " << it->name << it->ts << endl;
    }
}

int main()
{
}