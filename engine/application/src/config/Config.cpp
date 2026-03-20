#include "application/config/Config.hpp"
#include <fstream>
#include <iostream>

namespace vulkan_engine::application
{
    Config Config::load_from_file(const std::filesystem::path& path)
    {
        Config config;

        if (!std::filesystem::exists(path))
        {
            // Return default config if file doesn't exist
            return config;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return config;
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            // Simple key=value parsing
            auto pos = line.find('=');
            if (pos == std::string::npos)
            {
                continue;
            }

            std::string key   = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            auto trim = [](std::string& s)
            {
                size_t start = s.find_first_not_of(" \t");
                size_t end   = s.find_last_not_of(" \t");
                if (start != std::string::npos)
                {
                    s = s.substr(start, end - start + 1);
                }
            };

            trim(key);
            trim(value);

            // Parse values
            if (key == "window.width")
            {
                config.window.width = std::stoi(value);
            }
            else if (key == "window.height")
            {
                config.window.height = std::stoi(value);
            }
            else if (key == "window.title")
            {
                config.window.title = value;
            }
            else if (key == "window.fullscreen")
            {
                config.window.fullscreen = (value == "true" || value == "1");
            }
            else if (key == "window.vsync")
            {
                config.window.vsync = (value == "true" || value == "1");
            }
            else if (key == "rendering.validation")
            {
                config.rendering.enable_validation = (value == "true" || value == "1");
            }
            else if (key == "rendering.vsync")
            {
                config.rendering.vsync = (value == "true" || value == "1");
            }
            else if (key == "rendering.render_scale")
            {
                config.rendering.render_scale = std::stof(value);
            }
            else if (key == "graphics.texture_quality")
            {
                config.graphics.texture_quality = std::stoi(value);
            }
            else if (key == "graphics.shadow_quality")
            {
                config.graphics.shadow_quality = std::stoi(value);
            }
            else if (key == "graphics.max_fps")
            {
                config.graphics.max_fps = std::stoi(value);
            }
        }

        return config;
    }

    void Config::save_to_file(const std::filesystem::path& path) const
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to create config file: " << path << std::endl;
            return;
        }

        file << "# Vulkan Engine Configuration\n";
        file << "# Generated automatically\n\n";

        file << "[Window]\n";
        file << "window.width=" << window.width << "\n";
        file << "window.height=" << window.height << "\n";
        file << "window.title=" << window.title << "\n";
        file << "window.fullscreen=" << (window.fullscreen ? "true" : "false") << "\n";
        file << "window.vsync=" << (window.vsync ? "true" : "false") << "\n";
        file << "window.resizable=" << (window.resizable ? "true" : "false") << "\n\n";

        file << "[Rendering]\n";
        file << "rendering.validation=" << (rendering.enable_validation ? "true" : "false") << "\n";
        file << "rendering.vsync=" << (rendering.vsync ? "true" : "false") << "\n";
        file << "rendering.render_scale=" << rendering.render_scale << "\n";
        file << "rendering.use_render_graph=" << (rendering.use_render_graph ? "true" : "false") << "\n\n";

        file << "[Graphics]\n";
        file << "graphics.texture_quality=" << graphics.texture_quality << "\n";
        file << "graphics.shadow_quality=" << graphics.shadow_quality << "\n";
        file << "graphics.max_fps=" << graphics.max_fps << "\n";
        file << "graphics.enable_hdr=" << (graphics.enable_hdr ? "true" : "false") << "\n";
        file << "graphics.enable_aa=" << (graphics.enable_aa ? "true" : "false") << "\n";
    }

    ApplicationConfig Config::to_application_config() const
    {
        ApplicationConfig app_config;
        app_config.title             = window.title;
        app_config.width             = window.width;
        app_config.height            = window.height;
        app_config.fullscreen        = window.fullscreen;
        app_config.vsync             = window.vsync;
        app_config.resizable         = window.resizable;
        app_config.enable_validation = rendering.enable_validation;
        app_config.enable_profiling  = false;
        app_config.use_render_graph  = rendering.use_render_graph;
        return app_config;
    }

    void Config::from_application_config(const ApplicationConfig& app_config)
    {
        window.title                = app_config.title;
        window.width                = app_config.width;
        window.height               = app_config.height;
        window.fullscreen           = app_config.fullscreen;
        window.vsync                = app_config.vsync;
        window.resizable            = app_config.resizable;
        rendering.enable_validation = app_config.enable_validation;
        rendering.use_render_graph  = app_config.use_render_graph;
    }

    void Config::merge_from_args(const std::vector<std::string>& args)
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            const std::string& arg = args[i];

            if (arg == "--width" && i + 1 < args.size())
            {
                window.width = std::stoi(args[++i]);
            }
            else if (arg == "--height" && i + 1 < args.size())
            {
                window.height = std::stoi(args[++i]);
            }
            else if (arg == "--fullscreen")
            {
                window.fullscreen = true;
            }
            else if (arg == "--windowed")
            {
                window.fullscreen = false;
            }
            else if (arg == "--no-vsync")
            {
                window.vsync = false;
            }
            else if (arg == "--validation")
            {
                rendering.enable_validation = true;
            }
            else if (arg == "--no-validation")
            {
                rendering.enable_validation = false;
            }
        }
    }

    Config Config::get_default()
    {
        return Config{};
    }
} // namespace vulkan_engine::application