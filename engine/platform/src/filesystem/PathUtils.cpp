#include "platform/filesystem/PathUtils.hpp"
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vulkan_engine::core
{
    std::filesystem::path              PathUtils::project_root_;
    std::vector<std::filesystem::path> PathUtils::search_paths_;
    bool                               PathUtils::initialized_ = false;

    void PathUtils::initialize(const std::filesystem::path& executable_path)
    {
        if (initialized_)
        {
            return;
        }

        // Try to find project root
        if (!executable_path.empty())
        {
            project_root_ = find_project_root(executable_path);
        }
        else
        {
            // Try current working directory
            project_root_ = find_project_root(std::filesystem::current_path());
        }

        // Check environment variable
        if (project_root_.empty())
        {
            #ifdef _WIN32
            char*  env_root = nullptr;
            size_t len      = 0;
            _dupenv_s(&env_root, &len, "VULKAN_ENGINE_ROOT");
            if (env_root)
            {
                project_root_ = std::filesystem::path(env_root);
                free(env_root);
            }
            #else
            const char* env_root = std::getenv("VULKAN_ENGINE_ROOT");
            if (env_root)
            {
                project_root_ = std::filesystem::path(env_root);
            }
            #endif
        }

        // Fallback to current directory
        if (project_root_.empty())
        {
            project_root_ = std::filesystem::current_path();
        }

        // Setup default search paths
        search_paths_.clear();
        search_paths_.push_back(project_root_);
        search_paths_.push_back(project_root_ / "..");
        search_paths_.push_back(project_root_ / ".." / "..");

        initialized_ = true;
    }

    std::filesystem::path PathUtils::project_root()
    {
        if (!initialized_)
        {
            initialize();
        }
        return project_root_;
    }

    std::filesystem::path PathUtils::assets_dir()
    {
        return project_root();
    }

    std::filesystem::path PathUtils::shaders_dir()
    {
        return project_root() / "shaders";
    }

    std::filesystem::path PathUtils::materials_dir()
    {
        return project_root() / "materials";
    }

    std::filesystem::path PathUtils::models_dir()
    {
        return project_root() / "model";
    }

    std::filesystem::path PathUtils::textures_dir()
    {
        return project_root() / "Textures";
    }

    std::filesystem::path PathUtils::resolve(const std::string& relative_path)
    {
        return resolve(std::filesystem::path(relative_path));
    }

    std::filesystem::path PathUtils::resolve(const std::filesystem::path& relative_path)
    {
        if (!initialized_)
        {
            initialize();
        }

        // If already absolute, return as-is
        if (relative_path.is_absolute())
        {
            return relative_path;
        }

        // Try relative to project root first
        auto full_path = project_root_ / relative_path;
        if (std::filesystem::exists(full_path))
        {
            return full_path;
        }

        // Try search paths
        for (const auto& search_path : search_paths_)
        {
            full_path = search_path / relative_path;
            if (std::filesystem::exists(full_path))
            {
                return full_path;
            }
        }

        // Return project root relative path even if not found
        // (let the caller handle the error)
        return project_root_ / relative_path;
    }

    std::filesystem::path PathUtils::resolve_asset(const std::string& relative_path)
    {
        return resolve(relative_path);
    }

    std::filesystem::path PathUtils::resolve_shader(const std::string& name)
    {
        // Handle both "shader.vert.spv" and "shader" (auto-add extension)
        std::filesystem::path shader_path = name;
        if (shader_path.extension().empty())
        {
            // Try vertex shader first
            auto vert_path = shaders_dir() / (name + ".vert.spv");
            if (std::filesystem::exists(vert_path))
            {
                return vert_path;
            }
            // Return with .vert.spv extension anyway
            return vert_path;
        }
        return shaders_dir() / name;
    }

    std::filesystem::path PathUtils::resolve_material(const std::string& name)
    {
        std::filesystem::path material_path = name;
        if (material_path.extension().empty())
        {
            material_path += ".json";
        }
        return materials_dir() / material_path;
    }

    std::filesystem::path PathUtils::resolve_model(const std::string& name)
    {
        return models_dir() / name;
    }

    std::filesystem::path PathUtils::resolve_texture(const std::string& name)
    {
        return textures_dir() / name;
    }

    bool PathUtils::exists_in_search_paths(const std::string& path)
    {
        if (!initialized_)
        {
            initialize();
        }

        for (const auto& search_path : search_paths_)
        {
            if (std::filesystem::exists(search_path / path))
            {
                return true;
            }
        }
        return false;
    }

    void PathUtils::add_search_path(const std::filesystem::path& path)
    {
        search_paths_.push_back(path);
    }

    std::vector<std::filesystem::path> PathUtils::search_paths()
    {
        if (!initialized_)
        {
            initialize();
        }
        return search_paths_;
    }

    std::filesystem::path PathUtils::find_project_root(const std::filesystem::path& start_path)
    {
        auto current = start_path;

        // Search upwards for CMakeLists.txt or .git directory
        for (int i = 0; i < 10; ++i) // Limit search depth
        {
            // Check for project markers
            if (std::filesystem::exists(current / "CMakeLists.txt") ||
                std::filesystem::exists(current / ".git") ||
                std::filesystem::exists(current / "shaders") ||
                std::filesystem::exists(current / "materials"))
            {
                return current;
            }

            auto parent = current.parent_path();
            if (parent == current)
            {
                break; // Reached root
            }
            current = parent;
        }

        return {};
    }

    std::ifstream PathUtils::open_input_file(const std::filesystem::path& path, std::ios::openmode mode)
    {
        #ifdef _WIN32
        // On Windows, use wide string path for Unicode support
        return std::ifstream(path.wstring(), mode);
        #else
        return std::ifstream(path, mode);
        #endif
    }

    std::ofstream PathUtils::open_output_file(const std::filesystem::path& path, std::ios::openmode mode)
    {
        #ifdef _WIN32
        // On Windows, use wide string path for Unicode support
        return std::ofstream(path.wstring(), mode);
        #else
        return std::ofstream(path, mode);
        #endif
    }

    std::string PathUtils::to_string(const std::filesystem::path& path)
    {
        #ifdef _WIN32
        // On Windows, convert wide string to UTF-8
        if (path.empty()) return "";
        const std::wstring& wstr = path.wstring();
        if (wstr.empty()) return "";

        int         size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, NULL, NULL);
        return result;
        #else
        return path.string();
        #endif
    }
} // namespace vulkan_engine::core
