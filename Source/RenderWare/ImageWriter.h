#pragma once

#include <string>

#include "Image.h"

namespace RenderWare
{
    class ImageWriter
    {
    public:
        static bool SavePng(const std::string& Path, const Image& Source);
    };
}
