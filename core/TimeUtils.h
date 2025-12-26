#pragma once

#include <ctime>

namespace ob::time {

inline std::tm localtime_safe(std::time_t t) {
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &t);
#else
    localtime_r(&t, &result);
#endif
    return result;
}

inline std::string format_time(std::time_t t, const char* fmt = "%Y-%m-%d %H:%M:%S") {
    std::tm tm = localtime_safe(t);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), fmt, &tm);
    return buffer;
}

}