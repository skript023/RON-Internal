#pragma once

#include "common.hpp"
#include <SDK/Engine_classes.hpp>

namespace big
{
    inline SDK::FVector RotToForward(const SDK::FRotator& r)
    {
        float sp = std::sin(r.Pitch * M_PI / 180.f);
        float cp = std::cos(r.Pitch * M_PI / 180.f);
        float sy = std::sin(r.Yaw * M_PI / 180.f);
        float cy = std::cos(r.Yaw * M_PI / 180.f);

        return { cp * cy, cp * sy, sp };
    }

    inline SDK::FVector RotToRight(const SDK::FRotator& r)
    {
        float sy = std::sin(r.Yaw * M_PI / 180.f);
        float cy = std::cos(r.Yaw * M_PI / 180.f);

        return { -sy, cy, 0.f };
    }

    inline SDK::FVector RotToUp(const SDK::FRotator& r)
    {
        float sp = std::sin(r.Pitch * M_PI / 180.f);
        float cp = std::cos(r.Pitch * M_PI / 180.f);
        float sy = std::sin(r.Yaw * M_PI / 180.f);
        float cy = std::cos(r.Yaw * M_PI / 180.f);

        return { -sp * cy, -sp * sy, cp };
    }

    inline SDK::FVector QuatToUp(const SDK::FQuat& q)
    {
        return {
            2.f * (q.X * q.Z - q.W * q.Y),
            2.f * (q.Y * q.Z + q.W * q.X),
            1.f - 2.f * (q.X * q.X + q.Y * q.Y)
        };
    }

    inline SDK::FVector QuatToRight(const SDK::FQuat& q)
    {
        return {
            1.f - 2.f * (q.Y * q.Y + q.Z * q.Z),
            2.f * (q.X * q.Y + q.W * q.Z),
            2.f * (q.X * q.Z - q.W * q.Y)
        };
    }

    inline SDK::FVector RotationToVector(const SDK::FRotator& rotator)
    {
        float pitchRad = rotator.Pitch * (M_PI / 180.0f); // Convert pitch to radians
        float yawRad = rotator.Yaw * (M_PI / 180.0f);     // Convert yaw to radians

        float cosPitch = std::cos(pitchRad);
        return SDK::FVector{
            cosPitch * std::cos(yawRad),  // X component
            cosPitch * std::sin(yawRad),  // Y component
            std::sin(pitchRad)            // Z component
        };
    }

    struct Color
    {
        Color(std::uint8_t r = 0, std::uint8_t g = 0, std::uint8_t b = 0, std::uint8_t a = 255) :
            r(r), g(g), b(b), a(a)
        {}

        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Color, r, g, b, a)
    };

    static_assert(sizeof(Color) == sizeof(std::uint8_t) * 4);
    /**
     * \brief A stream buffer that uses a char[] buffer
     */
    class MemoryStreamBuf : public std::streambuf
    {
    public:
        MemoryStreamBuf(char* array, std::size_t size)
        {
            std::fill_n(array, size, '\0');
            std::streambuf::setp(array, array + size - 1);
        }

        template <std::size_t size>
        MemoryStreamBuf(char(&array)[size]) :
            MemoryStreamBuf(&array[0], size)
        {
        }
    };

    /**
     * \brief An input/output stream that uses a char[] buffer
     */
    class MemoryStringStream : virtual MemoryStreamBuf, public std::iostream
    {
    public:
        MemoryStringStream(char* array, std::size_t size) :
            MemoryStreamBuf(array, size),
            std::iostream(static_cast<MemoryStreamBuf*>(this))
        {
        }

        template <std::size_t size>
        MemoryStringStream(char(&array)[size]) :
            MemoryStreamBuf(array),
            std::iostream(static_cast<MemoryStreamBuf*>(this))
        {
        }
    };

    /**
     * \brief A MemoryStringStream with a built in buffer
     */
    template <std::size_t N>
    class MemoryStringStreamWithBuffer : public MemoryStringStream
    {
    public:
        MemoryStringStreamWithBuffer():
            MemoryStringStream(m_Buffer)
        {}
    private:
        char m_Buffer[N];
    };

    inline constexpr char ToLower(const char c)
    {
        return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    }

    /**
     * \brief Calculates JOAAT hash of string at compile time
     */
}