#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <atomic>
#include <vector>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vulkan_engine::logger
{
    // 鏃ュ織绾у埆
    enum class Level
    {
        Debug = 0,
        Info  = 1,
        Warn  = 2,
        Error = 3,
        Fatal = 4
    };

    // 鏃ュ織杈撳嚭鐩爣
    enum class OutputTarget
    {
        Console = 1 << 0,
        File    = 1 << 1,
        All     = Console | File
    };

    // 鏃ュ織閰嶇疆
    struct Config
    {
        Level        min_level = Level::Debug;
        OutputTarget target    = OutputTarget::Console;
        std::string  log_file_path;
        bool         flush_on_every_log = true;
        bool         include_timestamp  = true;
        bool         include_level      = true;
        bool         include_thread_id  = false;
    };

    // 鍐呴儴瀹炵幇鍓嶅悜澹版槑
    class LoggerImpl;

    /**
     * @brief 绾跨▼瀹夊叏鐨勬棩蹇楃郴缁?
     *
     * 鐗规€э細
     * - 鍗曚緥妯″紡锛屽叏灞€鍞竴瀹炰緥
     * - 绾跨▼瀹夊叏锛屾敮鎸佸绾跨▼骞跺彂鍐欏叆
     * - 鏀寔鎺у埗鍙板拰鏂囦欢鍙岃緭鍑?
     * - 鏀寔鏃ュ織绾у埆杩囨护
     * - Windows 涓嬫纭鐞?Unicode 鍜屾帶鍒跺彴缂栫爜
     * - 浠讳綍鎯呭喌涓嬮兘鑳借緭鍑猴紙鍗充娇鏂囦欢鎵撳紑澶辫触锛屼篃浼氬皾璇曟帶鍒跺彴锛?
     */
    class Logger
    {
        public:
            // 鑾峰彇鍗曚緥瀹炰緥
            static Logger& instance();

            // 鍒濆鍖栵紙鍙€夛紝濡傛灉涓嶈皟鐢ㄤ細浣跨敤榛樿閰嶇疆锛?
            void initialize(const Config& config);

            // 鍏抽棴鏃ュ織绯荤粺锛堝埛鏂扮紦鍐插尯锛屽叧闂枃浠讹級
            void shutdown();

            // 妫€鏌ユ槸鍚﹀凡鍒濆鍖?
            bool is_initialized() const;

            // 璁剧疆鏈€灏忔棩蹇楃骇鍒?
            void set_level(Level level);

            // 鑾峰彇褰撳墠鏃ュ織绾у埆
            Level get_level() const;

            // 鍚敤/绂佺敤鏂囦欢鏃ュ織
            void set_file_logging(bool enable, const std::string& file_path = "");

            // 鏍稿績鏃ュ織鍑芥暟
            void log(Level level, const std::string& message);

            // 渚挎嵎鍑芥暟
            void debug(const std::string& message);
            void info(const std::string& message);
            void warn(const std::string& message);
            void error(const std::string& message);
            void fatal(const std::string& message);

            // 鍒锋柊缂撳啿鍖?
            void flush();

            // 鑾峰彇褰撳墠鏃堕棿瀛楃涓?
            static std::string get_timestamp();

            // 鑾峰彇绾跨▼ID瀛楃涓?
            static std::string get_thread_id();

        private:
            Logger();
            ~Logger();

            // 绂佹鎷疯礉鍜岀Щ鍔?
            Logger(const Logger&)            = delete;
            Logger& operator=(const Logger&) = delete;
            Logger(Logger&&)                 = delete;
            Logger& operator=(Logger&&)      = delete;

            // 鍐呴儴瀹炵幇
            std::unique_ptr<LoggerImpl> impl_;
    };

    // 鍏ㄥ眬渚挎嵎鍑芥暟锛堜繚鎸佸悜鍚庡吋瀹癸級
    inline void debug(const std::string& message) { Logger::instance().debug(message); }
    inline void info(const std::string& message) { Logger::instance().info(message); }
    inline void warn(const std::string& message) { Logger::instance().warn(message); }
    inline void error(const std::string& message) { Logger::instance().error(message); }
    inline void fatal(const std::string& message) { Logger::instance().fatal(message); }

    // 閰嶇疆鍑芥暟
    inline void initialize(const Config& config) { Logger::instance().initialize(config); }
    inline void shutdown() { Logger::instance().shutdown(); }
    inline void set_level(Level level) { Logger::instance().set_level(level); }
    inline void flush() { Logger::instance().flush(); }

    // 瀹忓畾涔夛紙鏀寔鏂囦欢鍚嶅拰琛屽彿锛?
    #define LOG_DEBUG(msg) \
        do { \
            std::ostringstream _oss; \
            _oss << msg; \
            ::vulkan_engine::logger::Logger::instance().debug(_oss.str()); \
        } while(0)

    #define LOG_INFO(msg) \
        do { \
            std::ostringstream _oss; \
            _oss << msg; \
            ::vulkan_engine::logger::Logger::instance().info(_oss.str()); \
        } while(0)

    #define LOG_WARN(msg) \
        do { \
            std::ostringstream _oss; \
            _oss << msg; \
            ::vulkan_engine::logger::Logger::instance().warn(_oss.str()); \
        } while(0)

    #define LOG_ERROR(msg) \
        do { \
            std::ostringstream _oss; \
            _oss << msg; \
            ::vulkan_engine::logger::Logger::instance().error(_oss.str()); \
        } while(0)

    #define LOG_FATAL(msg) \
        do { \
            std::ostringstream _oss; \
            _oss << msg; \
            ::vulkan_engine::logger::Logger::instance().fatal(_oss.str()); \
        } while(0)
} // namespace vulkan_engine::logger
