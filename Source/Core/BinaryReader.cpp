#include "BinaryReader.h"

#include <cstring>
#include <stdexcept>

BinaryReader::BinaryReader(const std::vector<std::uint8_t>& Data)
    : Base(Data.data()), Length(Data.size()), Position(0)
{
}

BinaryReader::BinaryReader(const std::uint8_t* Data, std::size_t Size)
    : Base(Data), Length(Size), Position(0)
{
}

void BinaryReader::EnsureAvailable(std::size_t Count) const
{
    if (Position + Count > Length)
        throw std::runtime_error("BinaryReader: read past end of buffer");
}

std::uint8_t BinaryReader::ReadU8()
{
    EnsureAvailable(1);
    return Base[Position++];
}

std::uint16_t BinaryReader::ReadU16()
{
    EnsureAvailable(2);
    std::uint16_t Value = static_cast<std::uint16_t>(Base[Position]) |
                          static_cast<std::uint16_t>(Base[Position + 1]) << 8;
    Position += 2;
    return Value;
}

std::uint32_t BinaryReader::ReadU32()
{
    EnsureAvailable(4);
    std::uint32_t Value = static_cast<std::uint32_t>(Base[Position]) |
                          static_cast<std::uint32_t>(Base[Position + 1]) << 8 |
                          static_cast<std::uint32_t>(Base[Position + 2]) << 16 |
                          static_cast<std::uint32_t>(Base[Position + 3]) << 24;
    Position += 4;
    return Value;
}

std::int32_t BinaryReader::ReadI32()
{
    return static_cast<std::int32_t>(ReadU32());
}

void BinaryReader::ReadBytes(void* Destination, std::size_t Count)
{
    EnsureAvailable(Count);
    std::memcpy(Destination, Base + Position, Count);
    Position += Count;
}

std::string BinaryReader::ReadFixedString(std::size_t Count)
{
    EnsureAvailable(Count);
    const char* Start = reinterpret_cast<const char*>(Base + Position);
    std::size_t Used = 0;
    while (Used < Count && Start[Used] != '\0')
        ++Used;
    std::string Value(Start, Used);
    Position += Count;
    return Value;
}

void BinaryReader::Skip(std::size_t Count)
{
    EnsureAvailable(Count);
    Position += Count;
}

void BinaryReader::Seek(std::size_t NewPosition)
{
    if (NewPosition > Length)
        throw std::runtime_error("BinaryReader: seek past end of buffer");
    Position = NewPosition;
}

std::size_t BinaryReader::Tell() const
{
    return Position;
}

std::size_t BinaryReader::Size() const
{
    return Length;
}

std::size_t BinaryReader::Remaining() const
{
    return Length - Position;
}

bool BinaryReader::CanRead(std::size_t Count) const
{
    return Position + Count <= Length;
}
