#pragma once
#include <vector>
#include <array>

namespace nes
{

class ppu
{
public:
    uint8_t control;
    uint8_t status;
    uint8_t mask;
    uint8_t oam_addr;
    uint8_t oam_data;
    uint8_t scroll;
    uint8_t addr;
    uint8_t data;
    uint8_t oam_dma;

    void tick() {}

    std::array<uint32_t, 256 * 240> frame_buffer;
};

}