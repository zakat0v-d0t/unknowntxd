#pragma once

#include <cstdint>

#include "Core/BinaryReader.h"
#include "RwConstants.h"

namespace RenderWare
{
    struct ChunkHeader
    {
        ChunkType Type;
        std::uint32_t Size;
        std::uint32_t Version;

        std::uint32_t LibraryVersion() const;
        std::size_t DataEnd(std::size_t HeaderStart) const;
    };

    class RwStream
    {
    public:
        explicit RwStream(BinaryReader& Reader);

        ChunkHeader ReadHeader();
        bool FindChunk(ChunkType Type, ChunkHeader& OutHeader, std::size_t LimitEnd);

        BinaryReader& Reader();

    private:
        BinaryReader& Source;
    };
}
