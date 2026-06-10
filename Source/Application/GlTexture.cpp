#include "GlTexture.h"

#include <GL/gl.h>
#include <utility>

GlTexture::~GlTexture()
{
    Release();
}

GlTexture::GlTexture(GlTexture&& Other) noexcept
    : TextureId(Other.TextureId), TextureWidth(Other.TextureWidth), TextureHeight(Other.TextureHeight)
{
    Other.TextureId = 0;
    Other.TextureWidth = 0;
    Other.TextureHeight = 0;
}

GlTexture& GlTexture::operator=(GlTexture&& Other) noexcept
{
    if (this != &Other)
    {
        Release();
        TextureId = std::exchange(Other.TextureId, 0);
        TextureWidth = std::exchange(Other.TextureWidth, 0);
        TextureHeight = std::exchange(Other.TextureHeight, 0);
    }
    return *this;
}

void GlTexture::Upload(const RenderWare::Image& Source)
{
    if (Source.IsEmpty() || Source.Width() <= 0 || Source.Height() <= 0)
    {
        Release();
        return;
    }

    if (TextureId == 0)
        glGenTextures(1, &TextureId);

    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Source.Width(), Source.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, Source.Data());
    glBindTexture(GL_TEXTURE_2D, 0);

    TextureWidth = Source.Width();
    TextureHeight = Source.Height();
}

void GlTexture::Release()
{
    if (TextureId != 0)
    {
        glDeleteTextures(1, &TextureId);
        TextureId = 0;
    }
    TextureWidth = 0;
    TextureHeight = 0;
}

std::uint64_t GlTexture::Handle() const
{
    return static_cast<std::uint64_t>(TextureId);
}

bool GlTexture::IsValid() const
{
    return TextureId != 0;
}
