#include "libnes/cpu.h"

#include <stdexcept>
#include <unordered_map>
#include <string>

using namespace std::string_literals;

namespace nes
{

class unsupported_opcode : public std::runtime_error
{
public:
    explicit unsupported_opcode(uint8_t opcode)
        : std::runtime_error("Unsupported opcode: "s + std::to_string(opcode))
    {}
};

namespace
{

constexpr auto operator "" _i(unsigned long long x) { return static_cast<uint8_t>(x); }

auto carry(const flags_register& f)
{
    return f.test(cpu_flag::carry) ? 1_i : 0_i;
}

auto is_page_crossed(uint16_t base, uint16_t effective_address)
{
    return (base & 0xFF00) != (effective_address & 0xFF00);
}

auto index(uint16_t base, uint8_t offset)
{
    auto address = base + offset;
    auto additional_cycles = is_page_crossed(base, address) ? 1 : 0;

    return std::tuple{address, additional_cycles};
}

auto arith_result(int x)
{
    auto c = (x / 0x100) != 0;
    auto r = (x % 0x100);

    return std::tuple{r, c};
}

void adc_impl(auto& result, uint8_t accum, uint8_t operand, flags_register& flags, bool set_overflow=true)
{
    auto [r, c] = arith_result(
            accum + operand + carry(flags)
    );
    flags.set(cpu_flag::carry, c);

    if (set_overflow) {
        auto v = ((operand ^ r) & (r ^ accum) & 0x80) != 0;
        flags.set(cpu_flag::overflow, v);
    }

    result = r;
}

// Operations

auto adc = [](auto& cpu, auto fetch_addr)
{
    auto [address, additional_cycles] = fetch_addr();
    auto operand = cpu.read(address);

    adc_impl(cpu.a, cpu.a.value(), operand, cpu.p);
    return additional_cycles;
};

auto sbc = [](auto& cpu, auto fetch_addr)
{
    auto [address, additional_cycles] = fetch_addr();
    auto operand = cpu.read(address);

    adc_impl(cpu.a, cpu.a.value(), -operand, cpu.p);
    return additional_cycles;
};

auto cmp = [](auto& cpu, auto fetch_addr)
{
    auto [address, additional_cycles] = fetch_addr();
    auto operand = cpu.read(address);

    [[maybe_unused]]
    auto alu_result = arith_register{&cpu.p};

    adc_impl(alu_result, cpu.a.value(), -operand, cpu.p, false);
    return additional_cycles;
};

auto lda = [](auto& cpu, auto fetch_addr)
{
    auto [address, additional_cycles] = fetch_addr();
    auto operand = cpu.read(address);

    cpu.a = operand;
    return additional_cycles;
};

auto ldx = [](auto& cpu, auto fetch_addr)
{
    auto [address, additional_cycles] = fetch_addr();
    auto operand = cpu.read(address);
    cpu.x = operand;
    return additional_cycles;
};

auto sta = [](auto& cpu, auto fetch_addr) 
{
    auto [address, additional_cycles] = fetch_addr();
    cpu.write(address, cpu.a.value());
    return additional_cycles;
};

auto tax = [](auto& cpu, auto )
{
    cpu.x.assign(cpu.a.value());
    return 0;
};

auto tay = [](auto& cpu, auto )
{
    cpu.y.assign(cpu.a.value());
    return 0;
};

auto tsx = [](auto& cpu, auto )
{
    cpu.x.assign(cpu.s.value());
    return 0;
};

auto txa = [](auto& cpu, auto )
{
    cpu.a.assign(cpu.x.value());
    return 0;
};

auto txs = [](auto& cpu, auto )
{
    cpu.s.assign(cpu.x.value());
    return 0;
};

auto tya = [](auto& cpu, auto )
{
    cpu.a.assign(cpu.y.value());
    return 0;
};

auto pha = [](auto& cpu, auto )
{
    cpu.write(cpu.s.push(), cpu.a.value());
    return 0;
};

auto pla = [](auto& cpu, auto )
{
    cpu.a = cpu.read(cpu.s.pop());
    return 0;
};

auto php = [](auto& cpu, auto )
{
    cpu.write(cpu.s.push(), cpu.p.value());
    return 0;
};

auto plp = [](auto& cpu, auto )
{
    auto flags_value = cpu.read(cpu.s.pop());
    cpu.p.assign(flags_value);
    return 0;
};

auto jmp = [](auto& cpu, auto fetch_addr)
{
    auto [address, _] = fetch_addr();
    cpu.pc.assign(address);
    return 0;
};

auto jsr = [](auto& cpu, auto fetch_addr)
{
    auto [address, _] = fetch_addr();
    cpu.write(cpu.s.push(), cpu.pc.hi());
    cpu.write(cpu.s.push(), cpu.pc.lo() - 1);
    cpu.pc.assign(address);
    return 0;
};

auto rts = [](auto& cpu, auto _)
{
    auto lo = cpu.read(cpu.s.pop());
    auto hi = cpu.read(cpu.s.pop());
    auto address = (hi << 8) | lo;
    cpu.pc.assign(address + 1);
    return 0;
};

auto rti = [](auto& cpu, auto _)
{
    plp(cpu, _);
    rts(cpu, _);
    return 0;
};

auto brk = [](auto& cpu, auto _)
{
    jsr(cpu, [&cpu] {
        return std::tuple{cpu.read_word(0xFFFE), false};
    });
    php(cpu, _);
    cpu.p.set(cpu_flag::break_called);
    cpu.p.set(cpu_flag::int_disable);

    return 0;
};

auto nop = [](auto&, auto) { return false; };


// Addressing modes

auto imm = [](auto& cpu) 
{
    return std::tuple{cpu.pc.advance(), 0};
};

auto zp =  [](auto& cpu) 
{
    return std::tuple{cpu.read(cpu.pc.advance()), 0};
};

auto zpx = [](auto& cpu) 
{
    auto address = cpu.read(cpu.pc.advance());
    return std::tuple{(cpu.x.value() + address) % 0x100, 0};
};

auto zpy = [](auto& cpu) 
{
    auto address = cpu.read(cpu.pc.advance());
    return std::tuple{(cpu.y.value() + address) % 0x100, 0};
};

auto abs = [](auto& cpu) 
{
    auto address = cpu.read_word(cpu.pc.advance(2));
    return std::tuple{address, 0};
};

auto abx = [](auto& cpu) 
{
    return index(cpu.read_word(cpu.pc.advance(2)), cpu.x.value());
};

auto aby = [](auto& cpu) 
{
    return index(cpu.read_word(cpu.pc.advance(2)), cpu.y.value());
};

auto ind = [](auto& cpu)
{
    auto address = cpu.read_word(cpu.pc.advance(2));
    auto lo = cpu.read(address);
    auto hi = cpu.read((address / 0x100) * 0x100 + (address + 1) % 0x100);
    return std::tuple{static_cast<std::uint16_t>((hi << 8) | lo), 0};
};

auto izx = [](auto& cpu)
{
    auto indexed = cpu.read(cpu.pc.advance()) + cpu.x.value();
    return std::tuple{cpu.read_word(indexed), 0};
};

auto izy = [](auto& cpu) 
{
    auto base = cpu.read(cpu.pc.advance());
    return index(cpu.read_word(base), cpu.y.value());
};

auto imp = [](auto& ) -> std::tuple<uint16_t,bool>
{
    throw std::logic_error("Calling operand function for implied addressing mode");
};

}

cpu::cpu(std::vector<uint8_t>& memory)
    : memory_{memory}
    , instruction_set{
        {0xEA, { nop, imp, 2 }},

        {0xA9, { lda, imm, 2 }},
        {0xA5, { lda, zp , 3 }},
        {0xB5, { lda, zpx, 4 }},
        {0xAD, { lda, abs, 4 }},
        {0xBD, { lda, abx, 4 }},
        {0xB9, { lda, aby, 4 }},
        {0xA1, { lda, izx, 6 }},
        {0xB1, { lda, izy, 5 }},

        {0x85, { sta, zp , 3 }},
        {0x95, { sta, zpx, 4 }},
        {0x8D, { sta, abs, 4 }},
        {0x9D, { sta, abx, 5 }},
        {0x99, { sta, aby, 5 }},
        {0x81, { sta, izx, 6 }},
        {0x91, { sta, izy, 6 }},

        {0xAA, { tax, imp, 2 }},
        {0x8A, { txa, imp, 2 }},
        {0xA8, { tay, imp, 2 }},
        {0x98, { tya, imp, 2 }},

        {0xBA, { tsx, imp, 2 }},
        {0x9A, { txs, imp, 2 }},
        {0x48, { pha, imp, 3 }},
        {0x68, { pla, imp, 4 }},
        {0x08, { php, imp, 3 }},
        {0x28, { plp, imp, 4 }},

        {0xA2, { ldx, imm, 2 }},
        {0xA6, { ldx, zp , 3 }},
        {0xB6, { ldx, zpy, 4 }},
        {0xAE, { ldx, abs, 4 }},
        {0xBE, { ldx, aby, 4 }},

        {0x69, { adc, imm, 2 }},
        {0x65, { adc, zp , 3 }},
        {0x75, { adc, zpx, 4 }},
        {0x6D, { adc, abs, 4 }},
        {0x7D, { adc, abx, 4 }},
        {0x79, { adc, aby, 4 }},
        {0x61, { adc, izx, 6 }},
        {0x71, { adc, izy, 5 }},

        {0xE9, { sbc, imm, 2 }},
        {0xE5, { sbc, zp , 3 }},
        {0xF5, { sbc, zpx, 4 }},
        {0xED, { sbc, abs, 4 }},
        {0xFD, { sbc, abx, 4 }},
        {0xF9, { sbc, aby, 4 }},
        {0xE1, { sbc, izx, 6 }},
        {0xF1, { sbc, izy, 5 }},

        {0xC9, { cmp, imm, 2 }},
        {0xC5, { cmp, zp , 3 }},
        {0xD5, { cmp, zpx, 4 }},
        {0xCD, { cmp, abs, 4 }},
        {0xDD, { cmp, abx, 4 }},
        {0xD9, { cmp, aby, 4 }},
        {0xC1, { cmp, izx, 6 }},
        {0xD1, { cmp, izy, 5 }},

        {0x4C, { jmp, abs, 3 }},
        {0x6C, { jmp, ind, 5 }},
        {0x20, { jsr, abs, 6 }},
        {0x60, { rts, imp, 6 }},
        {0x40, { rti, imp, 6 }},
        {0x00, { brk, imp, 7 }}
    }
{
    pc.assign(read_word(0xFFFC));
}

void cpu::tick()
{
    if (current_instruction.is_finished()) {
        auto opcode = read(pc.advance());

        current_instruction = decode(opcode)
            .value_or(cpu::instruction{
                [opcode](auto...) -> bool { throw unsupported_opcode(opcode); },
                imp
            });
    }

    current_instruction.execute(*this);
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