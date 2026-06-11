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
        bool SetTextureMipmaps(int Index, bool Generate);
        bool RenameTexture(int Index, const std::string& Name, const std::string& Mask);

        bool AddTexture(const std::string& Name, const Image& Source);
        bool InsertTexture(int Index, const Texture& Item);
        bool RemoveTexture(int Index);

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
