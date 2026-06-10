#include "ImageOps.h"

#include "stb_image_resize2.h"

namespace RenderWare
{
    Image ImageOps::Resize(const Image& Source, int NewWidth, int NewHeight)
    {
        if (NewWidth < 1)
            NewWidth = 1;
        if (NewHeight < 1)
            NewHeight = 1;

        Image Result(NewWidth, NewHeight);
        if (Source.IsEmpty())
            return Result;

        stbir_resize_uint8_srgb(Source.Data(), Source.Width(), Source.Height(), 0,
                                Result.Data(), NewWidth, NewHeight, 0, STBIR_RGBA);
        return Result;
    }

    bool ImageOps::DetectAlpha(const Image& Source)
    {
        const std::uint8_t* Data = Source.Data();
        std::size_t Count = static_cast<std::size_t>(Source.Width()) * Source.Height();
        for (std::size_t Index = 0; Index < Count; ++Index)
        {
            if (Data[Index * 4 + 3] != 255)
                return true;
        }
        return false;
    }
}
