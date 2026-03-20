#include "core/utils/Logger.hpp"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

namespace vulkan_engine::logger
{
    // 日志级别转字符串
    static const char* level_to_string(Level level)
    {
        switch (level)
        {
            case Level::Debug: return "DEBUG";
            case Level::Info: return "INFO";
            case Level::Warn: return "WARN";
            case Level::Error: return "ERROR";
            case Level::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    // 日志级别转控制台颜色（Windows）
    #ifdef _WIN32
    static WORD level_to_color(Level level)
    {
        switch (level)
        {
            case Level::Debug: return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Cyan
            case Level::Info: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;                    // Green
            case Level::Warn: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;   // Yellow
            case Level::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY;                     // Red
            case Level::Fatal: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;   // Magenta
            default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;                 // White
        }
    }
    #endif

    // 内部实现类
    class LoggerImpl
    {
        public:
            LoggerImpl() = default;
            ~LoggerImpl() { shutdown(); }

            void initialize(const Config& config)
            {
                std::lock_guard<std::mutex> lock(mutex_);

                config_      = config;
                initialized_ = true;

                // 初始化控制台输出
                if (static_cast<int>(config.target) & static_cast<int>(OutputTarget::Console))
                {
                    initialize_console();
                }

                // 初始化文件输出
                if (static_cast<int>(config.target) & static_cast<int>(OutputTarget::File))
                {
                    open_log_file(config.log_file_path);
                }
            }

            void shutdown()
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (file_stream_.is_open())
                {
                    file_stream_.flush();
                    file_stream_.close();
                }

                initialized_ = false;
            }

            void log(Level level, const std::string& message)
            {
                // 快速路径：检查日志级别（无锁）
                if (static_cast<int>(level) < static_cast<int>(config_.min_level))
                    return;

                std::lock_guard<std::mutex> lock(mutex_);

                if (!initialized_)
                {
                    // 未初始化时，初始化默认配置
                    initialize_default();
                }

                // 格式化日志消息
                std::string formatted = format_message(level, message);

                // 输出到控制台
                if (static_cast<int>(config_.target) & static_cast<int>(OutputTarget::Console))
                {
                    write_to_console(level, formatted);
                }

                // 输出到文件
                if (file_stream_.is_open())
                {
                    file_stream_ << formatted << std::endl;
                    if (config_.flush_on_every_log)
                    {
                        file_stream_.flush();
                    }
                }
            }

            void set_level(Level level)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                config_.min_level = level;
            }

            Level get_level()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return config_.min_level;
            }

            void flush()
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (file_stream_.is_open())
                {
                    file_stream_.flush();
                }

