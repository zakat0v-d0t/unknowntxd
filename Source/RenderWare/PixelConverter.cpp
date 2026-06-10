#include "PixelConverter.h"

#include <array>

namespace RenderWare
{
    namespace
    {
        struct Color
        {
            std::uint8_t R = 0;
            std::uint8_t G = 0;
            std::uint8_t B = 0;
            std::uint8_t A = 255;
        };

        std::uint8_t Expand5(std::uint32_t Value)
        {
            Value &= 0x1F;
            return static_cast<std::uint8_t>(Value << 3 | Value >> 2);
        }

        std::uint8_t Expand6(std::uint32_t Value)
        {
            Value &= 0x3F;
            return static_cast<std::uint8_t>(Value << 2 | Value >> 4);
        }

        std::uint8_t Expand4(std::uint32_t Value)
        {
            Value &= 0x0F;
            return static_cast<std::uint8_t>(Value << 4 | Value);
        }

        void DecodeColorBlock(const std::uint8_t* Block, std::array<Color, 16>& Output, bool PunchThroughAlpha)
        {
            std::uint16_t Color0 = static_cast<std::uint16_t>(Block[0]) | static_cast<std::uint16_t>(Block[1]) << 8;
            std::uint16_t Color1 = static_cast<std::uint16_t>(Block[2]) | static_cast<std::uint16_t>(Block[3]) << 8;

            std::array<Color, 4> Palette;
            Palette[0] = {Expand5(Color0 >> 11), Expand6(Color0 >> 5), Expand5(Color0), 255};
            Palette[1] = {Expand5(Color1 >> 11), Expand6(Color1 >> 5), Expand5(Color1), 255};

            if (!PunchThroughAlpha || Color0 > Color1)
            {
                Palette[2] = {static_cast<std::uint8_t>((2 * Palette[0].R + Palette[1].R) / 3),
                              static_cast<std::uint8_t>((2 * Palette[0].G + Palette[1].G) / 3),
                              static_cast<std::uint8_t>((2 * Palette[0].B + Palette[1].B) / 3), 255};
                Palette[3] = {static_cast<std::uint8_t>((Palette[0].R + 2 * Palette[1].R) / 3),
                              static_cast<std::uint8_t>((Palette[0].G + 2 * Palette[1].G) / 3),
                              static_cast<std::uint8_t>((Palette[0].B + 2 * Palette[1].B) / 3), 255};
            }
            else
            {
                Palette[2] = {static_cast<std::uint8_t>((Palette[0].R + Palette[1].R) / 2),
                              static_cast<std::uint8_t>((Palette[0].G + Palette[1].G) / 2),
                              static_cast<std::uint8_t>((Palette[0].B + Palette[1].B) / 2), 255};
                Palette[3] = {0, 0, 0, 0};
            }

            std::uint32_t Indices = static_cast<std::uint32_t>(Block[4]) |
                                    static_cast<std::uint32_t>(Block[5]) << 8 |
                                    static_cast<std::uint32_t>(Block[6]) << 16 |
                                    static_cast<std::uint32_t>(Block[7]) << 24;

            for (int Index = 0; Index < 16; ++Index)
                Output[Index] = Palette[Indices >> (Index * 2) & 0x3];
        }
    }

    Image PixelConverter::Decode(const RasterDescription& Description,
                                 const std::vector<std::uint8_t>& PixelData,
                                 const std::vector<std::uint8_t>& Palette)
    {
        if (Description.Compression != CompressionType::None)
            return DecodeCompressed(Description, PixelData);
        if (Description.Format & RasterFormat::Palette4 || Description.Format & RasterFormat::Palette8)
            return DecodePalettised(Description, PixelData, Palette);
        return DecodeDirect(Description, PixelData);
    }

    Image PixelConverter::DecodeCompressed(const RasterDescription& Description,
                                           const std::vector<std::uint8_t>& PixelData)
    {
        Image Result(Description.Width, Description.Height);

        int BlocksX = (Description.Width + 3) / 4;
        int BlocksY = (Description.Height + 3) / 4;
        int BlockBytes = Description.Compression == CompressionType::Dxt1 ? 8 : 16;

        std::size_t Offset = 0;
        for (int BlockY = 0; BlockY < BlocksY; ++BlockY)
        {
            for (int BlockX = 0; BlockX < BlocksX; ++BlockX)
            {
                if (Offset + BlockBytes > PixelData.size())
                    return Result;

                const std::uint8_t* Block = PixelData.data() + Offset;
                std::array<std::uint8_t, 16> AlphaValues;
                AlphaValues.fill(255);

                const std::uint8_t* ColorBlock = Block;
                if (Description.Compression == CompressionType::Dxt3)
                {
                    for (int Index = 0; Index < 16; Index += 2)
                    {
                        std::uint8_t Packed = Block[Index / 2];
                        AlphaValues[Index] = Expand4(Packed & 0x0F);
                        AlphaValues[Index + 1] = Expand4(Packed >> 4);
                    }
                    ColorBlock = Block + 8;
                }
                else if (Description.Compression == CompressionType::Dxt5)
                {
                    std::array<std::uint8_t, 8> AlphaPalette;
                    AlphaPalette[0] = Block[0];
                    AlphaPalette[1] = Block[1];
                    if (AlphaPalette[0] > AlphaPalette[1])
                    {
                        for (int Index = 1; Index < 7; ++Index)
                            AlphaPalette[Index + 1] = static_cast<std::uint8_t>(
                                ((7 - Index) * AlphaPalette[0] + Index * AlphaPalette[1]) / 7);
                    }
                    else
                    {
                        for (int Index = 1; Index < 5; ++Index)
                            AlphaPalette[Index + 1] = static_cast<std::uint8_t>(
                                ((5 - Index) * AlphaPalette[0] + Index * AlphaPalette[1]) / 5);
                        AlphaPalette[6] = 0;
                        AlphaPalette[7] = 255;
                    }

                    std::uint64_t Bits = 0;
                    for (int Index = 0; Index < 6; ++Index)
                        Bits |= static_cast<std::uint64_t>(Block[2 + Index]) << (Index * 8);
                    for (int Index = 0; Index < 16; ++Index)
                        AlphaValues[Index] = AlphaPalette[Bits >> (Index * 3) & 0x7];
                    ColorBlock = Block + 8;
                }

                std::array<Color, 16> Colors;
                DecodeColorBlock(ColorBlock, Colors, Description.Compression == CompressionType::Dxt1);

                for (int Row = 0; Row < 4; ++Row)
                {
                    for (int Column = 0; Column < 4; ++Column)
                    {
                        int Index = Row * 4 + Column;
                        Color Texel = Colors[Index];
                        std::uint8_t Alpha = Description.Compression == CompressionType::Dxt1 ? Texel.A : AlphaValues[Index];
                        Result.SetPixel(BlockX * 4 + Column, BlockY * 4 + Row, Texel.R, Texel.G, Texel.B, Alpha);
                    }
                }

                Offset += BlockBytes;
            }
        }

        return Result;
    }

