#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Texture.h"

namespace RenderWare
{
    class RwStream;
    struct ChunkHeader;

    class TextureDictionary
    {
    public:
        bool LoadFromFile(const std::string& Path, std::string& OutError);
        bool SaveToFile(const std::string& Path, std::string& OutError);

        bool ReplaceTexture(int Index, const Image& Source);
        bool ResizeTexture(int Index, int NewWidth, int NewHeight);
        bool SetTextureCompression(int Index, CompressionType Compression);

        const std::vector<Texture>& Textures() const { return LoadedTextures; }
        bool IsLoaded() const { return Loaded; }
        bool IsModified() const { return Modified; }
        const std::string& SourcePath() const { return FilePath; }

    private:
        Texture ReadTextureNative(RwStream& Stream, const ChunkHeader& NativeHeader, std::size_t NativeStart);
        void RefreshEditedTexture(Texture& Item);

        std::vector<Texture> LoadedTextures;
        std::string FilePath;
        std::uint32_t FileVersion = 0x1803FFFF;
        std::uint16_t DeviceId = 0;
        bool Loaded = false;
        bool Modified = false;
    };
}
