//
// Created by C66 on 2025/11/23.
//
// TimeStamp.h
#pragma once
#ifndef VULKAN_TIMESTAMP_H
#define VULKAN_TIMESTAMP_H


#include <chrono>
#include <cstdint>
#include <string>
#include <iomanip>
#include <sstream>

class TimeStamp
{
    public:
        using Clock     = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        // 从当前系统时间创建
        static TimeStamp now()
        {
            return TimeStamp{Clock::now()};
        }

        // 从 ISO8601 字符串解析（例如 "2025-11-23T14:24:20.577Z"）
        static TimeStamp fromIso8601(const std::string& iso)
        {
            std::tm tm{};
            char    dot    = 0;
            int     millis = 0;

            // 先解析到秒
            std::istringstream ss(iso);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (!ss.fail())
            {
                // 可选的小数秒部分（.xxxZ）
                if (ss.peek() == '.')
                {
                    ss >> dot >> millis;
                }
                // 忽略末尾的 'Z'
            }

            auto tp = Clock::from_time_t(_mkgmtime(&tm)); // Windows 下使用 _mkgmtime
            tp += std::chrono::milliseconds(millis);
            return TimeStamp{tp};
        }

        // 输出 ISO8601 字符串，如 2025-11-23T14:24:20.577Z
        std::string toIso8601() const
        {
            using namespace std::chrono;
            auto    tp   = timePoint_;
            auto    secs = time_point_cast<seconds>(tp);
            auto    diff = duration_cast<milliseconds>(tp - secs);
            time_t  t    = Clock::to_time_t(secs);
            std::tm tm{};
            gmtime_s(&tm, &t); // 转为 UTC

            std::ostringstream ss;
            ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
            ss << '.' << std::setw(3) << std::setfill('0') << diff.count() << 'Z';
            return ss.str();
        }

        // 计算两个时间戳之间的毫秒差（this - other）
        std::int64_t diffMillis(const TimeStamp& other) const
        {
            using namespace std::chrono;
            return duration_cast<milliseconds>(timePoint_ - other.timePoint_).count();
        }

        const TimePoint& timePoint() const { return timePoint_; }

    private:
        explicit TimeStamp(const TimePoint& tp) : timePoint_(tp)
        {
        }

        TimePoint timePoint_{};
};

#endif //VULKAN_TIMESTAMP_H
