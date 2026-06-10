#pragma once

#include <cstdint>
#include <vector>

#include "Image.h"
#include "RwConstants.h"

namespace RenderWare
{
    struct RasterDescription
    {
        int Width = 0;
        int Height = 0;
        int Depth = 0;
        std::uint32_t Format = 0;
        CompressionType Compression = CompressionType::None;
    };

    class PixelConverter
    {
    public:
        static Image Decode(const RasterDescription& Description,
                            const std::vector<std::uint8_t>& PixelData,
                            const std::vector<std::uint8_t>& Palette);

    private:
        static Image DecodeCompressed(const RasterDescription& Description,
                                      const std::vector<std::uint8_t>& PixelData);
        static Image DecodePalettised(const RasterDescription& Description,
                                      const std::vector<std::uint8_t>& PixelData,
                                      const std::vector<std::uint8_t>& Palette);
        static Image DecodeDirect(const RasterDescription& Description,
                                  const std::vector<std::uint8_t>& PixelData);
    };
}
