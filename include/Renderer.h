//
// Created by C66 on 2025/11/23.
//
#pragma once

#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H


// 不暴露 Vulkan，纯 POD，给未来后端用
struct FrameTiming
{
    float    deltaTime;  // 秒
    uint64_t frameIndex; // 从 0 递增
};

struct FrameContext
{
    FrameTiming timing;
    // 以后可以扩展：帧级别的渲染标志位，比如是否暂停、是否开启调试可视化等
};
// 先定义最小数据结构
struct CameraData {
    // viewProj, position 等，以后细化
};
     
struct MeshHandle {
    uint32_t id;
};
     
struct MaterialHandle {
    uint32_t id;
};
     
struct Renderable {
    MeshHandle mesh;
    MaterialHandle material;
    // transform 等
};

class Renderer
{
    public:
        virtual ~Renderer() = default;

        // windowHandle 可用 void*，后面在 VulkanRenderer 里 reinterpret_cast 成具体类型
        virtual void initialize(void* windowHandle, int width, int height) = 0;
        virtual void resize(int width, int height) = 0;

        // 帧驱动：推荐拆成两步，兼顾清晰度和实际需求
        virtual bool beginFrame(const FrameContext& ctx) = 0;
        virtual void renderFrame() = 0; // 或命名为 submitFrame()
        // 如果当前阶段暂时不需要特殊收尾，可以不暴露 endFrame()
        virtual void waitIdle() = 0;


        // 资源上传接口（可选在 1.3 末尾或 2.x 开始做）
        virtual MeshHandle createMesh(const void* vertexData, size_t vertexCount,
                                      const void* indexData, size_t indexCount) = 0;
     
        virtual void destroyMesh(MeshHandle mesh) = 0;
     
        // 场景提交接口（每帧调用）
        virtual void submitCamera(const CameraData& camera) = 0;
        virtual void submitRenderables(const Renderable* renderables, size_t count) = 0;
};


#endif //VULKAN_RENDERER_H
