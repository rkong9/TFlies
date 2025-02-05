#include "utils.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>

std::string getTimeStr(int64_t time_ms) {
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

int64_t getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return millis.count();
}
