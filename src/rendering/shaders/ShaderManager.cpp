#include "rendering/shaders/ShaderManager.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace vulkan_engine::rendering
{
    struct ShaderManager::Impl
    {
        std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> programs;
        std::unordered_map<std::string, ShaderCompileResult>            compiled_shaders;

        std::filesystem::path shader_directory;
        std::string           cache_directory;
        bool                  enable_cache      = true;
        bool                  enable_hot_reload = false;

        // Hot reload tracking
        std::unordered_map<std::string, std::filesystem::file_time_type> file_timestamps;
    };

    ShaderManager::ShaderManager()
        : impl_(std::make_unique<Impl>())
    {
    }

    ShaderManager::~ShaderManager() = default;

    void ShaderManager::initialize(const std::filesystem::path& shader_dir)
    {
        impl_->shader_directory = shader_dir;
    }

    void ShaderManager::shutdown()
    {
        impl_->programs.clear();
        impl_->compiled_shaders.clear();
    }

    std::shared_ptr<ShaderProgram> ShaderManager::load_shader_program(const std::string& name)
    {
        // Check if already loaded
        auto it = impl_->programs.find(name);
        if (it != impl_->programs.end())
        {
            return it->second;
        }

        // Load vertex and fragment shaders
        auto vertex_shader   = load_shader(name + ".vert", ShaderType::Vertex);
        auto fragment_shader = load_shader(name + ".frag", ShaderType::Fragment);

        if (!vertex_shader.success || !fragment_shader.success)
        {
            return nullptr;
        }

        // Create program
        auto program             = std::make_shared<ShaderProgram>();
        program->name            = name;
        program->vertex_shader   = std::move(vertex_shader);
        program->fragment_shader = std::move(fragment_shader);

        impl_->programs[name] = program;
        return program;
    }

    ShaderCompileResult ShaderManager::load_shader(const std::filesystem::path& path, ShaderType type)
    {
        ShaderCompileResult result;
        result.success = false;

        std::filesystem::path full_path = impl_->shader_directory / path;

        // Read shader source
        std::ifstream file(full_path);
        if (!file.is_open())
        {
            result.error_message = "Failed to open shader file: " + path.string();
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        // Compile shader
        ShaderCompileInfo info;
        info.name    = path.stem().string();
        info.type    = type;
        info.source  = buffer.str();
        info.version = 450; // SPIR-V target version

        result = compile_shader(info);

        if (result.success)
        {
            // Store timestamp for hot reload
            impl_->file_timestamps[path.string()] = std::filesystem::last_write_time(full_path);
        }

        return result;
    }

    ShaderCompileResult ShaderManager::compile_shader(const ShaderCompileInfo& info)
    {
        ShaderCompileResult result;
        result.success = false;

        // Placeholder: Actual SPIR-V compilation would go here
        // For now, just store the source
        result.name    = info.name;
        result.type    = info.type;
        result.version = info.version;

        // In a real implementation, this would:
        // 1. Check cache
        // 2. Call shaderc or similar to compile GLSL/HLSL to SPIR-V
        // 3. Store the compiled bytecode

        // Mark as successful for now (actual bytecode would be empty)
        result.success  = true;
        result.bytecode = std::vector<uint32_t>(); // Placeholder

        return result;
    }

    void ShaderManager::reload_shader(const std::string& name)
    {
        auto it = impl_->programs.find(name);
        if (it != impl_->programs.end())
        {
            impl_->programs.erase(it);
            load_shader_program(name);
        }
    }

    void ShaderManager::reload_all_shaders()
    {
        std::vector<std::string> names;
        for (const auto& [name, _] : impl_->programs)
        {
            names.push_back(name);
        }

        impl_->programs.clear();

        for (const auto& name : names)
        {
            load_shader_program(name);
        }
    }

    void ShaderManager::update_hot_reloads()
    {
        if (!impl_->enable_hot_reload)
        {
            return;
        }

        for (auto& [path, last_time] : impl_->file_timestamps)
        {
            std::filesystem::path full_path = impl_->shader_directory / path;
            if (!std::filesystem::exists(full_path))
            {
                continue;
            }

            auto current_time = std::filesystem::last_write_time(full_path);
            if (current_time > last_time)
            {
                // File has been modified, reload
                last_time = current_time;

                std::string shader_name = std::filesystem::path(path).stem().string();
                reload_shader(shader_name);
            }
        }
    }

    void ShaderManager::set_shader_directory(const std::filesystem::path& directory)
    {
        impl_->shader_directory = directory;
    }

    void ShaderManager::set_cache_directory(const std::string& directory)
    {
        impl_->cache_directory = directory;
    }

    void ShaderManager::enable_caching(bool enable)
    {
        impl_->enable_cache = enable;
    }

    void ShaderManager::enable_hot_reloading(bool enable)
    {
        impl_->enable_hot_reload = enable;
    }

    std::shared_ptr<ShaderProgram> ShaderManager::get_shader_program(const std::string& name)
    {
        auto it = impl_->programs.find(name);
        return (it != impl_->programs.end()) ? it->second : nullptr;
    }

    bool ShaderManager::is_shader_loaded(const std::string& name) const
    {
        return impl_->programs.find(name) != impl_->programs.end();
    }

    std::vector<std::string> ShaderManager::get_loaded_shader_names() const
    {
        std::vector<std::string> names;
        for (const auto& [name, _] : impl_->programs)
        {
            names.push_back(name);
        }
        return names;
    }
} // namespace vulkan_engine::rendering