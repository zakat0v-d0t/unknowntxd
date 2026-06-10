#include "ImageWriter.h"

#include "stb_image_write.h"

namespace RenderWare
{
    bool ImageWriter::SavePng(const std::string& Path, const Image& Source)
    {
        if (Source.IsEmpty() || Source.Width() <= 0 || Source.Height() <= 0)
            return false;

        int Stride = Source.Width() * 4;
        return stbi_write_png(Path.c_str(), Source.Width(), Source.Height(), 4, Source.Data(), Stride) != 0;
    }
}
