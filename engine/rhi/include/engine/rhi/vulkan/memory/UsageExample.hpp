#pragma once

/**
 * @file UsageExample.hpp
 * @brief VMA 资源管理系统使用示例
 *
 * 这个文件展示了如何使用新的 VMA 资源管理系统。
 * 实际项目中不需要包含此文件。
 */

/*

================= 基本使用示例 =================


// 1. 创建 ResourceManager
auto resourceManager = std::make_shared<memory::ResourceManager>(deviceManager);


// 2. 创建 Buffer

// 2.1 使用便捷方法创建各种类型的 Buffer
auto stagingBuffer = resourceManager->createStagingBuffer(1024 * 1024);  // 1MB staging buffer
auto vertexBuffer = resourceManager->createVertexBuffer(1024 * 1024);    // 1MB vertex buffer
auto indexBuffer = resourceManager->createIndexBuffer(1024 * 1024);      // 1MB index buffer
auto uniformBuffer = resourceManager->createUniformBuffer(256, true);    // 256 bytes, 持久映射

// 2.2 使用 Builder 模式创建自定义 Buffer
auto customBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    .deviceLocal()
    .priority(0.8f)
    .build();

// 2.3 写入数据到 buffer
struct Vertex { float x, y, z; };
std::vector<Vertex> vertices = { {0,0,0}, {1,0,0}, {0,1,0} };
stagingBuffer->write(vertices.data(), vertices.size() * sizeof(Vertex));


// 3. 创建 Image

// 3.1 使用便捷方法
auto colorAttachment = resourceManager->createColorAttachment(1920, 1080, VK_FORMAT_B8G8R8A8_UNORM);
auto depthAttachment = resourceManager->createDepthAttachment(1920, 1080, VK_FORMAT_D32_SFLOAT);
auto texture = resourceManager->createTexture(1024, 1024, VK_FORMAT_R8G8B8A8_UNORM, 10);  // 带 mipmaps

// 3.2 使用 Builder 模式
auto customImage = memory::VmaImageBuilder(allocator)
    .width(1024)
    .height(1024)
    .format(VK_FORMAT_R16G16B16A16_SFLOAT)
    .usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
    .mipLevels(1)
    .deviceLocal()
    .build();

// 3.3 创建 Image View
VkImageView view = texture->createView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
                                       memory::ImageSubresourceRange::colorAll(10));


// 4. 布局转换
// 在命令缓冲中执行
vkCmdBeginCommandBuffer(cmd, ...);
texture->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
vkCmdEndCommandBuffer(cmd);


// 5. 每帧 Uniform Buffer（带 frame-in-flight 支持）
struct CameraData { glm::mat4 view; glm::mat4 proj; };
memory::PerFrameBuffer<CameraData> cameraBuffers(allocator, MAX_FRAMES_IN_FLIGHT);

cameraBuffers.update(cameraData);  // 更新当前帧
VkBuffer currentBuffer = cameraBuffers.currentHandle();  // 获取当前帧的 buffer


// 6. 使用内存池
auto& poolManager = resourceManager->poolManager();

// 获取特定池
memory::MemoryPool* stagingPool = poolManager.getPool(memory::PoolType::Staging);

// 创建自定义池
memory::MemoryPool::CreateInfo poolInfo;
poolInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
poolInfo.blockSize = 256 * 1024 * 1024;  // 256MB blocks
poolInfo.name = "CustomPool";
auto customPool = poolManager.createPool(poolInfo);

// 使用池创建 buffer
auto pooledBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .pool(customPool->handle())
    .build();


// 7. 统计信息

// 7.1 打印所有统计
resourceManager->printStats();

// 7.2 获取 VMA 统计
auto stats = resourceManager->getStats();
LOG_INFO("Allocated: {} MB", stats.totalBytesAllocated / (1024.0 * 1024.0));

// 7.3 获取各堆预算
auto budgets = resourceManager->getHeapBudgets();
for (size_t i = 0; i < budgets.size(); ++i) {
    LOG_INFO("Heap {}: {}/{} MB", i,
             budgets[i].usageBytes / (1024.0 * 1024.0),
             budgets[i].budgetBytes / (1024.0 * 1024.0));
}

// 7.4 打印池统计
poolManager.printStats();


// 8. 检查内存可用性
if (resourceManager->isMemoryAvailable(1024 * 1024 * 1024)) {  // 检查是否有 1GB 可用
    // 分配大型资源
}


// 9. 资源拷贝示例（CPU 侧）
auto srcBuffer = resourceManager->createStagingBuffer(1024);
auto dstBuffer = resourceManager->createStagingBuffer(1024);
srcBuffer->write(data, size);
dstBuffer->copyFrom(*srcBuffer, size);


// 10. 显式资源销毁（通常不需要，RAII 自动处理）
resourceManager->destroyBuffer(stagingBuffer);
resourceManager->destroyImage(texture);


================= 高级功能 =================


// A. 持久映射 Buffer（零拷贝更新）
auto persistentBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    .hostVisible(true)  // 启用持久映射
    .build();

// 直接写入，不需要 map/unmap
void* mapped = persistentBuffer->map();
std::memcpy(mapped, &data, sizeof(data));
// 不需要 unmap！


// B. 专用内存（用于大型资源）
auto largeBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024 * 1024 * 100)  // 100MB
    .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    .build();


// C. 优先内存分配策略
auto fastBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT)  // 优先速度
    .build();

auto compactBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT)  // 优先内存
    .build();


================= 最佳实践 =================

1. 对于频繁更新的数据（如 uniform buffer）：
   - 使用 host-visible + coherent 内存
   - 启用持久映射（persistent map）
   - 使用 PerFrameBuffer 管理多帧数据

2. 对于静态数据（如顶点/索引 buffer）：
   - 使用 device-local 内存
   - 通过 staging buffer 上传
   - 创建后不再修改

3. 对于纹理：
   - 使用 device-local 内存
   - 通过 staging buffer 上传
   - 预生成 mipmaps（如果需要）

4. 内存池使用：
   - 相似大小和生命周期的资源使用同一池
   - 避免频繁创建/销毁小分配
   - 使用池统计监控内存使用

5. 错误处理：
   - 检查 isValid() 确保资源创建成功
   - 使用 isMemoryAvailable() 预检查大分配
   - 捕获 VulkanError 处理 VMA 错误

*/