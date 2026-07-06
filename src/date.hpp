#pragma once

#include <chrono>
#include <ctime>
#include <string>

class Date
{
public:
    Date();
    explicit Date(std::chrono::sys_days days);
    Date(int year, int month, int day);
    explicit Date(const std::string& text); // YYYY-MM-DD
    explicit Date(const std::tm& tm);
    Date operator+(int days) const;
    int operator-(const Date& other) const;
    double ToEpoch(int hm = 0) const;
    std::string ToString() const;
    std::tm ToTm() const;

private:
    std::chrono::sys_days Days;
};
