#pragma once

#include <cstdint>
#include <vector>

namespace RenderWare
{
    class Image
    {
    public:
        Image() = default;
        Image(int Width, int Height);

        void Resize(int Width, int Height);

        int Width() const;
        int Height() const;
        bool IsEmpty() const;

        std::uint8_t* Data();
        const std::uint8_t* Data() const;

        void SetPixel(int X, int Y, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A);

    private:
        int PixelWidth = 0;
        int PixelHeight = 0;
        std::vector<std::uint8_t> Pixels;
    };
}
