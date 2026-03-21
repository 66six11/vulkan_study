#pragma once

// RHI Resource RAII Wrapper - Automatic resource lifetime management

#include "engine/rhi/Handle.hpp"
#include <memory>
#include <functional>
#include <utility>

namespace engine::rhi
{
    // Forward declarations
    class Device;

    // Resource deleter interface
    template <typename HandleType> class IResourceDeleter
    {
        public:
            virtual      ~IResourceDeleter() = default;
            virtual void destroy(HandleType handle) = 0;
    };

    // Generic resource RAII wrapper
    template <typename HandleType> class Resource
    {
        public:
            Resource() = default;

            Resource(std::shared_ptr<IResourceDeleter<HandleType>> deleter, HandleType handle)
                : deleter_(std::move(deleter))
                , handle_(handle)
            {
            }

            // Function-based deleter
            Resource(std::function<void(HandleType)> deleterFunc, HandleType handle)
                : deleterFunc_(std::move(deleterFunc))
                , handle_(handle)
            {
            }

            ~Resource()
            {
                reset();
            }

            // Move semantics
            Resource(Resource&& other) noexcept
                : deleter_(std::move(other.deleter_))
                , deleterFunc_(std::move(other.deleterFunc_))
                , handle_(std::exchange(other.handle_, HandleType{}))
            {
            }

            Resource& operator=(Resource&& other) noexcept
            {
                if (this != &other)
                {
                    reset();
                    deleter_     = std::move(other.deleter_);
                    deleterFunc_ = std::move(other.deleterFunc_);
                    handle_      = std::exchange(other.handle_, HandleType{});
                }
                return *this;
            }

            // No copy
            Resource(const Resource&)            = delete;
            Resource& operator=(const Resource&) = delete;

            // Get handle
            [[nodiscard]] HandleType get() const noexcept { return handle_; }
            [[nodiscard]] HandleType operator*() const noexcept { return handle_; }
            [[nodiscard]] explicit   operator bool() const noexcept { return handle_.isValid(); }

            // Arrow operator for pointer-like access
            [[nodiscard]] const HandleType* operator->() const noexcept { return &handle_; }

            // Reset and destroy resource
            void reset()
            {
                if (handle_.isValid())
                {
                    if (deleter_)
                    {
                        deleter_->destroy(handle_);
                    }
                    else if (deleterFunc_)
                    {
                        deleterFunc_(handle_);
                    }
                }
                handle_ = HandleType{};
                deleter_.reset();
                deleterFunc_ = nullptr;
            }

            // Release ownership (returns handle without destroying)
            [[nodiscard]] HandleType release() noexcept
            {
                deleter_.reset();
                deleterFunc_ = nullptr;
                return std::exchange(handle_, HandleType{});
            }

            // Swap
            void swap(Resource& other) noexcept
            {
                std::swap(deleter_, other.deleter_);
                std::swap(deleterFunc_, other.deleterFunc_);
                std::swap(handle_, other.handle_);
            }

        private:
            std::shared_ptr<IResourceDeleter<HandleType>> deleter_;
            std::function<void(HandleType)>               deleterFunc_;
            HandleType                                    handle_{};
    };

    // Concrete resource type aliases
    using TextureResource       = Resource<TextureId>;
    using BufferResource        = Resource<BufferId>;
    using PipelineResource      = Resource<PipelineId>;
    using SwapChainResource     = Resource<SwapChainId>;
    using CommandBufferResource = Resource<CommandBufferId>;
    using DescriptorSetResource = Resource<DescriptorSetId>;
    using ShaderResource        = Resource<ShaderId>;
    using SamplerResource       = Resource<SamplerId>;

    // Swap function
    template <typename HandleType> void swap(Resource<HandleType>& a, Resource<HandleType>& b) noexcept
    {
        a.swap(b);
    }

    // Device-bound resource deleter
    template <typename HandleType> class DeviceResourceDeleter : public IResourceDeleter<HandleType>
    {
        public:
            using DestroyFunc = std::function<void(Device*, HandleType)>;

            DeviceResourceDeleter(std::shared_ptr<Device> device, DestroyFunc destroyFunc)
                : device_(std::move(device))
                , destroyFunc_(std::move(destroyFunc))
            {
            }

            void destroy(HandleType handle) override
            {
                if (device_ && destroyFunc_)
                {
                    destroyFunc_(device_.get(), handle);
                }
            }

        private:
            std::shared_ptr<Device> device_;
            DestroyFunc             destroyFunc_;
    };

    // Helper to create resource with device-bound deleter
    template <typename HandleType, typename DestroyFunc> Resource<HandleType> makeResource(
        std::shared_ptr<Device> device,
        HandleType              handle,
        DestroyFunc             destroyFunc)
    {
        auto deleter = std::make_shared<DeviceResourceDeleter<HandleType>>(
                                                                           std::move(device),
                                                                           std::move(destroyFunc)
                                                                          );
        return Resource<HandleType>(std::move(deleter), handle);
    }

    // Helper to create resource with simple lambda deleter
    template <typename HandleType> Resource<HandleType> makeResource(
        HandleType                      handle,
        std::function<void(HandleType)> deleterFunc)
    {
        return Resource<HandleType>(std::move(deleterFunc), handle);
    }
} // namespace engine::rhi
