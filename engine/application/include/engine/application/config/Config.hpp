#pragma once

#include "engine/application/app/Application.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace vulkan_engine::application
{
    struct Config
    {
        struct WindowConfig
        {
            std::string title      = "Vulkan Engine";
            uint32_t    width      = 1280;
            uint32_t    height     = 720;
            bool        fullscreen = false;
            bool        vsync      = true;
            bool        resizable  = true;
        } window;

        struct RenderingConfig
        {
            bool  enable_validation = true;
            bool  vsync             = true;
            float render_scale      = 1.0f;
            bool  use_render_graph  = true;
        } rendering;

        struct GraphicsConfig
        {
            int  texture_quality = 2; // 0=low, 1=medium, 2=high, 3=ultra
            int  shadow_quality  = 2;
            int  max_fps         = 0; // 0=unlimited
            bool enable_hdr      = false;
            bool enable_aa       = true;
        } graphics;

        struct AudioConfig
        {
            float master_volume = 1.0f;
            float music_volume  = 1.0f;
            float sfx_volume    = 1.0f;
            bool  enabled       = true;
        } audio;

        struct DebugConfig
        {
            bool enable_profiling = false;
            bool enable_logging   = true;
            int  log_level        = 2; // 0=error, 1=warn, 2=info, 3=debug
        } debug;

        // Loading and saving
        static Config load_from_file(const std::filesystem::path& path);
        void          save_to_file(const std::filesystem::path& path) const;

        // Conversion to/from application config
        ApplicationConfig to_application_config() const;
        void              from_application_config(const ApplicationConfig& app_config);

        // Command line parsing
        void merge_from_args(const std::vector<std::string>& args);

        static Config get_default();
    };
} // namespace vulkan_engine::application