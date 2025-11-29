//
// Created by C66 on 2025/11/22.
//
#pragma once
#include <limits>
#include <shared_mutex>
#include <string>
#include <vector>
#include "core/constants.h"
#include "vulkan_backend/VulkanDevice.h"


/**
 * @brief 统一管理 Vulkan 缓冲区、图像和采样器等 GPU 资源
 *
 * ResourceManager 负责封装 VkBuffer / VkImage / VkSampler 等资源的创建与销毁，
 * 提供基于轻量句柄的访问接口，便于资源重用和集中管理生命周期。
 */
class ResourceManager
{
    public:
        struct BufferHandle
        {
            uint32_t index{std::numeric_limits<uint32_t>::max()};
            uint32_t generation{0};

            friend bool operator==(const BufferHandle&, const BufferHandle&) = default;
            explicit    operator bool() const noexcept { return index != UINT32_MAX; }
        };

        struct ImageHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
        };

        struct SamplerHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
        };

        /**
         * @brief 使用给定的 VulkanDevice 初始化资源管理器
         * @param device 引用到已经创建好的 VulkanDevice，对应的 VkDevice 用于创建/销毁资源
         */
        explicit ResourceManager(VulkanDevice& device);

        /**
         * @brief 析构函数
         *
         * 释放由 ResourceManager 创建并仍处于存活状态的所有 VkBuffer / VkImage / VkSampler 等资源。
         */
        ~ResourceManager();

        ResourceManager(const ResourceManager&)            = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        /**
         * @brief 缓冲区创建描述信息
         *
         * 描述缓冲区大小、用途以及内存属性等信息，用于统一创建 VkBuffer。
         */
        struct BufferDesc
        {
            VkDeviceSize          size;        ///< 缓冲区大小（字节）
            VkBufferUsageFlags    usage;       ///< VkBufferUsageFlags，用于指定顶点/索引/统一缓冲等用途
            VkMemoryPropertyFlags memoryFlags; ///< VkMemoryPropertyFlags，指定显存/主机可见等属性
            std::string           debugName;   ///< 调试名称，便于标记和调试
        };

        /**
         * @brief 图像创建描述信息
         *
         * 描述图像尺寸、格式、用途、多重采样等属性，用于统一创建 VkImage 及其默认视图。
         */
        struct ImageDesc
        {
            VkExtent3D            extent; ///< 图像尺寸（宽、高、深）
            VkFormat              format; ///< 图像像素格式
            VkImageUsageFlags     usage;  ///< VkImageUsageFlags，指定采样/颜色附件/深度附件等用途
            VkImageAspectFlags    aspect; ///< 视图 aspect 标志（如颜色或深度）
            uint32_t              mipLevels{1};
            uint32_t              arrayLayers{1};
            VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
            std::string           debugName; ///< 调试名称
        };

        /**
         * @brief 创建缓冲区资源并返回句柄
         * @param desc 缓冲区创建描述信息
         * @return 用于后续访问该缓冲区的 BufferHandle
         */
        BufferHandle createBuffer(const BufferDesc& desc);

        /**
         * @brief 销毁由 ResourceManager 创建的缓冲区
         * @param handle 需要销毁的缓冲区句柄；若句柄无效则忽略
         */
        void destroyBuffer(BufferHandle handle);

        /**
         * @brief 创建图像资源（及其默认视图）并返回句柄
         * @param desc 图像创建描述信息
         * @return 用于后续访问该图像的 ImageHandle
         */
        ImageHandle createImage(const ImageDesc& desc);

        /**
         * @brief 销毁由 ResourceManager 创建的图像及其相关视图/内存
         * @param handle 需要销毁的图像句柄；若句柄无效则忽略
         */
        void destroyImage(ImageHandle handle);

        /**
         * @brief 创建采样器资源并返回句柄
         * @param info      VkSamplerCreateInfo 结构，描述采样参数
         * @param debugName 调试名称（可选）
         * @return 用于后续访问该采样器的 SamplerHandle
         */
        SamplerHandle createSampler(
            const VkSamplerCreateInfo& info,
            std::string_view           debugName = {});

        /**
         * @brief 销毁由 ResourceManager 创建的采样器
         * @param handle 需要销毁的采样器句柄；若句柄无效则忽略
         */
        void destroySampler(SamplerHandle handle);

        /**
         * @brief 根据缓冲区句柄获取底层 VkBuffer 对象
         * @param handle 缓冲区句柄
         * @return 对应的 VkBuffer，若句柄无效则返回 VK_NULL_HANDLE
         */
        VkBuffer getBuffer(BufferHandle handle) const;

        /**
         * @brief 根据图像句柄获取底层 VkImage 对象
         * @param handle 图像句柄
         * @return 对应的 VkImage，若句柄无效则返回 VK_NULL_HANDLE
         */
        VkImage getImage(ImageHandle handle) const;

        /**
         * @brief 根据图像句柄获取默认 VkImageView 对象
         * @param handle 图像句柄
         * @return 对应的 VkImageView，若句柄无效或未创建视图则返回 VK_NULL_HANDLE
         */
        VkImageView getImageView(ImageHandle handle) const; // 如果内部帮你建 view

        /**
         * @brief 根据采样器句柄获取底层 VkSampler 对象
         * @param handle 采样器句柄
         * @return 对应的 VkSampler，若句柄无效则返回 VK_NULL_HANDLE
         */
        VkSampler getSampler(SamplerHandle handle) const;

        /**
         * @brief 获取缓冲区的创建描述信息（元数据）
         * @param handle 缓冲区句柄
         * @return 对应的 BufferDesc 引用；调用方需保证句柄有效
         */
        const BufferDesc& getBufferDesc(BufferHandle handle) const;

        /**
         * @brief 获取图像的创建描述信息（元数据）
         * @param handle 图像句柄
         * @return 对应的 ImageDesc 引用；调用方需保证句柄有效
         */
        const ImageDesc& getImageDesc(ImageHandle handle) const;

        /**
         * @brief 将数据上传到指定缓冲区
         *
         * 对于 HOST_VISIBLE 内存的缓冲区，该函数会直接映射内存并拷贝数据；
         * 对于纯 DEVICE_LOCAL 内存的缓冲区，可以在实现中使用 staging buffer + 复制命令实现。
         *
         * @param handle 目标缓冲区句柄
         * @param data   源数据指针
         * @param size   需要拷贝的数据大小（字节）
         * @param offset 目标缓冲区内偏移（字节）
         */
        void uploadBuffer(
            BufferHandle handle,
            const void*  data,
            VkDeviceSize size,
            VkDeviceSize offset = 0);

        /**
         * @brief 垃圾回收接口，用于延迟销毁或分帧回收资源
         *
         * 当前实现可以为空或简单遍历待回收列表，
         * 后续可扩展为与帧索引关联的延迟释放机制。
         */
        void garbageCollect(); // 延迟销毁、frame-based 回收等

    private:
        struct BufferEntry
        {
            VkBuffer       buffer{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE}; // 若没用 VMA
            BufferDesc     desc{};
            uint32_t       generation{0};
            bool           alive{false};
        };

        struct ImageEntry
        {
            VkImage        image{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE};
            VkImageView    defaultView{VK_NULL_HANDLE};
            ImageDesc      desc{};
            uint32_t       generation{0};
            bool           alive{false};
        };

        struct SamplerEntry
        {
            VkSampler   sampler{VK_NULL_HANDLE};
            std::string name;
            uint32_t    generation{0};
            bool        alive{false};
        };

        VulkanDevice& device_;

        std::vector<BufferEntry>  buffers_;
        std::vector<ImageEntry>   images_;
        std::vector<SamplerEntry> samplers_;
        std::vector<uint32_t>     freeBufferIndices_;
        std::vector<uint32_t>     freeImageIndices_;
        std::vector<uint32_t>     freeSamplerIndices_;

        // 可选细粒度锁：如果要支持多线程创建资源
        mutable std::shared_mutex bufferMutex_;
        mutable std::shared_mutex imageMutex_;
        mutable std::shared_mutex samplerMutex_;

        // 内部辅助函数：分配 index、创建 Vk 资源、绑定内存等
};
