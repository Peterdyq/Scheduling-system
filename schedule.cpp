#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;
using namespace std;

struct Timeslot
{
    int day;
    int startmin;
    int endmin;

    bool operator<(const Timeslot &other) const
    {
        if (day != other.day)
        {
            return day < other.day;
        }
        return startmin < other.startmin;
    }
};

struct Course
{
    string name;
    string num;
    Timeslot timeslot;
};

struct PrintItem
{
    string name;
    string num;
    Timeslot timeslot;

    bool operator<(const PrintItem &other) const
    {
        return timeslot < other.timeslot;
    }
};

int parse_time_to_minutes(const string &value)
{
    int hours = stoi(value.substr(0, 2));
    int minutes = stoi(value.substr(3, 2));
    return hours * 60 + minutes;
}

string format_time(int minutes)
{
    int hours = minutes / 60;
    int mins = minutes % 60;
    string result;

    if (hours < 10)
    {
        result += "0";
    }
    result += to_string(hours);
    result += ":";

    if (mins < 10)
    {
        result += "0";
    }
    result += to_string(mins);

    return result;
}

vector<Course> load_courses_from_json(const string &filename)
{
    ifstream input(filename);
    if (!input.is_open())
    {
        throw runtime_error("unable to open input JSON file");
    }

    json payload;
    input >> payload;

    vector<Course> courses;
    for (const auto &item : payload.at("course_list"))
    {
        Course course;
        course.name = item.at("name").get<string>();
        course.num = item.at("num").get<string>();
        course.timeslot.day = item.at("day").get<int>();
        course.timeslot.startmin = parse_time_to_minutes(item.at("starttime").get<string>());
        course.timeslot.endmin = parse_time_to_minutes(item.at("endtime").get<string>());
        courses.push_back(course);
    }

    return courses;
}

json course_to_json(const Course &course)
{
    return {
        {"name", course.name},
        {"num", course.num},
        {"day", course.timeslot.day},
        {"startmin", course.timeslot.startmin},
        {"endmin", course.timeslot.endmin},
        {"starttime", format_time(course.timeslot.startmin)},
        {"endtime", format_time(course.timeslot.endmin)}};
}

json build_sorted_courses_json(const vector<Course> &courses)
{
    vector<PrintItem> items;
    items.reserve(courses.size());

    for (const Course &course : courses)
    {
        items.push_back({course.name, course.num, course.timeslot});
    }

    sort(items.begin(), items.end());

    json result = json::array();
    for (const PrintItem &item : items)
    {
        result.push_back({
            {"name", item.name},
            {"num", item.num},
            {"day", item.timeslot.day},
            {"startmin", item.timeslot.startmin},
            {"endmin", item.timeslot.endmin},
            {"starttime", format_time(item.timeslot.startmin)},
            {"endtime", format_time(item.timeslot.endmin)}});
    }

    return result;
}

json find_conflicts(vector<Course> courses)
{
    sort(courses.begin(), courses.end(), [](const Course &a, const Course &b) {
        return a.timeslot < b.timeslot;
    });

    json conflicts = json::array();
    for (size_t i = 0; i < courses.size(); i++)
    {
        for (size_t j = i + 1; j < courses.size(); j++)
        {
            const Course &left = courses[i];
            const Course &right = courses[j];

            if (left.timeslot.day != right.timeslot.day)
            {
                break;
            }

            int overlap_start = max(left.timeslot.startmin, right.timeslot.startmin);
            int overlap_end = min(left.timeslot.endmin, right.timeslot.endmin);
            if (overlap_start < overlap_end)
            {
                conflicts.push_back({
                    {"day", left.timeslot.day},
                    {"startmin", overlap_start},
                    {"endmin", overlap_end},
                    {"starttime", format_time(overlap_start)},
                    {"endtime", format_time(overlap_end)},
                    {"courses", json::array({course_to_json(left), course_to_json(right)})}});
            }
        }
    }

    return conflicts;
}

json build_schedule_json(const vector<Course> &courses)
{
    json conflicts = find_conflicts(courses);
    return {
        {"status", "success"},
        {"has_conflict", !conflicts.empty()},
        {"courses", build_sorted_courses_json(courses)},
        {"conflicts", conflicts}};
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Error: please provide the JSON file path" << endl;
        return 1;
    }

    try
    {
        vector<Course> courses = load_courses_from_json(argv[1]);
        cout << build_schedule_json(courses).dump(4) << endl;
        return 0;
    }
    catch (const exception &error)
    {
        cerr << "Error: " << error.what() << endl;
        return 1;
    }
}
