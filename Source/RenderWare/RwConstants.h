#pragma once

#include <cstdint>

namespace RenderWare
{
    enum class ChunkType : std::uint32_t
    {
        Struct = 0x00000001,
        String = 0x00000002,
        Extension = 0x00000003,
        TextureNative = 0x00000015,
        TextureDictionary = 0x00000016,
    };

    enum class Platform : std::uint32_t
    {
        Direct3D8 = 8,
        Direct3D9 = 9,
    };

    namespace RasterFormat
    {
        constexpr std::uint32_t PixelMask = 0x00000F00;

        constexpr std::uint32_t Default = 0x00000000;
        constexpr std::uint32_t Format1555 = 0x00000100;
        constexpr std::uint32_t Format565 = 0x00000200;
        constexpr std::uint32_t Format4444 = 0x00000300;
        constexpr std::uint32_t FormatLum8 = 0x00000400;
        constexpr std::uint32_t Format8888 = 0x00000500;
        constexpr std::uint32_t Format888 = 0x00000600;
        constexpr std::uint32_t Format555 = 0x00000A00;

        constexpr std::uint32_t AutoMipmap = 0x00001000;
        constexpr std::uint32_t Palette8 = 0x00002000;
        constexpr std::uint32_t Palette4 = 0x00004000;
        constexpr std::uint32_t Mipmap = 0x00008000;
    }

    enum class CompressionType
    {
        None,
        Dxt1,
        Dxt3,
        Dxt5,
    };

    constexpr std::uint32_t FourCc(char A, char B, char C, char D)
    {
        return static_cast<std::uint32_t>(A) |
               static_cast<std::uint32_t>(B) << 8 |
               static_cast<std::uint32_t>(C) << 16 |
               static_cast<std::uint32_t>(D) << 24;
    }
}
