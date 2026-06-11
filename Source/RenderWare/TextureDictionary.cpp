#include "TextureDictionary.h"

#include <cstdio>
#include <stdexcept>

#include "Core/BinaryReader.h"
#include "Core/BinaryWriter.h"
#include "ImageOps.h"
#include "PixelConverter.h"
#include "RwStream.h"
#include "TextureEncoder.h"

namespace RenderWare
{
    namespace
    {
        int NearestPowerOfTwo(int Value)
        {
            if (Value < 1)
                Value = 1;
            if (Value > 4096)
                Value = 4096;
            int Lower = 1;
            while (Lower * 2 <= Value)
                Lower *= 2;
            int Upper = Lower * 2;
            return (Value - Lower < Upper - Value) ? Lower : Upper;
        }

        std::vector<std::uint8_t> ReadWholeFile(const std::string& Path)
        {
            std::FILE* Handle = std::fopen(Path.c_str(), "rb");
            if (Handle == nullptr)
                throw std::runtime_error("Unable to open file: " + Path);

            std::fseek(Handle, 0, SEEK_END);
            long FileSize = std::ftell(Handle);
            std::fseek(Handle, 0, SEEK_SET);

            std::vector<std::uint8_t> Buffer(FileSize > 0 ? static_cast<std::size_t>(FileSize) : 0);
            if (!Buffer.empty())
            {
                std::size_t Read = std::fread(Buffer.data(), 1, Buffer.size(), Handle);
                Buffer.resize(Read);
            }
            std::fclose(Handle);
            return Buffer;
        }

        CompressionType ResolveDxt8(std::uint8_t Compression)
        {
            switch (Compression)
            {
            case 1:
                return CompressionType::Dxt1;
            case 2:
            case 3:
                return CompressionType::Dxt3;
            case 4:
            case 5:
                return CompressionType::Dxt5;
            default:
                return CompressionType::None;
            }
        }

        CompressionType ResolveDxt9(std::uint32_t D3dFormat)
        {
            if (D3dFormat == FourCc('D', 'X', 'T', '1'))
                return CompressionType::Dxt1;
            if (D3dFormat == FourCc('D', 'X', 'T', '3'))
                return CompressionType::Dxt3;
            if (D3dFormat == FourCc('D', 'X', 'T', '5'))
                return CompressionType::Dxt5;
            return CompressionType::None;
        }

        std::string DescribeFormat(const RasterDescription& Description)
        {
            switch (Description.Compression)
            {
            case CompressionType::Dxt1:
                return "DXT1";
            case CompressionType::Dxt3:
                return "DXT3";
            case CompressionType::Dxt5:
                return "DXT5";
            default:
                break;
            }

            if (Description.Format & RasterFormat::Palette8)
                return "PAL8";
            if (Description.Format & RasterFormat::Palette4)
                return "PAL4";

            switch (Description.Format & RasterFormat::PixelMask)
            {
            case RasterFormat::Format8888:
                return "RGBA8888";
            case RasterFormat::Format888:
                return "RGB888";
            case RasterFormat::Format1555:
                return "RGBA1555";
            case RasterFormat::Format565:
                return "RGB565";
            case RasterFormat::Format4444:
                return "RGBA4444";
            case RasterFormat::Format555:
                return "RGB555";
            case RasterFormat::FormatLum8:
                return "LUM8";
            default:
                return "Unknown";
            }
        }
    }

