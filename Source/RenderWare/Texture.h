#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Image.h"
#include "RwConstants.h"

namespace RenderWare
{
    class Texture
    {
    public:
        const std::string& Name() const { return TextureName; }
        const std::string& MaskName() const { return MaskTextureName; }
        const std::string& FormatName() const { return PixelFormatName; }
        int Width() const { return ImageWidth; }
        int Height() const { return ImageHeight; }
        int MipLevels() const { return MipLevelCount; }
        bool HasAlpha() const { return AlphaPresent; }
        const Image& Pixels() const { return Bitmap; }
        bool IsEdited() const { return Edited; }

        std::string TextureName;
        std::string MaskTextureName;
        std::string PixelFormatName;
        int ImageWidth = 0;
        int ImageHeight = 0;
        int MipLevelCount = 0;
        bool AlphaPresent = false;
        Image Bitmap;

        std::uint32_t Platform = 0;
        std::uint32_t FilterAddressing = 0;
        std::uint8_t RasterType = 4;
        std::vector<std::uint8_t> RawChunk;

        bool Edited = false;
        Image EditSource;
        CompressionType EditCompression = CompressionType::None;
    };
}
