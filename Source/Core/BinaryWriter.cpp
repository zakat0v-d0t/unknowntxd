#include "BinaryWriter.h"

#include <cstring>
#include <string>

void BinaryWriter::WriteU8(std::uint8_t Value)
{
    Storage.push_back(Value);
}

void BinaryWriter::WriteU16(std::uint16_t Value)
{
    Storage.push_back(static_cast<std::uint8_t>(Value & 0xFF));
    Storage.push_back(static_cast<std::uint8_t>(Value >> 8 & 0xFF));
}

void BinaryWriter::WriteU32(std::uint32_t Value)
{
    Storage.push_back(static_cast<std::uint8_t>(Value & 0xFF));
    Storage.push_back(static_cast<std::uint8_t>(Value >> 8 & 0xFF));
    Storage.push_back(static_cast<std::uint8_t>(Value >> 16 & 0xFF));
    Storage.push_back(static_cast<std::uint8_t>(Value >> 24 & 0xFF));
}

void BinaryWriter::WriteBytes(const void* Data, std::size_t Count)
{
    const std::uint8_t* Source = static_cast<const std::uint8_t*>(Data);
    Storage.insert(Storage.end(), Source, Source + Count);
}

void BinaryWriter::WriteFixedString(const std::string& Value, std::size_t Count)
{
    std::size_t Copied = Value.size() < Count ? Value.size() : Count;
    WriteBytes(Value.data(), Copied);
    for (std::size_t Index = Copied; Index < Count; ++Index)
        Storage.push_back(0);
}

const std::vector<std::uint8_t>& BinaryWriter::Buffer() const
{
    return Storage;
}

std::size_t BinaryWriter::Size() const
{
    return Storage.size();
}
