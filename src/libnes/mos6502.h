#pragma once

#include <cstdint>
#include <vector>
namespace nes
{

class mos6502
{
public:
    mos6502(std::vector<uint8_t>& memory);

    uint16_t pc{0x8000};

    uint8_t a{0};
    uint8_t s;
    uint8_t p;

    void tick();

private:
    std::vector<uint8_t>& memory_;
};

}