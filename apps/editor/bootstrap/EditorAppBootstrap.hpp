#pragma once

// 闇€瑕佸寘鍚?Application 澶存枃浠舵潵鑾峰彇鍩虹被瀹氫箟
// 娉ㄦ剰锛氳繖鏄?apps/editor 鐨勭壒鏉冿紝瀹冧綔涓哄簲鐢ㄧ▼搴忓叆鍙ｅ彲浠ヨ闂紩鎿庡悇灞?
#include "engine/application/app/Application.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace vulkan_engine::rendering
{
    class ComposedRenderer;
}

namespace editor::bootstrap
{
    // 浣跨敤寮曟搸鍛藉悕绌洪棿
    using namespace vulkan_engine;

    // Forward declarations
    class EditorApplication;

    // Editor App 閰嶇疆缁撴瀯
    struct EditorAppConfig
    {
        std::string title             = "Vulkan Engine - Editor";
        uint32_t    width             = 1600;
        uint32_t    height            = 900;
        bool        vsync             = true;
        bool        enable_validation = true;
        bool        enable_profiling  = true;

        // 浠庡懡浠よ鍙傛暟瑙ｆ瀽閰嶇疆
        static EditorAppConfig parse(int argc, char* argv[]);
    };

    // Editor Application 宸ュ巶鍑芥暟
    std::unique_ptr<EditorApplication> create_editor_app(const EditorAppConfig& config);

    // Editor Application 绫诲０鏄?
    class EditorApplication : public vulkan_engine::application::ApplicationBase
    {
        public:
            explicit EditorApplication(const vulkan_engine::application::ApplicationConfig& config);
            ~EditorApplication() override = default;

            // 绂佺敤鎷疯礉
            EditorApplication(const EditorApplication&)            = delete;
            EditorApplication& operator=(const EditorApplication&) = delete;

            // 鍏佽绉诲姩
            EditorApplication(EditorApplication&&) noexcept            = default;
            EditorApplication& operator=(EditorApplication&&) noexcept = default;

        protected:
            bool on_initialize() override;
            void on_shutdown() override;
            void on_update(float delta_time) override;
            void on_render() override;
            void on_window_resize(const vulkan_engine::application::WindowResizeEvent& event) override;

        private:
            class Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace editor::bootstrap