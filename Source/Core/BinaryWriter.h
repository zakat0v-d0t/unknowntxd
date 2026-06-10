#pragma once

#include <cstdint>
#include <string>
#include <vector>

class BinaryWriter
{
public:
    void WriteU8(std::uint8_t Value);
    void WriteU16(std::uint16_t Value);
    void WriteU32(std::uint32_t Value);
    void WriteBytes(const void* Data, std::size_t Count);
    void WriteFixedString(const std::string& Value, std::size_t Count);

    const std::vector<std::uint8_t>& Buffer() const;
    std::size_t Size() const;

private:
    std::vector<std::uint8_t> Storage;
};
