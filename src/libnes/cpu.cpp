#include "libnes/cpu.h"

#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <string>

using namespace std::string_literals;

namespace nes
{

class unsupported_opcode : public std::runtime_error
{
public:
    unsupported_opcode(uint8_t opcode)
        : std::runtime_error("Unsupported opcode: "s + std::to_string(opcode))
    {}
};

namespace
{

// Operations

auto adc = [](auto& cpu, auto fetch_addr)
{
};

auto lda = [](auto& cpu, auto fetch_addr) 
{
    auto address = fetch_addr(cpu);
    auto operand = cpu.read(address);

    cpu.a = operand;

    cpu.p.set(cpu::flag::zero, cpu.a == 0);
    cpu.p.set(cpu::flag::negative, (cpu.a & 0x80 ) != 0);
};

auto ldx = [](auto& cpu, auto fetch_addr)
{
    auto address = fetch_addr(cpu);
    auto operand = cpu.read(address);
    cpu.x = operand;
};

auto sta = [](auto& cpu, auto fetch_addr) 
{
    auto address = fetch_addr(cpu);
    cpu.write(address, cpu.a);
};

auto brk = [](auto& cpu, auto _) 
{
    // TODO: set brake flag
};


// Addressing modes

auto imm = [](auto& cpu) 
{
    return cpu.pc++;
};

auto zp =  [](auto& cpu) 
{
    return cpu.read(cpu.pc++);
};

auto zpx = [](auto& cpu) 
{
    auto address = cpu.read(cpu.pc++);
    return cpu.x + address;
};

auto zpy = [](auto& cpu) 
{
    auto address = cpu.read(cpu.pc++);
    return cpu.y + address;
};

auto abs = [](auto& cpu) 
{
    auto address = cpu.read_word(cpu.pc);
    cpu.pc += 2;
    return address;
};

auto abx = [](auto& cpu) 
{
    auto address = cpu.read_word(cpu.pc) + cpu.x;
    cpu.pc += 2;
    return address;
};

auto aby = [](auto& cpu) 
{
    auto address = cpu.read_word(cpu.pc) + cpu.y;
    cpu.pc += 2;
    return address;
};

auto izx = [](auto& cpu) 
{
    auto index = cpu.read(cpu.pc++) + cpu.x;
    return cpu.read_word(index);
};

auto izy = [](auto& cpu) 
{
    auto index = cpu.read(cpu.pc++);
    return cpu.read_word(index) + cpu.y;
};

auto imp = [](auto& ) -> uint8_t 
{
    throw std::logic_error("Calling operand function for implied addressing mode");
};


// Instruction set lookup table

const std::unordered_map<uint8_t, cpu::instruction> instruction_set {

    {0xA9, { lda, imm, 2 }},
    {0xA5, { lda, zp , 3 }},
    {0xB5, { lda, zpx, 4 }},
    {0xAD, { lda, abs, 4 }},
    {0xBD, { lda, abx, 4, 1 }},
    {0xB9, { lda, aby, 4, 1 }},
    {0xA1, { lda, izx, 6 }},
    {0xB1, { lda, izy, 5, 1 }},

    {0x85, { sta, zp , 3 }},
    {0x95, { sta, zpx, 4 }},
    {0x8D, { sta, abs, 4 }},
    {0x9D, { sta, abx, 5 }},
    {0x99, { sta, aby, 5 }},
    {0x81, { sta, izx, 6 }},
    {0x91, { sta, izy, 6 }},

    {0xA2, { ldx, imm, 2 }},
    {0xA6, { ldx, zp , 3 }},
    {0xB6, { ldx, zpy, 3 }},
    {0xAE, { ldx, abs, 4 }},
    {0xBE, { ldx, aby, 4, 1 }},

    {0x00, { brk, imp, 7 }}
};

}

cpu::cpu(std::vector<uint8_t>& memory)
    : memory_{memory}
{
    pc = read_word(0xFFFC);
}

void cpu::tick()
{
    auto opcode = read(pc++);

    auto instruction = decode(opcode)
        .value_or(cpu::instruction{
            [opcode](auto...) { throw unsupported_opcode(opcode); },
            imp
        });

    instruction.execute(*this);
}

auto cpu::read_word(uint16_t addr) const -> uint16_t
{
    auto lo = read(addr);
    auto hi = read(addr + 1);

    return (hi << 8) | lo;
}

auto cpu::decode(uint8_t opcode) -> std::optional<instruction>
{
    auto found = instruction_set.find(opcode);
    if (found == std::end(instruction_set))
        return std::nullopt;

    return found->second;
}

}