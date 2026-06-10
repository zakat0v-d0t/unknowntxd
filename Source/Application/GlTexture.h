#pragma once

#include <cstdint>

#include "RenderWare/Image.h"

class GlTexture
{
public:
    GlTexture() = default;
    ~GlTexture();

    GlTexture(const GlTexture&) = delete;
    GlTexture& operator=(const GlTexture&) = delete;
    GlTexture(GlTexture&& Other) noexcept;
    GlTexture& operator=(GlTexture&& Other) noexcept;

    void Upload(const RenderWare::Image& Source);
    void Release();

    std::uint64_t Handle() const;
    bool IsValid() const;
    int Width() const { return TextureWidth; }
    int Height() const { return TextureHeight; }

private:
    unsigned int TextureId = 0;
    int TextureWidth = 0;
    int TextureHeight = 0;
};