    bool TextureDictionary::LoadFromFile(const std::string& Path, std::string& OutError)
    {
        try
        {
            std::vector<std::uint8_t> FileData = ReadWholeFile(Path);
            BinaryReader Reader(FileData);
            RwStream Stream(Reader);

            std::size_t DictionaryStart = Reader.Tell();
            ChunkHeader DictionaryHeader = Stream.ReadHeader();
            if (DictionaryHeader.Type != ChunkType::TextureDictionary)
                throw std::runtime_error("Not a RenderWare texture dictionary");

            std::size_t DictionaryEnd = DictionaryHeader.DataEnd(DictionaryStart);
            std::uint32_t Version = DictionaryHeader.Version;

            ChunkHeader StructHeader;
            if (!Stream.FindChunk(ChunkType::Struct, StructHeader, DictionaryEnd))
                throw std::runtime_error("Missing dictionary struct");

            std::uint16_t TextureCount = Reader.ReadU16();
            std::uint16_t Device = Reader.ReadU16();

            std::vector<Texture> Parsed;
            Parsed.reserve(TextureCount);

            for (std::uint16_t Index = 0; Index < TextureCount; ++Index)
            {
                ChunkHeader NativeHeader;
                if (!Stream.FindChunk(ChunkType::TextureNative, NativeHeader, DictionaryEnd))
                    break;

                std::size_t NativeStart = Reader.Tell() - 12;
                std::size_t NativeEnd = NativeHeader.DataEnd(NativeStart);
                Texture Item = ReadTextureNative(Stream, NativeHeader, NativeStart);
                Item.RawChunk.assign(FileData.begin() + NativeStart, FileData.begin() + NativeEnd);
                Parsed.push_back(std::move(Item));
                Reader.Seek(NativeEnd);
            }

            LoadedTextures = std::move(Parsed);
            FilePath = Path;
            FileVersion = Version;
            DeviceId = Device;
            Loaded = true;
            Modified = false;
            return true;
        }
        catch (const std::exception& Error)
        {
            OutError = Error.what();
            Loaded = false;
            LoadedTextures.clear();
            return false;
        }
    }

    Texture TextureDictionary::ReadTextureNative(RwStream& Stream, const ChunkHeader& NativeHeader, std::size_t NativeStart)
    {
        BinaryReader& Reader = Stream.Reader();
        std::size_t NativeEnd = NativeHeader.DataEnd(NativeStart);

        ChunkHeader StructHeader;
        if (!Stream.FindChunk(ChunkType::Struct, StructHeader, NativeEnd))
            throw std::runtime_error("Missing texture native struct");

        std::uint32_t PlatformId = Reader.ReadU32();
        std::uint32_t FilterAddressing = Reader.ReadU32();
        std::string Name = Reader.ReadFixedString(32);
        std::string MaskName = Reader.ReadFixedString(32);

        std::uint32_t Format = Reader.ReadU32();
        std::uint32_t SecondField = Reader.ReadU32();
        std::uint16_t Width = Reader.ReadU16();
        std::uint16_t Height = Reader.ReadU16();
        std::uint8_t Depth = Reader.ReadU8();
        std::uint8_t MipLevels = Reader.ReadU8();
        std::uint8_t RasterType = Reader.ReadU8();
        std::uint8_t FlagsOrCompression = Reader.ReadU8();

        RasterDescription Description;
        Description.Width = Width;
        Description.Height = Height;
        Description.Depth = Depth;
        Description.Format = Format;

        bool HasAlpha = false;
        if (PlatformId == static_cast<std::uint32_t>(Platform::Direct3D9))
        {
            HasAlpha = (FlagsOrCompression & 0x1) != 0;
            if (FlagsOrCompression & 0x8)
                Description.Compression = ResolveDxt9(SecondField);
        }
        else
        {
            HasAlpha = SecondField != 0;
            Description.Compression = ResolveDxt8(FlagsOrCompression);
        }

        std::vector<std::uint8_t> Palette;
        if (Format & RasterFormat::Palette8)
        {
            Palette.resize(256 * 4);
            Reader.ReadBytes(Palette.data(), Palette.size());
        }
        else if (Format & RasterFormat::Palette4)
        {
            Palette.resize(32 * 4);
            Reader.ReadBytes(Palette.data(), Palette.size());
        }

        std::vector<std::uint8_t> BaseLevel;
        for (std::uint8_t Level = 0; Level < MipLevels; ++Level)
        {
            std::uint32_t LevelSize = Reader.ReadU32();
            if (Level == 0)
            {
                BaseLevel.resize(LevelSize);
                Reader.ReadBytes(BaseLevel.data(), LevelSize);
            }
            else
            {
                Reader.Skip(LevelSize);
            }
        }

        Texture Result;
        Result.TextureName = Name;
        Result.MaskTextureName = MaskName;
        Result.PixelFormatName = DescribeFormat(Description);
        Result.ImageWidth = Width;
        Result.ImageHeight = Height;
        Result.MipLevelCount = MipLevels;
        Result.AlphaPresent = HasAlpha;
        Result.SourceCompression = Description.Compression;
        Result.Bitmap = PixelConverter::Decode(Description, BaseLevel, Palette);
        Result.Platform = PlatformId;
        Result.FilterAddressing = FilterAddressing;
        Result.RasterType = RasterType;
        return Result;
    }

