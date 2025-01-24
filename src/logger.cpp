#include "logger.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> pLogger = spdlog::stdout_color_mt("console");
