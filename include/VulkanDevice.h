// Created by C66 on 2025/11/22.
#pragma once
#include <functional>
#include <optional>
#include <vector>
#include "constants.h"


/// 设备创建配置
struct VulkanDeviceConfig
{
    // 必须支持的设备扩展（直接用 const char*，不重新枚举）
    std::vector<const char*> requiredExtensions;

    // 必须支持的特性（你关心的那几个 true 即可）
    // 例如：cfg.requiredFeatures.samplerAnisotropy = VK_TRUE;
    VkPhysicalDeviceFeatures requiredFeatures{}; // 要求“至少支持”的特性
};


/**
 * @brief Vulkan设备管理类
 * 
 * 封装Vulkan物理设备、逻辑设备和队列的管理，提供设备能力查询接口，
 * 管理设备特性和扩展，以及提供设备级工具函数
 */
class VulkanDevice
{
    public:
        /**
         * @brief 队列信息结构体
         * 保存队列族索引和队列句柄
         */
        struct QueueInfo
        {
            uint32_t familyIndex{};           ///< 队列族索引
            VkQueue  handle = VK_NULL_HANDLE; ///< 队列句柄
        };

        /**
         * @brief 构造函数
         * 
         * 初始化Vulkan设备，选择合适的物理设备，创建逻辑设备，
         * 并获取图形队列和呈现队列
         * 
         * @param instance Vulkan实例句柄
         * @param surface Vulkan表面句柄，用于呈现功能
         * @param config 设备创建配置
         */
        /// 主构造函数：根据配置筛选物理设备并创建逻辑设备
        VulkanDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanDeviceConfig& config);

        /**
         * @brief 析构函数
         * 
         * 清理和销毁Vulkan设备相关资源
         */
        ~VulkanDevice();

        // 禁用拷贝构造和赋值操作符，确保Vulkan设备的唯一性
        VulkanDevice(const VulkanDevice&)            = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        /**
         * @brief 获取Vulkan逻辑设备句柄
         * @return VkDevice 逻辑设备句柄
         */
        VkDevice device() const noexcept { return device_; }

        /**
         * @brief 获取Vulkan物理设备句柄
         * @return VkPhysicalDevice 物理设备句柄
         */
        VkPhysicalDevice physicalDevice() const noexcept { return physicalDevice_; }

        /**
         * @brief 获取Vulkan实例句柄
         * @return VkInstance 实例句柄
         */
        VkInstance vkInstance() const noexcept { return instance_; }

        /**
         * @brief 获取图形队列信息
         * @return const QueueInfo& 图形队列信息引用
         */
        const QueueInfo& graphicsQueue() const noexcept { return graphicsQueue_; }

        /**
         * @brief 获取呈现队列信息
         * @return const QueueInfo& 呈现队列信息引用
         */
        const QueueInfo& presentQueue() const noexcept { return presentQueue_; }

        /**
         * @brief 获取计算队列信息
         * @return std::optional<QueueInfo> 计算队列信息（如果支持的话）
         */
        std::optional<QueueInfo> computeQueue() const noexcept;

        /**
         * @brief 获取传输队列信息
         * @return std::optional<QueueInfo> 传输队列信息（如果支持的话）
         */
        std::optional<QueueInfo> transferQueue() const noexcept;

        /**
         * @brief 获取物理设备属性
         * @return const VkPhysicalDeviceProperties& 物理设备属性引用
         */
        const VkPhysicalDeviceProperties& properties() const noexcept;

        /**
         * @brief 获取物理设备内存属性
         * @return const VkPhysicalDeviceMemoryProperties& 物理设备内存属性引用
         */
        const VkPhysicalDeviceMemoryProperties& memoryProperties() const noexcept;

        /**
         * @brief 检查队列族是否支持呈现功能
         * @param surface 表面句柄
         * @param family 队列族索引
         * @return bool 如果支持呈现功能返回true，否则返回false
         */
        bool supportsPresentation(VkSurfaceKHR surface, uint32_t family) const;

        /**
         * @brief 检查物理设备是否支持指定格式的特定功能
         * 
         * 此函数用于验证物理设备是否支持特定格式在特定平铺模式下的功能
         * 
         * @param fmt 格式类型
         * @param tiling 图像平铺模式（线性或最优）
         * @param features 格式功能标志
         * @return bool 如果支持指定格式和功能返回true，否则返回false
         */

        bool supportsFormat(VkFormat fmt, VkImageTiling tiling, VkFormatFeatureFlags features) const;

        /**
         * @brief 创建命令池
         * 创建指定队列族的命令池，用于分配命令缓冲区
         * @param familyIndex 队列族索引
         * @param flags 命令池创建标志
         * @return VkCommandPool 新创建的命令池句柄
         */
        VkCommandPool createCommandPool(uint32_t familyIndex, VkCommandPoolCreateFlags flags) const;

        /**
         * @brief 立即提交命令
         *     
         * 创建临时命令缓冲区，执行记录函数，然后立即提交到指定队列族    
         * 这个函数通常用于一次性命令提交，如资源上传等    
         * @param queueFamily 队列族索引    
         * @param recordFn 命令记录函数，接受命令缓冲区句柄作为参数   
         */
        void submitImmediate(uint32_t queueFamily, std::function<void(VkCommandBuffer)> recordFn) const;

    private:
        VkInstance                       instance_       = VK_NULL_HANDLE; ///< Vulkan实例句柄（非所有者：由上层Platform持有）
        VkPhysicalDevice                 physicalDevice_ = VK_NULL_HANDLE; ///< Vulkan物理设备句柄（非所有者）
        VkDevice                         device_         = VK_NULL_HANDLE; ///< Vulkan逻辑设备句柄（所有者）
        QueueInfo                        graphicsQueue_{};                 ///< 图形队列信息
        QueueInfo                        presentQueue_{};                  ///< 呈现队列信息
        std::optional<QueueInfo>         computeQueue_;                    ///< 计算队列信息（可选）
        std::optional<QueueInfo>         transferQueue_;                   ///< 传输队列信息（可选）
        VkPhysicalDeviceProperties       properties_{};                    ///< 物理设备属性
        VkPhysicalDeviceMemoryProperties memoryProps_{};                   ///< 物理设备内存属性
};
