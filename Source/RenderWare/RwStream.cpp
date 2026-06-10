#include "RwStream.h"

#include <stdexcept>

namespace RenderWare
{
    std::uint32_t ChunkHeader::LibraryVersion() const
    {
        if (Version & 0xFFFF0000u)
            return ((Version >> 14 & 0x3FF00u) + 0x30000u) | (Version >> 16 & 0x3Fu);
        return Version << 8;
    }

    std::size_t ChunkHeader::DataEnd(std::size_t HeaderStart) const
    {
        return HeaderStart + 12 + Size;
    }

    RwStream::RwStream(BinaryReader& InReader)
        : Source(InReader)
    {
    }

    ChunkHeader RwStream::ReadHeader()
    {
        ChunkHeader Header;
        Header.Type = static_cast<ChunkType>(Source.ReadU32());
        Header.Size = Source.ReadU32();
        Header.Version = Source.ReadU32();
        return Header;
    }

    bool RwStream::FindChunk(ChunkType Type, ChunkHeader& OutHeader, std::size_t LimitEnd)
    {
        while (Source.Tell() + 12 <= LimitEnd)
        {
            std::size_t HeaderStart = Source.Tell();
            ChunkHeader Header = ReadHeader();
            if (Header.Type == Type)
            {
                OutHeader = Header;
                return true;
            }
            Source.Seek(Header.DataEnd(HeaderStart));
        }
        return false;
    }

    BinaryReader& RwStream::Reader()
    {
        return Source;
    }
}