    void TextureDictionary::RefreshEditedTexture(Texture& Item)
    {
        Item.Bitmap = Item.EditSource;
        Item.ImageWidth = Item.EditSource.Width();
        Item.ImageHeight = Item.EditSource.Height();

        bool Alpha = ImageOps::DetectAlpha(Item.EditSource);
        Item.AlphaPresent = Item.EditCompression == CompressionType::Dxt1 ? false : Alpha;

        RasterDescription Description;
        Description.Format = RasterFormat::Format8888;
        Description.Compression = Item.EditCompression;
        Item.PixelFormatName = DescribeFormat(Description);
        Item.MipLevelCount = Item.EditGenerateMips ? Item.FullMipLevels() : 1;
    }

    bool TextureDictionary::ReplaceTexture(int Index, const Image& Source)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()) || Source.IsEmpty())
            return false;

        int TargetWidth = NearestPowerOfTwo(Source.Width());
        int TargetHeight = NearestPowerOfTwo(Source.Height());
        Image Fitted = (TargetWidth == Source.Width() && TargetHeight == Source.Height())
                           ? Source
                           : ImageOps::Resize(Source, TargetWidth, TargetHeight);

        Texture& Item = LoadedTextures[Index];
        Item.EditSource = Fitted;
        Item.EditCompression = ImageOps::DetectAlpha(Fitted) ? CompressionType::Dxt5 : CompressionType::Dxt1;
        Item.Edited = true;
        RefreshEditedTexture(Item);
        Modified = true;
        return true;
    }

    bool TextureDictionary::ResizeTexture(int Index, int NewWidth, int NewHeight)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()))
            return false;

        Texture& Item = LoadedTextures[Index];
        const Image& Base = Item.Edited ? Item.EditSource : Item.Bitmap;
        if (Base.IsEmpty())
            return false;

        if (!Item.Edited)
            Item.EditCompression = ImageOps::DetectAlpha(Base) ? CompressionType::Dxt5 : CompressionType::Dxt1;

        Item.EditSource = ImageOps::Resize(Base, NearestPowerOfTwo(NewWidth), NearestPowerOfTwo(NewHeight));
        Item.Edited = true;
        RefreshEditedTexture(Item);
        Modified = true;
        return true;
    }

    bool TextureDictionary::SetTextureCompression(int Index, CompressionType Compression)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()))
            return false;

        Texture& Item = LoadedTextures[Index];
        if (!Item.Edited)
            Item.EditSource = Item.Bitmap;

        Item.EditCompression = Compression;
        Item.Edited = true;
        RefreshEditedTexture(Item);
        Modified = true;
        return true;
    }

    bool TextureDictionary::SetTextureMipmaps(int Index, bool Generate)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()))
            return false;

        Texture& Item = LoadedTextures[Index];
        if (!Item.Edited)
        {
            Item.EditSource = Item.Bitmap;
            Item.EditCompression = Item.SourceCompression;
        }
        if (Item.EditSource.IsEmpty())
            return false;

        Item.EditGenerateMips = Generate;
        Item.Edited = true;
        RefreshEditedTexture(Item);
        Modified = true;
        return true;
    }

    bool TextureDictionary::RenameTexture(int Index, const std::string& Name, const std::string& Mask)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()))
            return false;

        auto PatchFixed = [](std::vector<std::uint8_t>& Buffer, std::size_t Offset, const std::string& Value) {
            for (std::size_t Position = 0; Position < 32; ++Position)
                Buffer[Offset + Position] = Position < Value.size() ? static_cast<std::uint8_t>(Value[Position]) : 0;
        };

        Texture& Item = LoadedTextures[Index];
        Item.TextureName = Name;
        Item.MaskTextureName = Mask;

        if (!Item.Edited && Item.RawChunk.size() >= 96)
        {
            PatchFixed(Item.RawChunk, 32, Name);
            PatchFixed(Item.RawChunk, 64, Mask);
        }

        Modified = true;
        return true;
    }

    bool TextureDictionary::SaveToFile(const std::string& Path, std::string& OutError)
    {
        auto WriteHeader = [&](BinaryWriter& Writer, ChunkType Type, std::uint32_t Size) {
            Writer.WriteU32(static_cast<std::uint32_t>(Type));
            Writer.WriteU32(Size);
            Writer.WriteU32(FileVersion);
        };

        try
        {
            BinaryWriter Struct;
            Struct.WriteU16(static_cast<std::uint16_t>(LoadedTextures.size()));
            Struct.WriteU16(DeviceId);

            BinaryWriter Payload;
            WriteHeader(Payload, ChunkType::Struct, static_cast<std::uint32_t>(Struct.Size()));
            Payload.WriteBytes(Struct.Buffer().data(), Struct.Size());

            for (Texture& Item : LoadedTextures)
            {
                if (Item.Edited)
                {
                    std::vector<std::uint8_t> Native = TextureEncoder::EncodeNative(Item, FileVersion);
                    Payload.WriteBytes(Native.data(), Native.size());
                    Item.RawChunk = std::move(Native);
                    Item.EditSource = Image();
                    Item.Edited = false;
                }
                else
                {
                    Payload.WriteBytes(Item.RawChunk.data(), Item.RawChunk.size());
                }
            }

            WriteHeader(Payload, ChunkType::Extension, 0);

            BinaryWriter File;
            WriteHeader(File, ChunkType::TextureDictionary, static_cast<std::uint32_t>(Payload.Size()));
            File.WriteBytes(Payload.Buffer().data(), Payload.Size());

            std::FILE* Handle = std::fopen(Path.c_str(), "wb");
            if (Handle == nullptr)
                throw std::runtime_error("Unable to open file for writing: " + Path);
            std::fwrite(File.Buffer().data(), 1, File.Size(), Handle);
            std::fclose(Handle);

            FilePath = Path;
            Modified = false;
            return true;
        }
        catch (const std::exception& Error)
        {
            OutError = Error.what();
            return false;
        }
    }

    bool TextureDictionary::AddTexture(const std::string& Name, const Image& Source)
    {
        if (Source.IsEmpty()) return false;
        
        Texture Item;
        Item.TextureName = Name;
        Item.Platform = static_cast<std::uint32_t>(Platform::Direct3D9);
        Item.FilterAddressing = 0x1101;
        Item.RasterType = 0x4; // Texture
        
        LoadedTextures.push_back(Item);
        int Index = static_cast<int>(LoadedTextures.size()) - 1;
        ReplaceTexture(Index, Source); 
        Modified = true;
        return true;
    }

    bool TextureDictionary::RemoveTexture(int Index)
    {
        if (Index < 0 || Index >= static_cast<int>(LoadedTextures.size()))
            return false;
            
        LoadedTextures.erase(LoadedTextures.begin() + Index);
        Modified = true;
        return true;
    }

    bool TextureDictionary::InsertTexture(int Index, const Texture& Item)
    {
        if (Index < 0 || Index > static_cast<int>(LoadedTextures.size()))
            return false;
            
        LoadedTextures.insert(LoadedTextures.begin() + Index, Item);
        Modified = true;
        return true;
    }
}
