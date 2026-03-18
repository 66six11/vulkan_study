/**
 * @file VmaMemoryTest.cpp
 * @brief VMA 内存管理完整测试套件 (GTest)
 */

#include <gtest/gtest.h>
#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/memory/VmaBuffer.hpp"
#include "vulkan/memory/VmaImage.hpp"
#include "vulkan/memory/ResourceManager.hpp"
#include "vulkan/device/Device.hpp"
#include <memory>
#include <vector>
#include <cstring>

using namespace vulkan_engine::vulkan;
using namespace vulkan_engine::vulkan::memory;

// ==================== 测试夹具 ====================

class VmaAllocatorTest : public ::testing::Test
{
    protected:
        std::shared_ptr<DeviceManager>                               deviceManager;
        std::shared_ptr<vulkan_engine::vulkan::memory::VmaAllocator> allocator;

        void SetUp() override
        {
            DeviceManager::CreateInfo createInfo;
            createInfo.application_name   = "VMA Test";
            createInfo.enable_validation  = false;
            createInfo.enable_debug_utils = false;

            deviceManager = std::make_shared<DeviceManager>(createInfo);
            ASSERT_TRUE(deviceManager->initialize());

            vulkan_engine::vulkan::memory::VmaAllocator::CreateInfo allocatorInfo;
            allocatorInfo.enableBudget = true;
            allocator                  = std::make_shared<vulkan_engine::vulkan::memory::VmaAllocator>(deviceManager, allocatorInfo);
        }

        void TearDown() override
        {
            allocator.reset();
            deviceManager.reset();
        }
};

class VmaBufferTest : public ::testing::Test
{
    protected:
        std::shared_ptr<DeviceManager>                               deviceManager;
        std::shared_ptr<vulkan_engine::vulkan::memory::VmaAllocator> allocator;

        void SetUp() override
        {
            DeviceManager::CreateInfo createInfo;
            createInfo.application_name   = "VMA Buffer Test";
            createInfo.enable_validation  = false;
            createInfo.enable_debug_utils = false;

            deviceManager = std::make_shared<DeviceManager>(createInfo);
            ASSERT_TRUE(deviceManager->initialize());

            vulkan_engine::vulkan::memory::VmaAllocator::CreateInfo allocatorInfo;
            allocator = std::make_shared<vulkan_engine::vulkan::memory::VmaAllocator>(deviceManager, allocatorInfo);
        }

        void TearDown() override
        {
            allocator.reset();
            deviceManager.reset();
        }
};

class VmaImageTest : public ::testing::Test
{
    protected:
        std::shared_ptr<DeviceManager>                               deviceManager;
        std::shared_ptr<vulkan_engine::vulkan::memory::VmaAllocator> allocator;

        void SetUp() override
        {
            DeviceManager::CreateInfo createInfo;
            createInfo.application_name   = "VMA Image Test";
            createInfo.enable_validation  = false;
            createInfo.enable_debug_utils = false;

            deviceManager = std::make_shared<DeviceManager>(createInfo);
            ASSERT_TRUE(deviceManager->initialize());

            vulkan_engine::vulkan::memory::VmaAllocator::CreateInfo allocatorInfo;
            allocator = std::make_shared<vulkan_engine::vulkan::memory::VmaAllocator>(deviceManager, allocatorInfo);
        }

        void TearDown() override
        {
            allocator.reset();
            deviceManager.reset();
        }
};

class ResourceManagerTest : public ::testing::Test
{
    protected:
        std::shared_ptr<DeviceManager> deviceManager;

        void SetUp() override
        {
            DeviceManager::CreateInfo createInfo;
            createInfo.application_name   = "Resource Manager Test";
            createInfo.enable_validation  = false;
            createInfo.enable_debug_utils = false;

            deviceManager = std::make_shared<DeviceManager>(createInfo);
            ASSERT_TRUE(deviceManager->initialize());
        }

        void TearDown() override
        {
            deviceManager.reset();
        }
};

// ==================== VmaAllocator 测试 ====================

TEST_F(VmaAllocatorTest, CreationAndValidity)
{
    EXPECT_NE(allocator, nullptr);
    EXPECT_NE(allocator->handle(), VK_NULL_HANDLE);
    EXPECT_TRUE(allocator->isValid());
}

