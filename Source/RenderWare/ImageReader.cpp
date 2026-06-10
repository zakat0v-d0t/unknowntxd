#include "ImageReader.h"

#include <cstring>

#include "stb_image.h"

namespace RenderWare
{
    bool ImageReader::Load(const std::string& Path, Image& OutImage)
    {
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        stbi_uc* Data = stbi_load(Path.c_str(), &Width, &Height, &Channels, 4);
        if (Data == nullptr)
            return false;

        OutImage.Resize(Width, Height);
        std::memcpy(OutImage.Data(), Data, static_cast<std::size_t>(Width) * Height * 4);
        stbi_image_free(Data);
        return true;
    }
}
