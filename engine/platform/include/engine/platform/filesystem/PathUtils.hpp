#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace vulkan_engine::core
{
    // Utility class for path resolution and management
    // Provides project-relative path resolution to avoid hardcoded absolute paths
    class PathUtils
    {
        public:
            // Initialize with executable path or specified root
            static void initialize(const std::filesystem::path& executable_path = {});

            // Get project root directory
            static std::filesystem::path project_root();

            // Get asset directories
            static std::filesystem::path assets_dir();
            static std::filesystem::path shaders_dir();
            static std::filesystem::path materials_dir();
            static std::filesystem::path models_dir();
            static std::filesystem::path textures_dir();

            // Resolve a path relative to project root
            // If path is already absolute, returns it as-is
            static std::filesystem::path resolve(const std::string& relative_path);
            static std::filesystem::path resolve(const std::filesystem::path& relative_path);

            // Resolve asset paths
            static std::filesystem::path resolve_asset(const std::string& relative_path);
            static std::filesystem::path resolve_shader(const std::string& name);
            static std::filesystem::path resolve_material(const std::string& name);
            static std::filesystem::path resolve_model(const std::string& name);
            static std::filesystem::path resolve_texture(const std::string& name);

            // Check if file exists in search paths
            static bool exists_in_search_paths(const std::string& path);

            // Add custom search path
            static void add_search_path(const std::filesystem::path& path);

            // Get all search paths
            static std::vector<std::filesystem::path> search_paths();

            // Cross-platform file stream opening (handles UTF-8 paths on Windows)

            static std::ifstream open_input_file(const std::filesystem::path& path, std::ios::openmode mode = std::ios::binary);

            static std::ofstream open_output_file(const std::filesystem::path& path, std::ios::openmode mode = std::ios::binary);


            // Convert path to string for logging (handles Unicode correctly)

            static std::string to_string(const std::filesystem::path& path);

        private:
            static std::filesystem::path              project_root_;
            static std::vector<std::filesystem::path> search_paths_;
            static bool                               initialized_;

            // Try to find project root from executable path
            static std::filesystem::path find_project_root(const std::filesystem::path& start_path);
    };
} // namespace vulkan_engine::core