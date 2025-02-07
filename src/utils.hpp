#ifndef _UTILS_HPP_
#define _UTILS_HPP_
#include <ctime>
#include <string>

std::string getDateStr(int64_t time_ms);
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

std::string TEffiToStr(uint8_t efficiency, bool color=false);
std::string TEffiToStrShort(uint8_t efficiency, bool color=false);

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BLOD    "\033[1m"

std::string getColors(int status);

std::string getTrueColors(uint8_t r, uint8_t g, uint8_t b);

#endif

