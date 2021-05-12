#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>

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

    struct instruction
    {
        using arg_t = std::function<uint8_t(mos6502&)>;
        using op_t = std::function<void(mos6502&, arg_t)>;

        op_t op;
        arg_t arg;
        int cycles{ 1 };
    };

    void tick();

    auto read(uint16_t addr) const { return memory_[addr]; }
    auto decode(uint8_t opcode)->std::optional<instruction>;

private:
    std::vector<uint8_t>& memory_;
};

}