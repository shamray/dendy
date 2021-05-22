#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>

#include "libnes/cpu_registers.hpp"

namespace nes
{

class cpu
{
public:
    explicit cpu(std::vector<uint8_t>& memory);

    program_counter pc;
    
    stack_register s{0x0100, 0xFD};
    flags_register p;

    arith_register a{&p};
    arith_register x{&p};
    arith_register y{&p};

    struct instruction
    {
        instruction(auto op, auto am, int cycles = 1)
            : command_{[op, am](auto& cpu){ return op(cpu, [&cpu, am](){ return am(cpu); }); }}
            , c_{cycles}
        {}
        instruction() = default;

        void execute(cpu& cpu)
        {
            if (is_finished())
                return;

            if (c_ == 0) {
                --ac_;
            }
            else if (--c_ == 0) {
                if (auto additional_cycles = command_(cpu))
                    ac_ = additional_cycles;
            }
        }

        [[nodiscard]] bool is_finished() const { return c_ == 0 && ac_ == 0; }

    private:
        std::function<int(cpu&)> command_;
        int c_{0};
        int ac_{0};
    };

    void tick();
    auto is_executing() { return !current_instruction.is_finished();}

    void write(uint16_t addr, uint8_t value) const { memory_[addr] = value; }

    [[nodiscard]] auto read(uint16_t addr) const { return memory_[addr]; }
    [[nodiscard]] auto read_signed(uint16_t addr) const { return static_cast<int8_t>(memory_[addr]); }
    [[nodiscard]] auto read_word(uint16_t addr) const -> uint16_t;
    [[nodiscard]] auto read_word_wrapped(uint16_t addr) const -> uint16_t;

    auto decode(uint8_t opcode) -> std::optional<instruction>;

private:
    std::vector<uint8_t>& memory_;
    instruction current_instruction;
    std::unordered_map<uint8_t, cpu::instruction> instruction_set;
};

}