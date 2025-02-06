#include "utils.hpp"
#include "logger.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <regex>

std::string getDateStr(int64_t time_ms) {
  if (time_ms < 0) {
    return std::string("none");
  }

  int ms = time_ms % 1000;
  int64_t time_s = time_ms / 1000;

  std::chrono::system_clock::time_point tp
    = std::chrono::system_clock::from_time_t(time_s);

  // 将 time_point 转换为 std::tm
  std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::tm* time_info = std::localtime(&t);
  std::stringstream time_ss;
  time_ss << std::put_time(time_info, "%Y%m%d %H:%M:%S") << "+" << std::setw(3) << std::setfill('0') << ms;
  return time_ss.str();
}

std::string getTimeStr(int64_t time_ms) {
  std::stringstream ss;
  if (time_ms >= 3600000) {
    int64_t h = time_ms / 3600000;
    time_ms = time_ms % 3600000;
    ss << std::to_string(h) << "h";
  }
  if (time_ms >= 60000) {
    int min = time_ms / 60000;
    time_ms = time_ms % 60000;
    ss << min << "min";
  }
  if (time_ms >= 1000) {
    int s = time_ms / 1000;
    time_ms = time_ms % 1000;
    ss << s << "s";
  }
  // if (time_ms > 0) {
  //   ss << time_ms << "ms";
  // }
  return ss.str();
}

int64_t getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return millis.count();
}

// 检查月份是否有效
bool isValidMonth(int month) {
    return month >= 1 && month <= 12;
}

// 检查日期是否有效
bool isValidDay(int year, int month, int day) {
    if (day < 1 || day > 31) return false;

    // 检查月份对应的日期
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return day <= 30; // 4 月、6 月、9 月、11 月最多 30 天
    } else if (month == 2) {
        // 检查闰年
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return day <= (isLeapYear ? 29 : 28); // 2 月最多 28 或 29 天
    } else {
        return day <= 31; // 其他月份最多 31 天
    }
}

// 检查时间值是否有效
bool isValidTime(int hour, int minute, int second, int milliseconds) {
    return (hour >= 0 && hour <= 23) &&
           (minute >= 0 && minute <= 59) &&
           (second >= 0 && second <= 59) &&
           (milliseconds >= 0 && milliseconds <= 999);
}

bool isFutureTime(const std::tm& tm, int milliseconds) {
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::time_t time = std::mktime(const_cast<std::tm*>(&tm));
    auto inputMs = time * 1000 + milliseconds;

    // 比较时间戳
    return inputMs > nowMs;
}

int64_t timeParserDate(const std::string &timeStr) {
    std::regex pattern(R"((\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2})\+(\d{3}))");
    std::smatch match;

    int64_t finalTimestamp(-1);
    do {
      if (std::regex_match(timeStr, match, pattern)) {
          int year = std::stoi(match[1]);
          int month = std::stoi(match[2]);
          int day = std::stoi(match[3]);
          int hour = std::stoi(match[4]);
          int minute = std::stoi(match[5]);
          int second = std::stoi(match[6]);
          int milliseconds = std::stoi(match[7]);

          // 检查时间值的有效性
          if (!isValidMonth(month)) {
              pLogger->warn("invalid month:{}", month);
              break;
          }
          if (!isValidDay(year, month, day)) {
              pLogger->warn("invalid day:{}", day);
              break;
          }
          if (!isValidTime(hour, minute, second, milliseconds)) {
              pLogger->warn("invalid time:{}h, {}m, {}s, {}ms",
              hour, minute, second, milliseconds);
              break;
          }

          // 构造 tm 结构体
          std::tm tm = {};
          tm.tm_year = year - 1900; // tm_year 是从 1900 开始的年份
          tm.tm_mon = month - 1;    // tm_mon 是从 0 开始的月份
          tm.tm_mday = day;
          tm.tm_hour = hour;
          tm.tm_min = minute;
          tm.tm_sec = second;

          // 检查时间是否大于当前时间
          if (!isFutureTime(tm, milliseconds)) {
              pLogger->warn("Time is not in the future");
              break;
          }

          // 将 tm 转换为 time_t（秒级时间戳）
          std::time_t time = std::mktime(&tm);

          // 转换为 chrono::system_clock::time_point
          auto timePoint = std::chrono::system_clock::from_time_t(time);

          // 加上毫秒部分
          timePoint += std::chrono::milliseconds(milliseconds);

          // 计算时间戳（从 1970-01-01 开始的毫秒数）
          finalTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              timePoint.time_since_epoch()).count();
      } else {
        pLogger->warn("invalid time format:{}, should:yyyymmdd-hhmmss+xxx");
      }
    } while(0);
    return finalTimestamp;
}

int64_t timeParserValue(const std::string &str) {
  std::regex time_pattern(R"(^\d+(ms|s|min|h))");
  int64_t finalTime(-1);
  if (std::regex_match(str, time_pattern)) {
    // get digits
    size_t idx(0);
    while(idx < str.length()) {
      if (!std::isdigit(str[idx])) {
        break;
      }
      idx++;
    }
    finalTime = std::atol(str.substr(0, idx).c_str());
    std::string unit = str.substr(idx);
    if (unit == "s") { 
      finalTime *= 1000;
    } else if (unit == "min") {
      finalTime *= 1000 * 60;
    } else if (unit == "h") {
      finalTime *= 1000 * 60 * 60;
    }
  }
  return finalTime;
}

std::string TStatusToStr(int status) {
    switch (status) {
        case 0: return std::string("Todo");
        case 1: return std::string("Progress");
        case 2: return std::string("Pause");
        case 3: return std::string("Done");

    }
    return "Unknown";
}

std::string TStatusToStrS(int status) {
    switch (status) {
        case 0: return std::string("T");
        case 1: return std::string("I");
        case 2: return std::string("P");
        case 3: return std::string("D");

    }
    return "U";
}

