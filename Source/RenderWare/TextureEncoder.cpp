#include "TextureEncoder.h"

#include <algorithm>

#include "Core/BinaryWriter.h"
#include "ImageOps.h"
#include "RwConstants.h"
#include "stb_dxt.h"

namespace RenderWare
{
    namespace
    {
        constexpr std::uint32_t D3dFmtA8R8G8B8 = 21;

        void GatherBlock(const Image& Source, int BlockX, int BlockY, std::uint8_t* Output)
        {
            for (int Row = 0; Row < 4; ++Row)
            {
                for (int Column = 0; Column < 4; ++Column)
                {
                    int X = std::min(BlockX * 4 + Column, Source.Width() - 1);
                    int Y = std::min(BlockY * 4 + Row, Source.Height() - 1);
                    const std::uint8_t* Texel = Source.Data() + (static_cast<std::size_t>(Y) * Source.Width() + X) * 4;
                    std::uint8_t* Destination = Output + (Row * 4 + Column) * 4;
                    Destination[0] = Texel[0];
                    Destination[1] = Texel[1];
                    Destination[2] = Texel[2];
                    Destination[3] = Texel[3];
                }
            }
        }

        std::vector<std::uint8_t> CompressDxt(const Image& Level, CompressionType Compression)
        {
            int BlocksX = (Level.Width() + 3) / 4;
            int BlocksY = (Level.Height() + 3) / 4;
            int BlockBytes = Compression == CompressionType::Dxt1 ? 8 : 16;

            std::vector<std::uint8_t> Output(static_cast<std::size_t>(BlocksX) * BlocksY * BlockBytes);
            std::size_t Offset = 0;

            for (int BlockY = 0; BlockY < BlocksY; ++BlockY)
            {
                for (int BlockX = 0; BlockX < BlocksX; ++BlockX)
                {
                    std::uint8_t Block[64];
                    GatherBlock(Level, BlockX, BlockY, Block);

                    if (Compression == CompressionType::Dxt1)
                    {
                        stb_compress_dxt_block(Output.data() + Offset, Block, 0, STB_DXT_NORMAL);
                    }
                    else if (Compression == CompressionType::Dxt5)
                    {
                        stb_compress_dxt_block(Output.data() + Offset, Block, 1, STB_DXT_NORMAL);
                    }
                    else
                    {
                        std::uint8_t Temp[16];
                        stb_compress_dxt_block(Temp, Block, 1, STB_DXT_NORMAL);
                        for (int Index = 0; Index < 8; ++Index)
                        {
                            std::uint8_t Low = Block[(Index * 2 + 0) * 4 + 3] >> 4;
                            std::uint8_t High = Block[(Index * 2 + 1) * 4 + 3] >> 4;
                            Output[Offset + Index] = static_cast<std::uint8_t>(Low | (High << 4));
                        }
                        std::copy(Temp + 8, Temp + 16, Output.begin() + Offset + 8);
                    }

                    Offset += BlockBytes;
                }
            }

            return Output;
        }

        std::vector<std::uint8_t> EncodeBgra(const Image& Level)
        {
            std::size_t Count = static_cast<std::size_t>(Level.Width()) * Level.Height();
            std::vector<std::uint8_t> Output(Count * 4);
            const std::uint8_t* Source = Level.Data();
            for (std::size_t Index = 0; Index < Count; ++Index)
            {
                Output[Index * 4 + 0] = Source[Index * 4 + 2];
                Output[Index * 4 + 1] = Source[Index * 4 + 1];
                Output[Index * 4 + 2] = Source[Index * 4 + 0];
                Output[Index * 4 + 3] = Source[Index * 4 + 3];
            }
            return Output;
        }

        std::vector<std::uint8_t> EncodeLevel(const Image& Level, CompressionType Compression)
        {
            if (Compression == CompressionType::None)
                return EncodeBgra(Level);
            return CompressDxt(Level, Compression);
        }

        void WriteChunkHeader(BinaryWriter& Writer, ChunkType Type, std::uint32_t Size, std::uint32_t Version)
        {
            Writer.WriteU32(static_cast<std::uint32_t>(Type));
            Writer.WriteU32(Size);
            Writer.WriteU32(Version);
        }
    }

