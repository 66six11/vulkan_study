#pragma once

/**
 * @file UsageExample.hpp
 * @brief VMA 璧勬簮绠＄悊绯荤粺浣跨敤绀轰緥
 *
 * 杩欎釜鏂囦欢灞曠ず浜嗗浣曚娇鐢ㄦ柊鐨?VMA 璧勬簮绠＄悊绯荤粺銆?
 * 瀹為檯椤圭洰涓笉闇€瑕佸寘鍚鏂囦欢銆?
 */

/*

================= 鍩烘湰浣跨敤绀轰緥 =================


// 1. 鍒涘缓 ResourceManager
auto resourceManager = std::make_shared<memory::ResourceManager>(deviceManager);


// 2. 鍒涘缓 Buffer

// 2.1 浣跨敤渚挎嵎鏂规硶鍒涘缓鍚勭绫诲瀷鐨?Buffer
auto stagingBuffer = resourceManager->createStagingBuffer(1024 * 1024);  // 1MB staging buffer
auto vertexBuffer = resourceManager->createVertexBuffer(1024 * 1024);    // 1MB vertex buffer
auto indexBuffer = resourceManager->createIndexBuffer(1024 * 1024);      // 1MB index buffer
auto uniformBuffer = resourceManager->createUniformBuffer(256, true);    // 256 bytes, 鎸佷箙鏄犲皠

// 2.2 浣跨敤 Builder 妯″紡鍒涘缓鑷畾涔?Buffer
auto customBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    .deviceLocal()
    .priority(0.8f)
    .build();

// 2.3 鍐欏叆鏁版嵁鍒?buffer
struct Vertex { float x, y, z; };
std::vector<Vertex> vertices = { {0,0,0}, {1,0,0}, {0,1,0} };
stagingBuffer->write(vertices.data(), vertices.size() * sizeof(Vertex));


// 3. 鍒涘缓 Image

// 3.1 浣跨敤渚挎嵎鏂规硶
auto colorAttachment = resourceManager->createColorAttachment(1920, 1080, VK_FORMAT_B8G8R8A8_UNORM);
auto depthAttachment = resourceManager->createDepthAttachment(1920, 1080, VK_FORMAT_D32_SFLOAT);
auto texture = resourceManager->createTexture(1024, 1024, VK_FORMAT_R8G8B8A8_UNORM, 10);  // 甯?mipmaps

// 3.2 浣跨敤 Builder 妯″紡
auto customImage = memory::VmaImageBuilder(allocator)
    .width(1024)
    .height(1024)
    .format(VK_FORMAT_R16G16B16A16_SFLOAT)
    .usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
    .mipLevels(1)
    .deviceLocal()
    .build();

// 3.3 鍒涘缓 Image View
VkImageView view = texture->createView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
                                       memory::ImageSubresourceRange::colorAll(10));


// 4. 甯冨眬杞崲
// 鍦ㄥ懡浠ょ紦鍐蹭腑鎵ц
vkCmdBeginCommandBuffer(cmd, ...);
texture->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
vkCmdEndCommandBuffer(cmd);


// 5. 姣忓抚 Uniform Buffer锛堝甫 frame-in-flight 鏀寔锛?
struct CameraData { glm::mat4 view; glm::mat4 proj; };
memory::PerFrameBuffer<CameraData> cameraBuffers(allocator, MAX_FRAMES_IN_FLIGHT);

cameraBuffers.update(cameraData);  // 鏇存柊褰撳墠甯?
VkBuffer currentBuffer = cameraBuffers.currentHandle();  // 鑾峰彇褰撳墠甯х殑 buffer


// 6. 浣跨敤鍐呭瓨姹?
auto& poolManager = resourceManager->poolManager();

// 鑾峰彇鐗瑰畾姹?
memory::MemoryPool* stagingPool = poolManager.getPool(memory::PoolType::Staging);

// 鍒涘缓鑷畾涔夋睜
memory::MemoryPool::CreateInfo poolInfo;
poolInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
poolInfo.blockSize = 256 * 1024 * 1024;  // 256MB blocks
poolInfo.name = "CustomPool";
auto customPool = poolManager.createPool(poolInfo);

// 浣跨敤姹犲垱寤?buffer
auto pooledBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .pool(customPool->handle())
    .build();


// 7. 缁熻淇℃伅

// 7.1 鎵撳嵃鎵€鏈夌粺璁?
resourceManager->printStats();

// 7.2 鑾峰彇 VMA 缁熻
auto stats = resourceManager->getStats();
LOG_INFO("Allocated: {} MB", stats.totalBytesAllocated / (1024.0 * 1024.0));

// 7.3 鑾峰彇鍚勫爢棰勭畻
auto budgets = resourceManager->getHeapBudgets();
for (size_t i = 0; i < budgets.size(); ++i) {
    LOG_INFO("Heap {}: {}/{} MB", i,
             budgets[i].usageBytes / (1024.0 * 1024.0),
             budgets[i].budgetBytes / (1024.0 * 1024.0));
}

// 7.4 鎵撳嵃姹犵粺璁?
poolManager.printStats();


// 8. 妫€鏌ュ唴瀛樺彲鐢ㄦ€?
if (resourceManager->isMemoryAvailable(1024 * 1024 * 1024)) {  // 妫€鏌ユ槸鍚︽湁 1GB 鍙敤
    // 鍒嗛厤澶у瀷璧勬簮
}


// 9. 璧勬簮鎷疯礉绀轰緥锛圕PU 渚э級
auto srcBuffer = resourceManager->createStagingBuffer(1024);
auto dstBuffer = resourceManager->createStagingBuffer(1024);
srcBuffer->write(data, size);
dstBuffer->copyFrom(*srcBuffer, size);


// 10. 鏄惧紡璧勬簮閿€姣侊紙閫氬父涓嶉渶瑕侊紝RAII 鑷姩澶勭悊锛?
resourceManager->destroyBuffer(stagingBuffer);
resourceManager->destroyImage(texture);


================= 楂樼骇鍔熻兘 =================


// A. 鎸佷箙鏄犲皠 Buffer锛堥浂鎷疯礉鏇存柊锛?
auto persistentBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    .hostVisible(true)  // 鍚敤鎸佷箙鏄犲皠
    .build();

// 鐩存帴鍐欏叆锛屼笉闇€瑕?map/unmap
void* mapped = persistentBuffer->map();
std::memcpy(mapped, &data, sizeof(data));
// 涓嶉渶瑕?unmap锛?


// B. 涓撶敤鍐呭瓨锛堢敤浜庡ぇ鍨嬭祫婧愶級
auto largeBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024 * 1024 * 100)  // 100MB
    .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    .build();


// C. 浼樺厛鍐呭瓨鍒嗛厤绛栫暐
auto fastBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT)  // 浼樺厛閫熷害
    .build();

auto compactBuffer = memory::VmaBufferBuilder(allocator)
    .size(1024)
    .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    .deviceLocal()
    .allocationFlags(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT)  // 浼樺厛鍐呭瓨
    .build();


================= 鏈€浣冲疄璺?=================

1. 瀵逛簬棰戠箒鏇存柊鐨勬暟鎹紙濡?uniform buffer锛夛細
   - 浣跨敤 host-visible + coherent 鍐呭瓨
   - 鍚敤鎸佷箙鏄犲皠锛坧ersistent map锛?
   - 浣跨敤 PerFrameBuffer 绠＄悊澶氬抚鏁版嵁

2. 瀵逛簬闈欐€佹暟鎹紙濡傞《鐐?绱㈠紩 buffer锛夛細
   - 浣跨敤 device-local 鍐呭瓨
   - 閫氳繃 staging buffer 涓婁紶
   - 鍒涘缓鍚庝笉鍐嶄慨鏀?

3. 瀵逛簬绾圭悊锛?
   - 浣跨敤 device-local 鍐呭瓨
   - 閫氳繃 staging buffer 涓婁紶
   - 棰勭敓鎴?mipmaps锛堝鏋滈渶瑕侊級

4. 鍐呭瓨姹犱娇鐢細
   - 鐩镐技澶у皬鍜岀敓鍛藉懆鏈熺殑璧勬簮浣跨敤鍚屼竴姹?
   - 閬垮厤棰戠箒鍒涘缓/閿€姣佸皬鍒嗛厤
   - 浣跨敤姹犵粺璁＄洃鎺у唴瀛樹娇鐢?

5. 閿欒澶勭悊锛?
   - 妫€鏌?isValid() 纭繚璧勬簮鍒涘缓鎴愬姛
   - 浣跨敤 isMemoryAvailable() 棰勬鏌ュぇ鍒嗛厤
   - 鎹曡幏 VulkanError 澶勭悊 VMA 閿欒

*/