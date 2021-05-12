#include "libnes/mos6502.h"

#include <cassert>

namespace nes
{
mos6502::mos6502(std::vector<uint8_t>& memory)
    : memory_(memory)
{}

void mos6502::tick()
{
    auto cmd = memory_[pc++];
    assert(cmd == 0xa9);
    auto arg = memory_[pc++];

    a = arg;
}

}