    std::vector<std::uint8_t> TextureEncoder::EncodeNative(const Texture& Item, std::uint32_t Version)
    {
        const Image& Base = Item.EditSource;
        CompressionType Compression = Item.EditCompression;
        bool HasAlpha = Compression == CompressionType::Dxt1 ? false : ImageOps::DetectAlpha(Base);

        std::vector<Image> Levels;
        Levels.push_back(Base);
        if (Item.EditGenerateMips)
        {
            int LevelWidth = Base.Width();
            int LevelHeight = Base.Height();
            while (LevelWidth > 1 || LevelHeight > 1)
            {
                LevelWidth = LevelWidth > 1 ? LevelWidth / 2 : 1;
                LevelHeight = LevelHeight > 1 ? LevelHeight / 2 : 1;
                Levels.push_back(ImageOps::Resize(Base, LevelWidth, LevelHeight));
            }
        }

        std::uint32_t RasterFormatBits;
        std::uint8_t Depth;
        switch (Compression)
        {
        case CompressionType::Dxt1:
            RasterFormatBits = HasAlpha ? RasterFormat::Format1555 : RasterFormat::Format565;
            Depth = 16;
            break;
        case CompressionType::Dxt3:
            RasterFormatBits = RasterFormat::Format4444;
            Depth = 16;
            break;
        case CompressionType::Dxt5:
            RasterFormatBits = RasterFormat::Format8888;
            Depth = 16;
            break;
        default:
            RasterFormatBits = RasterFormat::Format8888;
            Depth = 32;
            break;
        }

        if (Levels.size() > 1)
            RasterFormatBits |= RasterFormat::Mipmap;

        bool IsD3d9 = Item.Platform != static_cast<std::uint32_t>(Platform::Direct3D8);
        std::uint32_t PlatformId = IsD3d9 ? static_cast<std::uint32_t>(Platform::Direct3D9)
                                          : static_cast<std::uint32_t>(Platform::Direct3D8);

        std::uint32_t SecondField;
        std::uint8_t FlagsOrCompression;
        std::uint8_t CompressionNumber = Compression == CompressionType::Dxt1 ? 1 :
                                         Compression == CompressionType::Dxt3 ? 3 :
                                         Compression == CompressionType::Dxt5 ? 5 : 0;

        if (IsD3d9)
        {
            switch (Compression)
            {
            case CompressionType::Dxt1: SecondField = FourCc('D', 'X', 'T', '1'); break;
            case CompressionType::Dxt3: SecondField = FourCc('D', 'X', 'T', '3'); break;
            case CompressionType::Dxt5: SecondField = FourCc('D', 'X', 'T', '5'); break;
            default: SecondField = D3dFmtA8R8G8B8; break;
            }
            FlagsOrCompression = 0;
            if (HasAlpha)
                FlagsOrCompression |= 0x1;
            if (Compression != CompressionType::None)
                FlagsOrCompression |= 0x8;
        }
        else
        {
            SecondField = HasAlpha ? 1 : 0;
            FlagsOrCompression = CompressionNumber;
        }

        BinaryWriter Struct;
        Struct.WriteU32(PlatformId);
        Struct.WriteU32(Item.FilterAddressing);
        Struct.WriteFixedString(Item.TextureName, 32);
        Struct.WriteFixedString(Item.MaskTextureName, 32);
        Struct.WriteU32(RasterFormatBits);
        Struct.WriteU32(SecondField);
        Struct.WriteU16(static_cast<std::uint16_t>(Base.Width()));
        Struct.WriteU16(static_cast<std::uint16_t>(Base.Height()));
        Struct.WriteU8(Depth);
        Struct.WriteU8(static_cast<std::uint8_t>(Levels.size()));
        Struct.WriteU8(Item.RasterType);
        Struct.WriteU8(FlagsOrCompression);

        for (const Image& Level : Levels)
        {
            std::vector<std::uint8_t> Encoded = EncodeLevel(Level, Compression);
            Struct.WriteU32(static_cast<std::uint32_t>(Encoded.size()));
            Struct.WriteBytes(Encoded.data(), Encoded.size());
        }

        BinaryWriter Native;
        WriteChunkHeader(Native, ChunkType::Struct, static_cast<std::uint32_t>(Struct.Size()), Version);
        Native.WriteBytes(Struct.Buffer().data(), Struct.Size());
        WriteChunkHeader(Native, ChunkType::Extension, 0, Version);

        BinaryWriter Chunk;
        WriteChunkHeader(Chunk, ChunkType::TextureNative, static_cast<std::uint32_t>(Native.Size()), Version);
        Chunk.WriteBytes(Native.Buffer().data(), Native.Size());

        return Chunk.Buffer();
    }
}
