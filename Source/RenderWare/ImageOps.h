#pragma once

#include "Image.h"

namespace RenderWare
{
    class ImageOps
    {
    public:
        static Image Resize(const Image& Source, int NewWidth, int NewHeight);
        static bool DetectAlpha(const Image& Source);
    };
}