TEST_F(VmaAllocatorTest, InitialStatsAreZero)
{
    auto stats = allocator->getStats();
    EXPECT_EQ(stats.allocationCount, 0u);
    EXPECT_EQ(stats.totalBytesAllocated, 0u);
    EXPECT_EQ(stats.totalBytesUsed, 0u);
}

TEST_F(VmaAllocatorTest, HeapBudgetsAvailable)
{
    auto budgets = allocator->getHeapBudgets();
    EXPECT_FALSE(budgets.empty());

    for (const auto& budget : budgets)
    {
        EXPECT_GT(budget.budgetBytes, 0u);
    }
}

TEST_F(VmaAllocatorTest, MemoryTypeSupport)
{
    const auto& memProps = deviceManager->memory_properties();

    // 验证至少有一些内存类型支持常用属性
    uint32_t hostVisibleCount = 0;
    uint32_t deviceLocalCount = 0;

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if (allocator->supportsMemoryType(i, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            hostVisibleCount++;
        }
        if (allocator->supportsMemoryType(i, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            deviceLocalCount++;
        }
    }

    // 至少应该有一种内存类型支持这些属性
    EXPECT_GT(hostVisibleCount + deviceLocalCount, 0u);
}

// ==================== VmaBuffer 测试 ====================

TEST_F(VmaBufferTest, BasicCreation)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

    auto buffer = std::make_shared<VmaBuffer>(
                                              allocator,
                                              1024,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              allocInfo
                                             );

    EXPECT_NE(buffer, nullptr);
    EXPECT_NE(buffer->handle(), VK_NULL_HANDLE);
    EXPECT_TRUE(buffer->isValid());
    EXPECT_EQ(buffer->size(), 1024);
    EXPECT_EQ(buffer->usage(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

TEST_F(VmaBufferTest, VariousBufferTypes)
{
    struct TestCase
    {
        const char*        name;
        VkDeviceSize       size;
        VkBufferUsageFlags usage;
    };

    std::vector<TestCase> testCases = {
        {"Vertex Buffer", 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
        {"Index Buffer", 512, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
        {"Uniform Buffer", 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
        {"Storage Buffer", 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
        {"Transfer Source", 2048, VK_BUFFER_USAGE_TRANSFER_SRC_BIT},
        {"Transfer Destination", 2048, VK_BUFFER_USAGE_TRANSFER_DST_BIT},
    };

    for (const auto& testCase : testCases)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

        auto buffer = std::make_shared<VmaBuffer>(
                                                  allocator,
                                                  testCase.size,
                                                  testCase.usage,
                                                  allocInfo
                                                 );

        EXPECT_NE(buffer, nullptr) << "Failed to create " << testCase.name;
        EXPECT_EQ(buffer->size(), testCase.size) << testCase.name << " size mismatch";
    }
}

TEST_F(VmaBufferTest, PersistentMapping)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto buffer = std::make_shared<VmaBuffer>(
                                              allocator,
                                              256,
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              allocInfo
                                             );

    EXPECT_TRUE(buffer->isMapped());

    // 测试数据写入和读取
    struct TestData
    {
        float matrix[16];
        int   count;
        float padding[3];
    };

    TestData writeData;
    for (int i = 0; i < 16; ++i) writeData.matrix[i] = static_cast<float>(i);
    writeData.count = 42;

    buffer->write(&writeData, sizeof(TestData));

    TestData readData;
    buffer->read(&readData, sizeof(TestData));

    EXPECT_EQ(readData.count, 42);
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_FLOAT_EQ(readData.matrix[i], static_cast<float>(i));
    }
}

TEST_F(VmaBufferTest, DataReadWrite)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto buffer = std::make_shared<VmaBuffer>(
                                              allocator,
                                              1024,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              allocInfo
                                             );

    // 写入数据
    std::vector<float> testData(256, 3.14159f);
    buffer->write(testData.data(), testData.size() * sizeof(float));

    // 读取数据
    std::vector<float> readData(256);
    buffer->read(readData.data(), readData.size() * sizeof(float));

    // 验证数据
    for (size_t i = 0; i < testData.size(); ++i)
    {
        EXPECT_FLOAT_EQ(readData[i], testData[i]);
    }
}

TEST_F(VmaBufferTest, TemplateReadWrite)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto buffer = std::make_shared<VmaBuffer>(
                                              allocator,
                                              256,
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              allocInfo
                                             );

    struct UniformData
    {
        float viewMatrix[16];
        float projMatrix[16];
        float time;
        int   frameCount;
    };

    UniformData writeData;
    for (int i = 0; i < 16; ++i)
    {
        writeData.viewMatrix[i] = static_cast<float>(i);
        writeData.projMatrix[i] = static_cast<float>(i * 2);
    }
    writeData.time       = 123.456f;
    writeData.frameCount = 1000;

    buffer->writeT(writeData);
    auto readData = buffer->readT<UniformData>();

    EXPECT_FLOAT_EQ(readData.time, writeData.time);
    EXPECT_EQ(readData.frameCount, writeData.frameCount);
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_FLOAT_EQ(readData.viewMatrix[i], writeData.viewMatrix[i]);
    }
}

TEST_F(VmaBufferTest, MoveSemantics)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

    auto buffer1 = std::make_shared<VmaBuffer>(
                                               allocator,
                                               1024,
                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               allocInfo
                                              );

    VkBuffer handle = buffer1->handle();

    // 移动构造
    VmaBuffer buffer2 = std::move(*buffer1);
    EXPECT_EQ(buffer2.handle(), handle);
    EXPECT_EQ(buffer2.size(), 1024);
}

