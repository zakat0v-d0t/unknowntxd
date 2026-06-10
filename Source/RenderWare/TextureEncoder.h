#pragma once

#include <cstdint>
#include <vector>

#include "Texture.h"

namespace RenderWare
{
    class TextureEncoder
    {
    public:
        static std::vector<std::uint8_t> EncodeNative(const Texture& Item, std::uint32_t Version);
    };
}
