#include "Image.h"

namespace RenderWare
{
    Image::Image(int Width, int Height)
    {
        Resize(Width, Height);
    }

    void Image::Resize(int Width, int Height)
    {
        PixelWidth = Width;
        PixelHeight = Height;
        Pixels.assign(static_cast<std::size_t>(Width) * Height * 4, 0);
    }

    int Image::Width() const
    {
        return PixelWidth;
    }

    int Image::Height() const
    {
        return PixelHeight;
    }

    bool Image::IsEmpty() const
    {
        return Pixels.empty();
    }

    std::uint8_t* Image::Data()
    {
        return Pixels.data();
    }

    const std::uint8_t* Image::Data() const
    {
        return Pixels.data();
    }

    void Image::SetPixel(int X, int Y, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A)
    {
        if (X < 0 || Y < 0 || X >= PixelWidth || Y >= PixelHeight)
            return;
        std::size_t Offset = (static_cast<std::size_t>(Y) * PixelWidth + X) * 4;
        Pixels[Offset + 0] = R;
        Pixels[Offset + 1] = G;
        Pixels[Offset + 2] = B;
        Pixels[Offset + 3] = A;
    }
}
