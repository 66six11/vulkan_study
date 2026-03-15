#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vulkan_engine::rendering
{
    enum class ShaderType
    {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    struct ShaderCompileInfo
    {
        std::string                                  name;
        ShaderType                                   type;
        std::string                                  source;
        std::string                                  entry_point = "main";
        uint32_t                                     version     = 450;
        std::vector<std::filesystem::path>           include_paths;
        std::unordered_map<std::string, std::string> defines;
    };

    struct ShaderCompileResult
    {
        bool                     success = false;
        std::string              name;
        ShaderType               type;
        uint32_t                 version = 0;
        std::vector<uint32_t>    bytecode;
        std::string              error_message;
        std::vector<std::string> warnings;
    };

    struct ShaderProgram
    {
        std::string         name;
        ShaderCompileResult vertex_shader;
        ShaderCompileResult fragment_shader;
        ShaderCompileResult geometry_shader;
        ShaderCompileResult compute_shader;
        // Vulkan pipeline layout, descriptor set layout, etc.
    };

    class ShaderManager
    {
        public:
            ShaderManager();
            ~ShaderManager();

            void initialize(const std::filesystem::path& shader_dir);
            void shutdown();

            // Load shaders
            std::shared_ptr<ShaderProgram> load_shader_program(const std::string& name);
            ShaderCompileResult            load_shader(const std::filesystem::path& path, ShaderType type);

            // Compile shaders
            ShaderCompileResult compile_shader(const ShaderCompileInfo& info);

            // Load pre-compiled SPIR-V
            ShaderCompileResult load_spirv(const std::filesystem::path& path, ShaderType type);

            // Slang compilation support (requires slangc in PATH)
            ShaderCompileResult compile_slang(
                const std::filesystem::path& source_path,
                ShaderType type,
                const std::string& entry_point = "main");

            // Reload
            void reload_shader(const std::string& name);
            void reload_all_shaders();
            void update_hot_reloads();

            // Configuration
            void set_shader_directory(const std::filesystem::path& directory);
            void set_cache_directory(const std::string& directory);
            void enable_caching(bool enable);
            void enable_hot_reloading(bool enable);

            // Query
            std::shared_ptr<ShaderProgram> get_shader_program(const std::string& name);
            bool                           is_shader_loaded(const std::string& name) const;
            std::vector<std::string>       get_loaded_shader_names() const;

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace vulkan_engine::rendering