    Image PixelConverter::DecodePalettised(const RasterDescription& Description,
                                           const std::vector<std::uint8_t>& PixelData,
                                           const std::vector<std::uint8_t>& Palette)
    {
        Image Result(Description.Width, Description.Height);
        bool IsFourBit = (Description.Format & RasterFormat::Palette4) != 0;

        auto Lookup = [&](std::uint32_t PaletteIndex) -> Color {
            std::size_t Offset = static_cast<std::size_t>(PaletteIndex) * 4;
            if (Offset + 3 >= Palette.size())
                return {};
            return {Palette[Offset + 0], Palette[Offset + 1], Palette[Offset + 2], Palette[Offset + 3]};
        };

        for (int Y = 0; Y < Description.Height; ++Y)
        {
            for (int X = 0; X < Description.Width; ++X)
            {
                std::uint32_t PaletteIndex = 0;
                if (IsFourBit)
                {
                    std::size_t ByteOffset = (static_cast<std::size_t>(Y) * Description.Width + X) / 2;
                    if (ByteOffset >= PixelData.size())
                        continue;
                    std::uint8_t Packed = PixelData[ByteOffset];
                    PaletteIndex = (X & 1) ? (Packed >> 4) : (Packed & 0x0F);
                }
                else
                {
                    std::size_t ByteOffset = static_cast<std::size_t>(Y) * Description.Width + X;
                    if (ByteOffset >= PixelData.size())
                        continue;
                    PaletteIndex = PixelData[ByteOffset];
                }

                Color Texel = Lookup(PaletteIndex);
                Result.SetPixel(X, Y, Texel.R, Texel.G, Texel.B, Texel.A);
            }
        }

        return Result;
    }

    Image PixelConverter::DecodeDirect(const RasterDescription& Description,
                                       const std::vector<std::uint8_t>& PixelData)
    {
        Image Result(Description.Width, Description.Height);
        std::uint32_t PixelFormat = Description.Format & RasterFormat::PixelMask;
        int BytesPerPixel = Description.Depth / 8;
        if (BytesPerPixel <= 0)
            return Result;

        for (int Y = 0; Y < Description.Height; ++Y)
        {
            for (int X = 0; X < Description.Width; ++X)
            {
                std::size_t Offset = (static_cast<std::size_t>(Y) * Description.Width + X) * BytesPerPixel;
                if (Offset + BytesPerPixel > PixelData.size())
                    continue;

                const std::uint8_t* Source = PixelData.data() + Offset;
                Color Texel;

                if (PixelFormat == RasterFormat::Format8888)
                {
                    Texel = {Source[2], Source[1], Source[0], Source[3]};
                }
                else if (PixelFormat == RasterFormat::Format888)
                {
                    Texel = {Source[2], Source[1], Source[0], 255};
                }
                else if (PixelFormat == RasterFormat::FormatLum8)
                {
                    Texel = {Source[0], Source[0], Source[0], 255};
                }
                else
                {
                    std::uint16_t Value = static_cast<std::uint16_t>(Source[0]) | static_cast<std::uint16_t>(Source[1]) << 8;
                    if (PixelFormat == RasterFormat::Format565)
                        Texel = {Expand5(Value >> 11), Expand6(Value >> 5), Expand5(Value), 255};
                    else if (PixelFormat == RasterFormat::Format1555)
                        Texel = {Expand5(Value >> 10), Expand5(Value >> 5), Expand5(Value),
                                 static_cast<std::uint8_t>((Value & 0x8000) ? 255 : 0)};
                    else if (PixelFormat == RasterFormat::Format4444)
                        Texel = {Expand4(Value >> 8), Expand4(Value >> 4), Expand4(Value), Expand4(Value >> 12)};
                    else if (PixelFormat == RasterFormat::Format555)
                        Texel = {Expand5(Value >> 10), Expand5(Value >> 5), Expand5(Value), 255};
                    else
                        Texel = {Expand5(Value >> 10), Expand5(Value >> 5), Expand5(Value), 255};
                }

                Result.SetPixel(X, Y, Texel.R, Texel.G, Texel.B, Texel.A);
            }
        }

        return Result;
    }
}
