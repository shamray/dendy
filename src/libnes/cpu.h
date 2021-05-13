#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>

namespace nes
{

class cpu
{
public:
    cpu(std::vector<uint8_t>& memory);

    uint16_t pc{0x8000};

    uint8_t a{0};
    uint8_t s{0};
    uint8_t p{0};

    uint8_t x{0};
    uint8_t y{0};

    struct instruction
    {
        using fetch_address = std::function< uint16_t (cpu&) >;
        using command       = std::function< void (cpu&, fetch_address) >;

        command         operation;
        fetch_address   fetch_operand_address;
        int             cycles {1};
        int             additional_cycles {0};

        void execute(cpu& cpu) const { operation(cpu, fetch_operand_address); }
    };

    void tick();

    auto read(uint16_t addr) const { return memory_[addr]; }
    void write(uint16_t addr, uint8_t value) const { memory_[addr] = value; }
    auto read_word(uint16_t addr) const -> uint16_t;
    auto decode(uint8_t opcode)->std::optional<instruction>;

private:
    std::vector<uint8_t>& memory_;
};

}