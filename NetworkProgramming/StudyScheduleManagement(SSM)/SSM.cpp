#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
using namespace std;

typedef struct studentAccount
{
    string Sid;
    string password;
} StudentAccount;

typedef struct studentCourse
{
    string Sid;
    string Cid;
} StudentCourse;

typedef struct course
{
    string Cid;
    string Ccode;
    string Cname;
    string CtimeStart;
    string CtimeEnd;
    string Croom;
} Course;

string currentSid;
vector<StudentAccount> studentAccounts;
vector<StudentCourse> studentCourses;
vector<Course> courses;


void LoadStudentAccounts(const string &filename, vector<StudentAccount> &studentAccounts)
{
    vector<string> lines;
    ifstream file(filename);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            StudentAccount account;
            getline(ss, account.Sid, '\t');
            getline(ss, account.password, '\t');
            studentAccounts.push_back(account);
        }
        cout << "Loaded " << studentAccounts.size() << " student accounts." << endl;
        file.close();
    } else {
        cout << "Unable to open file: " << filename << endl;
        return;
    }
}

void login(vector<StudentAccount> &studentAccounts){
    string Sid, password;
    cout << "Enter Student ID: ";
    cin >> Sid;
    cout << "Enter Password: ";
    cin >> password;

    bool found = false;
    for (const auto &account : studentAccounts)
    {
        if (account.Sid == Sid && account.password == password)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        cout << "Login successful!" << endl;
    }
    else
    {
        cout << "Invalid Student ID or Password." << endl;
    }
}

void viewSchedulebyWeek(const vector<StudentCourse> &studentCourses, const vector<Course> &courses)
{
    vector<string> enrolledCourseIds;
    for (const auto &sc : studentCourses)
    {
        if (sc.Sid == currentSid)
        {
            enrolledCourseIds.push_back(sc.Cid);
        }
    }

    if (enrolledCourseIds.empty())
    {
        cout << "No courses found for Student ID: " << currentSid << endl;
        return;
    }

    cout << "Courses for Student ID " << currentSid << ":" << endl;
    for (const auto &cid : enrolledCourseIds)
    {
        for (const auto &course : courses)
        {
            if (course.Cid == cid)
            {
                cout << "Course Code: " << course.Ccode << ", Course Name: " << course.Cname
                     << ", Time: " << course.CtimeStart << " - " << course.CtimeEnd
                     << ", Room: " << course.Croom << endl;
            }
        }
    }
}

void viewSchedulebyDay(const vector<StudentCourse> &studentCourses, const vector<Course> &courses)
{
    string day;
    cout << "Enter day (e.g., Monday): ";
    cin >> day;

    vector<string> enrolledCourseIds;
    for (const auto &sc : studentCourses)
    {
        if (sc.Sid == currentSid)
        {
            enrolledCourseIds.push_back(sc.Cid);
        }
    }

    if (enrolledCourseIds.empty())
    {
        cout << "No courses found for Student ID: " << currentSid << endl;
        return;
    }

    cout << "Courses for Student ID " << currentSid << " on " << day << ":" << endl;
    for (const auto &cid : enrolledCourseIds)
    {
        for (const auto &course : courses)
        {
            if (course.Cid == cid && course.CtimeStart.find(day) != string::npos)
            {
                cout << "Course Code: " << course.Ccode << ", Course Name: " << course.Cname
                     << ", Time: " << course.CtimeStart << " - " << course.CtimeEnd
                     << ", Room: " << course.Croom << endl;
            }
        }
    }
}
int main(){
    
}