#pragma once

#include <cmath>
#include <concepts>
#include <type_traits>

namespace vulkan_engine::math
{
    // Type-safe vector concepts
    template <typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template <typename T>
    concept FloatingPoint = std::is_floating_point_v<T>;

    template <typename T>
    concept Integral = std::is_integral_v<T>;

    // Vector base class template
    template <Arithmetic T, size_t N> class Vector
    {
        public:
            using value_type             = T;
            static constexpr size_t size = N;

            // Default constructor
            constexpr Vector() noexcept = default;

            // Single value constructor
            explicit constexpr Vector(T value) noexcept
            {
                for (size_t i = 0; i < N; ++i)
                {
                    data_[i] = value;
                }
            }

            // Array constructor
            template <typename... Args>
                requires (sizeof...(Args) == N)
            constexpr Vector(Args... args) noexcept : data_{static_cast<T>(args)...}
            {
            }

            // Access operators
            constexpr T&       operator[](size_t index) noexcept { return data_[index]; }
            constexpr const T& operator[](size_t index) const noexcept { return data_[index]; }

            // Arithmetic operations
            constexpr Vector operator+(const Vector& other) const noexcept
            {
                Vector result;
                for (size_t i = 0; i < N; ++i)
                {
                    result[i] = data_[i] + other[i];
                }
                return result;
            }

            constexpr Vector operator-(const Vector& other) const noexcept
            {
                Vector result;
                for (size_t i = 0; i < N; ++i)
                {
                    result[i] = data_[i] - other[i];
                }
                return result;
            }

            // Dot product
            constexpr T dot(const Vector& other) const noexcept
            {
                T result = T{};
                for (size_t i = 0; i < N; ++i)
                {
                    result += data_[i] * other[i];
                }
                return result;
            }

            // Magnitude
            constexpr T length() const noexcept requires FloatingPoint<T>
            {
                return std::sqrt(dot(*this));
            }

            // Normalization
            constexpr Vector normalized() const noexcept requires FloatingPoint<T>
            {
                T len = length();
                if (len > 0)
                {
                    return (*this) * (T{1} / len);
                }
                return *this;
            }

            // Data access
            constexpr T*       data() noexcept { return data_; }
            constexpr const T* data() const noexcept { return data_; }

        private:
            T data_[N]{};
    };

    // Common vector types
    using Vector2f = Vector<float, 2>;
    using Vector3f = Vector<float, 3>;
    using Vector4f = Vector<float, 4>;
    using Vector2i = Vector<int32_t, 2>;
    using Vector3i = Vector<int32_t, 3>;
    using Vector4i = Vector<int32_t, 4>;
    using Vector2u = Vector<uint32_t, 2>;
    using Vector3u = Vector<uint32_t, 3>;
    using Vector4u = Vector<uint32_t, 4>;
} // namespace vulkan_engine::math