                #ifdef _WIN32
                if (stdout_handle_ != INVALID_HANDLE_VALUE)
                {
                    FlushFileBuffers(stdout_handle_);
                }
                #else
                std::cout.flush();
                std::cerr.flush();
                #endif
            }

            bool is_initialized() const
            {
                return initialized_;
            }

            void set_file_logging(bool enable, const std::string& file_path)
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (enable)
                {
                    open_log_file(file_path);
                    config_.target = static_cast<OutputTarget>(
                        static_cast<int>(config_.target) | static_cast<int>(OutputTarget::File));
                }
                else
                {
                    if (file_stream_.is_open())
                    {
                        file_stream_.flush();
                        file_stream_.close();
                    }
                    config_.target = static_cast<OutputTarget>(
                        static_cast<int>(config_.target) & ~static_cast<int>(OutputTarget::File));
                }
            }

        private:
            void initialize_default()
            {
                initialize_console();
                initialized_ = true;
            }

            void initialize_console()
            {
                #ifdef _WIN32
                // 获取标准输出句柄
                stdout_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
                stderr_handle_ = GetStdHandle(STD_ERROR_HANDLE);

                // 尝试设置控制台代码页为 UTF-8
                SetConsoleOutputCP(CP_UTF8);

                // 如果 stdout 被重定向到文件，尝试启用虚拟终端处理
                DWORD mode = 0;
                if (GetConsoleMode(stdout_handle_, &mode))
                {
                    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(stdout_handle_, mode);
                }

                // 设置控制台字体支持 Unicode
                CONSOLE_FONT_INFOEX font_info = {sizeof(CONSOLE_FONT_INFOEX)};
                if (GetCurrentConsoleFontEx(stdout_handle_, FALSE, &font_info))
                {
                    // 使用 Consolas 或 Lucida Console 字体
                    wcscpy_s(font_info.FaceName, L"Consolas");
                    SetCurrentConsoleFontEx(stdout_handle_, FALSE, &font_info);
                }
                #endif
            }

            void open_log_file(const std::string& file_path)
            {
                std::string path = file_path;
                if (path.empty())
                {
                    // 生成默认日志文件名
                    auto    now  = std::chrono::system_clock::now();
                    auto    time = std::chrono::system_clock::to_time_t(now);
                    std::tm local_time;
                    #ifdef _WIN32
                    localtime_s(&local_time, &time);
                    #else
                    localtime_r(&time, &local_time);
                    #endif

                    std::ostringstream oss;
                    oss << "logs/";
                    oss << std::put_time(&local_time, "%Y-%m-%d_%H-%M-%S");
                    oss << ".log";
                    path = oss.str();
                }

                // 确保目录存在
                #ifdef _WIN32
                std::string dir = path.substr(0, path.find_last_of("/\\"));
                if (!dir.empty())
                {
                    CreateDirectoryA(dir.c_str(), nullptr);
                }
                #else
                std::string dir = path.substr(0, path.find_last_of('/'));
                if (!dir.empty())
                {
                    std::system(("mkdir -p " + dir).c_str());
                }
                #endif

                file_stream_.open(path, std::ios::out | std::ios::app);
                if (!file_stream_.is_open())
                {
                    // 文件打开失败，输出错误到控制台
                    write_to_console(Level::Error, "[ERROR] Failed to open log file: " + path);
                }
            }

            std::string format_message(Level level, const std::string& message)
            {
                std::ostringstream oss;

                // 时间戳
                if (config_.include_timestamp)
                {
                    oss << "[" << Logger::get_timestamp() << "] ";
                }

                // 日志级别
                if (config_.include_level)
                {
                    oss << "[" << std::setw(5) << std::left << level_to_string(level) << "] ";
                }

                // 线程ID
                if (config_.include_thread_id)
                {
                    oss << "[" << Logger::get_thread_id() << "] ";
                }

                // 消息内容
                oss << message;

                return oss.str();
            }

            void write_to_console(Level level, const std::string& message)
            {
                #ifdef _WIN32
                // 使用 Windows API 写入控制台，支持 Unicode
                HANDLE handle = (level >= Level::Error) ? stderr_handle_ : stdout_handle_;

                if (handle != INVALID_HANDLE_VALUE)
                {
                    // 设置颜色
                    CONSOLE_SCREEN_BUFFER_INFO console_info;
                    WORD                       original_attributes = 0;
                    bool                       has_console_info    = GetConsoleScreenBufferInfo(handle, &console_info);
                    if (has_console_info)
                    {
                        original_attributes = console_info.wAttributes;
                        SetConsoleTextAttribute(handle, level_to_color(level));
                    }

                    // 转换为宽字符并写入
                    std::wstring wmessage = utf8_to_wstring(message + "\n");
                    DWORD        written  = 0;
                    WriteConsoleW(handle, wmessage.c_str(), static_cast<DWORD>(wmessage.size()), &written, nullptr);

                    // 恢复原始颜色
                    if (has_console_info)
                    {
                        SetConsoleTextAttribute(handle, original_attributes);
                    }
                }
                else
                {
                    // 控制台句柄无效，使用标准输出（可能被重定向到文件）
                    if (level >= Level::Error)
                    {
                        std::cerr << message << std::endl;
                    }
                    else
                    {
                        std::cout << message << std::endl;
                    }
                }
                #else
                // Linux/Mac 使用 ANSI 颜色代码
                const char* color_code = "";
                switch (level)
                {
                    case Level::Debug: color_code = "\033[36m";
                        break; // Cyan
                    case Level::Info: color_code = "\033[32m";
                        break; // Green
                    case Level::Warn: color_code = "\033[33m";
                        break; // Yellow
                    case Level::Error: color_code = "\033[31m";
                        break; // Red
                    case Level::Fatal: color_code = "\033[35m";
                        break; // Magenta
                }
                const char* reset_code = "\033[0m";

                if (level >= Level::Error)
                {
                    std::cerr << color_code << message << reset_code << std::endl;
                }
                else
                {
                    std::cout << color_code << message << reset_code << std::endl;
                }
                #endif
            }

            std::wstring utf8_to_wstring(const std::string& str)
            {
                if (str.empty()) return std::wstring();

                #ifdef _WIN32
                int size_needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), (int)str.size(), nullptr, 0);
                if (size_needed <= 0) return std::wstring(L"[invalid utf-8]");

                std::wstring wstr(size_needed, 0);
                MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
                return wstr;
                #else
                // Linux/Mac 使用标准转换
                std::wstring wstr;
                wstr.reserve(str.size());
                for (char c : str)
                {
                    wstr += static_cast<wchar_t>(static_cast<unsigned char>(c));
                }
                return wstr;
                #endif
            }

        private:
            Config            config_;
            std::atomic<bool> initialized_{false};
            std::mutex        mutex_;
            std::ofstream     file_stream_;

            #ifdef _WIN32
            HANDLE stdout_handle_ = INVALID_HANDLE_VALUE;
            HANDLE stderr_handle_ = INVALID_HANDLE_VALUE;
            #endif
    };

    // Logger 类实现

    Logger& Logger::instance()
    {
        static Logger instance;
        return instance;
    }

    Logger::Logger() : impl_(std::make_unique<LoggerImpl>())
    {
    }

    Logger::~Logger() = default;

    void Logger::initialize(const Config& config)
    {
        impl_->initialize(config);
    }

    void Logger::shutdown()
    {
        impl_->shutdown();
    }

    bool Logger::is_initialized() const
    {
        return impl_->is_initialized();
    }

    void Logger::set_level(Level level)
    {
        impl_->set_level(level);
    }

    Level Logger::get_level() const
    {
        return impl_->get_level();
    }

    void Logger::set_file_logging(bool enable, const std::string& file_path)
    {
        impl_->set_file_logging(enable, file_path);
    }

    void Logger::log(Level level, const std::string& message)
    {
        impl_->log(level, message);
    }

    void Logger::debug(const std::string& message)
    {
        log(Level::Debug, message);
    }

    void Logger::info(const std::string& message)
    {
        log(Level::Info, message);
    }

    void Logger::warn(const std::string& message)
    {
        log(Level::Warn, message);
    }

    void Logger::error(const std::string& message)
    {
        log(Level::Error, message);
    }

    void Logger::fatal(const std::string& message)
    {
        log(Level::Fatal, message);
    }

    void Logger::flush()
    {
        impl_->flush();
    }

    std::string Logger::get_timestamp()
    {
        auto now  = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm local_time;
        #ifdef _WIN32
        localtime_s(&local_time, &time);
        #else
        localtime_r(&time, &local_time);
        #endif

        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

        // 添加毫秒
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                        now.time_since_epoch()) % 1000;
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }

    std::string Logger::get_thread_id()
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }
} // namespace vulkan_engine::logger