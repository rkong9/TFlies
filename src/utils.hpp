#ifndef _UTILS_HPP_
#define _UTILS_HPP_
#include <ctime>
#include <string>

std::string getTimeStr(int64_t time_ms);
int64_t getCurrentTimeMs();
bool isValidMonth(int month);
bool isValidDay(int year, int month, int day);
bool isValidTime(int hour, int minute, int second, int milliseconds);
bool isFutureTime(const std::tm& tm, int milliseconds);
int64_t timeParserDate(const std::string &timeStr);
int64_t timeParserValue(const std::string &str);

std::string TStatusToStr(int status);
std::string TStatusToStrS(int status);

#endif

