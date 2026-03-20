#include "engine/rendering/shaders/ShaderManager.hpp"
#include "engine/platform/filesystem/PathUtils.hpp"
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

        // Try to load pre-compiled SPIR-V files
        // Slang naming convention: {name}.vert.spv, {name}.frag.spv
        auto vertex_shader   = load_shader(name + ".vert.spv", ShaderType::Vertex);
        auto fragment_shader = load_shader(name + ".frag.spv", ShaderType::Fragment);

        // If not found, try alternative naming
        if (!vertex_shader.success)
        {
            vertex_shader = load_shader(name + "_vert.spv", ShaderType::Vertex);
        }
        if (!fragment_shader.success)
        {
            fragment_shader = load_shader(name + "_frag.spv", ShaderType::Fragment);
        }

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

        // Check if file has .spv extension (pre-compiled SPIR-V)
        if (full_path.extension() == ".spv")
        {
            result = load_spirv(full_path, type);
        }
        else
        {
            result.error_message = "Unsupported shader file format: " + full_path.extension().string() +
                                   ". Expected .spv (pre-compiled SPIR-V)";
        }

        if (result.success)
        {
            // Store timestamp for hot reload
            if (std::filesystem::exists(full_path))
            {
                impl_->file_timestamps[path.string()] = std::filesystem::last_write_time(full_path);
            }
        }

        return result;
    }

    ShaderCompileResult ShaderManager::load_spirv(const std::filesystem::path& path, ShaderType type)
    {
        ShaderCompileResult result;
        result.success = false;
        result.type    = type;
        result.name    = path.stem().string();

        auto file = core::PathUtils::open_input_file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            result.error_message = "Failed to open SPIR-V file: " + path.string();
            return result;
        }

        auto file_size = static_cast<size_t>(file.tellg());
        if (file_size == 0)
        {
            result.error_message = "SPIR-V file is empty: " + path.string();
            return result;
        }

        if (file_size % 4 != 0)
        {
            result.error_message = "SPIR-V file size is not 4-byte aligned: " + path.string();
            return result;
        }

        result.bytecode.resize(file_size / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(result.bytecode.data()), file_size);
        file.close();

        // Validate SPIR-V magic number
        if (result.bytecode.empty() || result.bytecode[0] != 0x07230203)
        {
            result.error_message = "Invalid SPIR-V file (wrong magic number): " + path.string();
            result.bytecode.clear();
            return result;
        }

        result.success = true;
        return result;
    }

    ShaderCompileResult ShaderManager::compile_shader(const ShaderCompileInfo& info)
    {
        ShaderCompileResult result;
        result.success = false;
        result.name    = info.name;
        result.type    = info.type;
        result.version = info.version;

        // Runtime compilation not supported - use pre-compiled .spv files
        // For Slang shaders, use compile_slang() method or pre-compile using slangc
        result.error_message = "Runtime shader compilation not supported. "
                "Please use pre-compiled .spv files or call compile_slang() for Slang shaders.";

        return result;
    }

    ShaderCompileResult ShaderManager::compile_slang(
        const std::filesystem::path& source_path,
        ShaderType                   type,
        const std::string&           entry_point)
    {
        ShaderCompileResult result;
        result.success = false;
        result.name    = source_path.stem().string();
        result.type    = type;

        // Check if slangc is available
        // For now, just check if the expected output .spv file exists
        std::filesystem::path spv_path = source_path;
        spv_path.replace_extension();

        // Determine output extension based on shader type
        switch (type)
        {
            case ShaderType::Vertex:
                spv_path += ".vert.spv";
                break;
            case ShaderType::Fragment:
                spv_path += ".frag.spv";
                break;
            default:
                result.error_message = "Unsupported shader type for Slang compilation";
                return result;
        }

        // Check if pre-compiled .spv exists
        if (std::filesystem::exists(spv_path))
        {
            result = load_spirv(spv_path, type);
            if (result.success)
            {
                // Store timestamp for hot reload tracking
                impl_->file_timestamps[source_path.string()] = std::filesystem::last_write_time(source_path);
            }
            return result;
        }

        // If .spv doesn't exist, report error with instructions
        result.error_message = "Pre-compiled SPIR-V not found: " + spv_path.string() +
                               "\nPlease compile Slang shader using: "
                               "slangc -target spirv -stage " +
                               (type == ShaderType::Vertex ? "vertex" : "fragment") +
                               " -entry " + entry_point + " " + source_path.string() +
                               " -o " + spv_path.string();
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