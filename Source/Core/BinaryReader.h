#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class BinaryReader
{
public:
    explicit BinaryReader(const std::vector<std::uint8_t>& Data);
    BinaryReader(const std::uint8_t* Data, std::size_t Size);

    std::uint8_t ReadU8();
    std::uint16_t ReadU16();
    std::uint32_t ReadU32();
    std::int32_t ReadI32();

    void ReadBytes(void* Destination, std::size_t Count);
    std::string ReadFixedString(std::size_t Count);
    void Skip(std::size_t Count);
    void Seek(std::size_t Position);

    std::size_t Tell() const;
    std::size_t Size() const;
    std::size_t Remaining() const;
    bool CanRead(std::size_t Count) const;

private:
    void EnsureAvailable(std::size_t Count) const;

    const std::uint8_t* Base;
    std::size_t Length;
    std::size_t Position;
};
