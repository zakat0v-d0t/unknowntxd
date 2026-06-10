#pragma once

#include <string>

#include "Image.h"

namespace RenderWare
{
    class ImageReader
    {
    public:
        static bool Load(const std::string& Path, Image& OutImage);
    };
}
