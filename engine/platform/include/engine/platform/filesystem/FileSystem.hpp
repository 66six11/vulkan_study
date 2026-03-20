#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

namespace vulkan_engine::filesystem
{
    struct FileInfo
    {
        std::filesystem::path           path;
        std::string                     name;
        std::string                     extension;
        bool                            exists        = false;
        bool                            is_file       = false;
        bool                            is_directory  = false;
        bool                            is_symlink    = false;
        bool                            is_readable   = false;
        bool                            is_writable   = false;
        bool                            is_executable = false;
        uint64_t                        size          = 0;
        std::filesystem::file_time_type creation_time;
        std::filesystem::file_time_type modification_time;
        std::filesystem::file_time_type access_time;
    };

    class FileSystem
    {
        public:
            // File operations
            static bool exists(const std::filesystem::path& path);
            static bool is_file(const std::filesystem::path& path);
            static bool is_directory(const std::filesystem::path& path);

            static std::vector<uint8_t> read_binary_file(const std::filesystem::path& path);
            static std::string          read_text_file(const std::filesystem::path& path);

            static void write_binary_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);
            static void write_text_file(const std::filesystem::path& path, const std::string& content);

            // Directory operations
            static void create_directory(const std::filesystem::path& path);
            static void remove(const std::filesystem::path& path);
            static void copy(const std::filesystem::path& source, const std::filesystem::path& destination);
            static void move(const std::filesystem::path& source, const std::filesystem::path& destination);

            static std::vector<std::filesystem::path> list_directory(
                const std::filesystem::path& path,
                bool                         recursive = false);

            // Path utilities
            static std::filesystem::path get_executable_path();
            static std::filesystem::path get_working_directory();
            static void                  set_working_directory(const std::filesystem::path& path);
            static std::filesystem::path get_user_directory();
            static std::filesystem::path get_temp_directory();

            static std::filesystem::path join(const std::filesystem::path& base, const std::filesystem::path& relative);
            static std::filesystem::path get_absolute_path(const std::filesystem::path& path);
            static std::filesystem::path get_relative_path(
                const std::filesystem::path& path,
                const std::filesystem::path& base);

            // File info
            static std::string           get_filename(const std::filesystem::path& path);
            static std::string           get_extension(const std::filesystem::path& path);
            static std::string           get_stem(const std::filesystem::path& path);
            static std::filesystem::path get_parent_directory(const std::filesystem::path& path);
            static FileInfo              get_file_info(const std::filesystem::path& path);
    };
} // namespace vulkan_engine::filesystem