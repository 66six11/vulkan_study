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
    // 日志级别
    enum class Level
    {
        Debug = 0,
        Info  = 1,
        Warn  = 2,
        Error = 3,
        Fatal = 4
    };

    // 日志输出目标
    enum class OutputTarget
    {
        Console = 1 << 0,
        File    = 1 << 1,
        All     = Console | File
    };

    // 日志配置
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

    // 内部实现前向声明
    class LoggerImpl;

    /**
     * @brief 线程安全的日志系统
     *
     * 特性：
     * - 单例模式，全局唯一实例
     * - 线程安全，支持多线程并发写入
     * - 支持控制台和文件双输出
     * - 支持日志级别过滤
     * - Windows 下正确处理 Unicode 和控制台编码
     * - 任何情况下都能输出（即使文件打开失败，也会尝试控制台）
     */
    class Logger
    {
        public:
            // 获取单例实例
            static Logger& instance();

            // 初始化（可选，如果不调用会使用默认配置）
            void initialize(const Config& config);

            // 关闭日志系统（刷新缓冲区，关闭文件）
            void shutdown();

            // 检查是否已初始化
            bool is_initialized() const;

            // 设置最小日志级别
            void set_level(Level level);

            // 获取当前日志级别
            Level get_level() const;

            // 启用/禁用文件日志
            void set_file_logging(bool enable, const std::string& file_path = "");

            // 核心日志函数
            void log(Level level, const std::string& message);

            // 便捷函数
            void debug(const std::string& message);
            void info(const std::string& message);
            void warn(const std::string& message);
            void error(const std::string& message);
            void fatal(const std::string& message);

            // 刷新缓冲区
            void flush();

            // 获取当前时间字符串
            static std::string get_timestamp();

            // 获取线程ID字符串
            static std::string get_thread_id();

        private:
            Logger();
            ~Logger();

            // 禁止拷贝和移动
            Logger(const Logger&)            = delete;
            Logger& operator=(const Logger&) = delete;
            Logger(Logger&&)                 = delete;
            Logger& operator=(Logger&&)      = delete;

            // 内部实现
            std::unique_ptr<LoggerImpl> impl_;
    };

    // 全局便捷函数（保持向后兼容）
    inline void debug(const std::string& message) { Logger::instance().debug(message); }
    inline void info(const std::string& message) { Logger::instance().info(message); }
    inline void warn(const std::string& message) { Logger::instance().warn(message); }
    inline void error(const std::string& message) { Logger::instance().error(message); }
    inline void fatal(const std::string& message) { Logger::instance().fatal(message); }

    // 配置函数
    inline void initialize(const Config& config) { Logger::instance().initialize(config); }
    inline void shutdown() { Logger::instance().shutdown(); }
    inline void set_level(Level level) { Logger::instance().set_level(level); }
    inline void flush() { Logger::instance().flush(); }

    // 宏定义（支持文件名和行号）
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