TEST_F(VmaBufferTest, ZeroSizeThrows)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

    VmaBufferPtr buffer;
    EXPECT_THROW(
                 buffer = std::make_shared<VmaBuffer>(
                     allocator, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, allocInfo
                 ),
                 std::runtime_error
                );
}

// ==================== VmaImage 测试 ====================

TEST_F(VmaImageTest, Basic2DCreation)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent            = {256, 256, 1};
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    auto image = std::make_shared<VmaImage>(allocator, imageInfo, allocInfo);

    EXPECT_NE(image, nullptr);
    EXPECT_NE(image->handle(), VK_NULL_HANDLE);
    EXPECT_TRUE(image->isValid());
    EXPECT_EQ(image->width(), 256);
    EXPECT_EQ(image->height(), 256);
    EXPECT_EQ(image->depth(), 1);
    EXPECT_EQ(image->format(), VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(image->mipLevels(), 1);
    EXPECT_EQ(image->arrayLayers(), 1);
}

TEST_F(VmaImageTest, DepthAttachment)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent            = {512, 512, 1};
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    auto image = std::make_shared<VmaImage>(allocator, imageInfo, allocInfo);

    EXPECT_NE(image, nullptr);
    EXPECT_EQ(image->format(), VK_FORMAT_D32_SFLOAT);
}

TEST_F(VmaImageTest, TextureWithMipmaps)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent            = {1024, 1024, 1};
    imageInfo.mipLevels         = 11; // log2(1024) + 1
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    auto image = std::make_shared<VmaImage>(allocator, imageInfo, allocInfo);

    EXPECT_NE(image, nullptr);
    EXPECT_EQ(image->mipLevels(), 11);
}

TEST_F(VmaImageTest, LayoutTracking)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.extent            = {128, 128, 1};
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.format            = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    auto image = std::make_shared<VmaImage>(allocator, imageInfo, allocInfo);

    EXPECT_EQ(image->currentLayout(), VK_IMAGE_LAYOUT_UNDEFINED);

    image->setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(image->currentLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

// ==================== ResourceManager 测试 ====================

TEST_F(ResourceManagerTest, CreationAndValidity)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = true;
    createInfo.enableBudget       = true;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    EXPECT_NE(resourceManager, nullptr);
    EXPECT_NE(resourceManager->allocator(), nullptr);
}

