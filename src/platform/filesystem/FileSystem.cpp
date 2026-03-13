#include "platform/filesystem/FileSystem.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir _mkdir
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace vulkan_engine::filesystem
{
    bool FileSystem::exists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    bool FileSystem::is_file(const std::filesystem::path& path)
    {
        return std::filesystem::is_regular_file(path);
    }

    bool FileSystem::is_directory(const std::filesystem::path& path)
    {
        return std::filesystem::is_directory(path);
    }

    std::vector<uint8_t> FileSystem::read_binary_file(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + path.string());
        }

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        return buffer;
    }

    std::string FileSystem::read_text_file(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + path.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void FileSystem::write_binary_file(const std::filesystem::path& path, const std::vector<uint8_t>& data)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to create file: " + path.string());
        }

        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    void FileSystem::write_text_file(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to create file: " + path.string());
        }

        file << content;
    }

    void FileSystem::create_directory(const std::filesystem::path& path)
    {
        std::filesystem::create_directories(path);
    }

    void FileSystem::remove(const std::filesystem::path& path)
    {
        std::filesystem::remove_all(path);
    }

    void FileSystem::copy(const std::filesystem::path& source, const std::filesystem::path& destination)
    {
        std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
    }

    void FileSystem::move(const std::filesystem::path& source, const std::filesystem::path& destination)
    {
        std::filesystem::rename(source, destination);
    }

    std::vector<std::filesystem::path> FileSystem::list_directory(const std::filesystem::path& path, bool recursive)
    {
        std::vector<std::filesystem::path> results;

        if (recursive)
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
            {
                results.push_back(entry.path());
            }
        }
        else
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                results.push_back(entry.path());
            }
        }

        return results;
    }

    std::filesystem::path FileSystem::get_executable_path()
    {
        #ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path();
        #else
        char    buffer[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1)
        {
            buffer[len] = '\0';
            return std::filesystem::path(buffer).parent_path();
        }
        return std::filesystem::current_path();
        #endif
    }

    std::filesystem::path FileSystem::get_working_directory()
    {
        return std::filesystem::current_path();
    }

    void FileSystem::set_working_directory(const std::filesystem::path& path)
    {
        std::filesystem::current_path(path);
    }

    std::filesystem::path FileSystem::get_user_directory()
    {
        #ifdef _WIN32
        const char* user_profile = std::getenv("USERPROFILE");
        if (user_profile)
        {
            return std::filesystem::path(user_profile);
        }
        return get_working_directory();
        #else
        const char* home = std::getenv("HOME");
        if (home)
        {
            return std::filesystem::path(home);
        }
        return get_working_directory();
        #endif
    }

    std::filesystem::path FileSystem::get_temp_directory()
    {
        return std::filesystem::temp_directory_path();
    }

    std::filesystem::path FileSystem::join(const std::filesystem::path& base, const std::filesystem::path& relative)
    {
        return base / relative;
    }

    std::filesystem::path FileSystem::get_absolute_path(const std::filesystem::path& path)
    {
        return std::filesystem::absolute(path);
    }

    std::filesystem::path FileSystem::get_relative_path(const std::filesystem::path& path, const std::filesystem::path& base)
    {
        return std::filesystem::relative(path, base);
    }

    std::string FileSystem::get_filename(const std::filesystem::path& path)
    {
        return path.filename().string();
    }

    std::string FileSystem::get_extension(const std::filesystem::path& path)
    {
        return path.extension().string();
    }

    std::string FileSystem::get_stem(const std::filesystem::path& path)
    {
        return path.stem().string();
    }

    std::filesystem::path FileSystem::get_parent_directory(const std::filesystem::path& path)
    {
        return path.parent_path();
    }

    FileInfo FileSystem::get_file_info(const std::filesystem::path& path)
    {
        FileInfo info;
        info.path      = path;
        info.name      = path.filename().string();
        info.extension = path.extension().string();
        info.exists    = std::filesystem::exists(path);

        if (info.exists)
        {
            auto status        = std::filesystem::status(path);
            info.is_directory  = std::filesystem::is_directory(status);
            info.is_file       = std::filesystem::is_regular_file(status);
            info.is_symlink    = std::filesystem::is_symlink(status);
            info.is_readable   = true; // Simplified - actual check would be more complex
            info.is_writable   = true;
            info.is_executable = !info.is_directory;

            if (info.is_file)
            {
                info.size = static_cast<uint64_t>(std::filesystem::file_size(path));
            }

            info.creation_time     = std::filesystem::last_write_time(path);
            info.modification_time = info.creation_time;
            info.access_time       = info.creation_time;
        }

        return info;
    }
} // namespace vulkan_engine::filesystem