TEST_F(ResourceManagerTest, CreateAllResourceTypes)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = true;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    // Buffers
    auto vertexBuffer  = resourceManager->createVertexBuffer(1024);
    auto indexBuffer   = resourceManager->createIndexBuffer(512);
    auto uniformBuffer = resourceManager->createUniformBuffer(256, true);
    auto storageBuffer = resourceManager->createStorageBuffer(4096, false);
    auto stagingBuffer = resourceManager->createStagingBuffer(2048);

    EXPECT_NE(vertexBuffer, nullptr);
    EXPECT_NE(indexBuffer, nullptr);
    EXPECT_NE(uniformBuffer, nullptr);
    EXPECT_NE(storageBuffer, nullptr);
    EXPECT_NE(stagingBuffer, nullptr);

    EXPECT_NE(vertexBuffer->handle(), VK_NULL_HANDLE);
    EXPECT_NE(indexBuffer->handle(), VK_NULL_HANDLE);
    EXPECT_NE(uniformBuffer->handle(), VK_NULL_HANDLE);

    // Images
    auto colorImage = resourceManager->createColorAttachment(256, 256, VK_FORMAT_R8G8B8A8_UNORM);
    auto depthImage = resourceManager->createDepthAttachment(256, 256, VK_FORMAT_D32_SFLOAT);
    auto texture    = resourceManager->createTexture(512, 512, VK_FORMAT_R8G8B8A8_SRGB, 10);
    auto cubemap    = resourceManager->createCubemap(128, VK_FORMAT_R8G8B8A8_UNORM, 8);

    EXPECT_NE(colorImage, nullptr);
    EXPECT_NE(depthImage, nullptr);
    EXPECT_NE(texture, nullptr);
    EXPECT_NE(cubemap, nullptr);

    EXPECT_EQ(colorImage->format(), VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(depthImage->format(), VK_FORMAT_D32_SFLOAT);
    EXPECT_EQ(texture->mipLevels(), 10);
    EXPECT_EQ(cubemap->arrayLayers(), 6);
}

TEST_F(ResourceManagerTest, StatisticsTracking)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = true;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    auto statsBefore = resourceManager->getStats();
    EXPECT_EQ(statsBefore.allocationCount, 0u);

    // 创建资源
    auto buffer1 = resourceManager->createVertexBuffer(1024);
    auto buffer2 = resourceManager->createUniformBuffer(256);
    auto image   = resourceManager->createColorAttachment(256, 256, VK_FORMAT_R8G8B8A8_UNORM);

    auto statsAfter = resourceManager->getStats();
    EXPECT_GE(statsAfter.allocationCount, 3u);
    EXPECT_GT(statsAfter.totalBytesAllocated, 0u);
}

TEST_F(ResourceManagerTest, BudgetQuery)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = true;
    createInfo.enableBudget       = true;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    auto budgets = resourceManager->getHeapBudgets();
    EXPECT_FALSE(budgets.empty());

    for (const auto& budget : budgets)
    {
        EXPECT_GT(budget.budgetBytes, 0u);
    }
}

TEST_F(ResourceManagerTest, MemoryAvailabilityCheck)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = true;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    // 检查小内存可用性
    EXPECT_TRUE(resourceManager->isMemoryAvailable(1024 * 1024));       // 1MB
    EXPECT_TRUE(resourceManager->isMemoryAvailable(100 * 1024 * 1024)); // 100MB
}

TEST_F(ResourceManagerTest, ResourceCreationAndReuse)
{
    ResourceManager::CreateInfo createInfo;
    createInfo.enableDefaultPools = false;

    auto resourceManager = std::make_unique<ResourceManager>(deviceManager, createInfo);

    // 创建多个 buffer
    auto buffer1 = resourceManager->createStagingBuffer(1024);
    auto buffer2 = resourceManager->createStagingBuffer(2048);

    EXPECT_NE(buffer1, nullptr);
    EXPECT_NE(buffer2, nullptr);
    EXPECT_NE(buffer1->handle(), buffer2->handle());

    // 验证可以正常使用
    int testData = 42;
    buffer1->write(&testData, sizeof(testData));

    int readData = 0;
    buffer1->read(&readData, sizeof(readData));
    EXPECT_EQ(readData, testData);
}

// ==================== Builder 测试 ====================

TEST_F(VmaBufferTest, BufferBuilderBasic)
{
    auto buffer = VmaBufferBuilder(allocator)
                 .size(2048)
                 .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
                 .deviceLocal()
                 .build();

    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->size(), 2048);
    EXPECT_TRUE(buffer->isValid());
}

TEST_F(VmaBufferTest, BufferBuilderHostVisible)
{
    auto buffer = VmaBufferBuilder(allocator)
                 .size(512)
                 .usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
                 .hostVisible(true)
                 .build();

    EXPECT_NE(buffer, nullptr);
    EXPECT_TRUE(buffer->isMapped());
}

TEST_F(VmaBufferTest, BufferBuilderConvenienceMethods)
{
    auto staging = VmaBufferBuilder::createStagingBuffer(allocator, 1024);
    auto vertex  = VmaBufferBuilder::createVertexBuffer(allocator, 2048);
    auto index   = VmaBufferBuilder::createIndexBuffer(allocator, 512);
    auto uniform = VmaBufferBuilder::createUniformBuffer(allocator, 256, true);

    EXPECT_NE(staging, nullptr);
    EXPECT_NE(vertex, nullptr);
    EXPECT_NE(index, nullptr);
    EXPECT_NE(uniform, nullptr);

    EXPECT_EQ(staging->size(), 1024);
    EXPECT_EQ(vertex->size(), 2048);
    EXPECT_EQ(index->size(), 512);
    EXPECT_EQ(uniform->size(), 256);
}

TEST_F(VmaImageTest, ImageBuilderBasic)
{
    auto image = VmaImageBuilder(allocator)
                .width(512)
                .height(512)
                .format(VK_FORMAT_R8G8B8A8_UNORM)
                .usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .deviceLocal()
                .build();

    EXPECT_NE(image, nullptr);
    EXPECT_EQ(image->width(), 512);
    EXPECT_EQ(image->height(), 512);
    EXPECT_TRUE(image->isValid());
}

TEST_F(VmaImageTest, ImageBuilderConvenienceMethods)
{
    auto texture = VmaImageBuilder::createTexture(
                                                  allocator,
                                                  256,
                                                  256,
                                                  VK_FORMAT_R8G8B8A8_SRGB,
                                                  9
                                                 );

    auto depth = VmaImageBuilder::createDepthAttachment(
                                                        allocator,
                                                        1024,
                                                        768,
                                                        VK_FORMAT_D32_SFLOAT
                                                       );

    auto color = VmaImageBuilder::createColorAttachment(
                                                        allocator,
                                                        1920,
                                                        1080,
                                                        VK_FORMAT_R8G8B8A8_UNORM,
                                                        1,
                                                        VK_SAMPLE_COUNT_1_BIT
                                                       );

    EXPECT_NE(texture, nullptr);
    EXPECT_NE(depth, nullptr);
    EXPECT_NE(color, nullptr);

    EXPECT_EQ(texture->mipLevels(), 9);
    EXPECT_EQ(depth->format(), VK_FORMAT_D32_SFLOAT);
    EXPECT_EQ(color->width(), 1920);
    EXPECT_EQ(color->height(), 1080);
}

// ==================== 性能和压力测试 ====================

TEST_F(VmaAllocatorTest, MultipleAllocations)
{
    const int                 numBuffers = 100;
    std::vector<VmaBufferPtr> buffers;
    buffers.reserve(numBuffers);

    for (int i = 0; i < numBuffers; ++i)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

        buffers.push_back(std::make_shared<VmaBuffer>(
                                                      allocator,
                                                      1024,
                                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                      allocInfo
                                                     ));
    }

    // 验证所有 buffer 都创建成功
    for (const auto& buffer : buffers)
    {
        EXPECT_NE(buffer->handle(), VK_NULL_HANDLE);
    }

    // 验证统计信息
    auto stats = allocator->getStats();
    EXPECT_GE(stats.allocationCount, static_cast<uint32_t>(numBuffers));
}

TEST_F(VmaBufferTest, LargeAllocationHandling)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

    // 尝试分配 100MB
    VkDeviceSize largeSize = 100 * 1024 * 1024;

    try
    {
        auto buffer = std::make_shared<VmaBuffer>(
                                                  allocator,
                                                  largeSize,
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                  allocInfo
                                                 );
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->size(), largeSize);
    }
    catch (const std::exception&)
    {
        // 如果设备内存不足，这是预期的行为
        SUCCEED() << "Large allocation failed due to memory constraints";
    }
}

TEST_F(VmaImageTest, MultipleImageFormats)
{
    std::vector<VkFormat> formats = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
    };

    for (auto format : formats)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {128, 128, 1};
        imageInfo.mipLevels         = 1;
        imageInfo.arrayLayers       = 1;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        try
        {
            auto image = std::make_shared<VmaImage>(allocator, imageInfo, allocInfo);
            EXPECT_NE(image, nullptr);
            EXPECT_EQ(image->format(), format);
        }
        catch (const std::exception&)
        {
            SUCCEED() << "Format may not be supported";
        }
    }
}

// 主函数
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
