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

auto index(uint16_t base, int16_t offset)
{
    uint16_t address = int(base) + offset;
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

auto adc = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();

    adc_impl(cpu.a, cpu.a.value(), operand, cpu.p);
    return additional_cycles;
};

auto sbc = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();

    adc_impl(cpu.a, cpu.a.value(), -operand, cpu.p);
    return additional_cycles;
};

auto cmp = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();

    [[maybe_unused]]
    auto alu_result = arith_register{&cpu.p};

    adc_impl(alu_result, cpu.a.value(), -operand, cpu.p, false);
    return additional_cycles;
};

auto inc = [](auto& cpu, auto address_mode)
{
    auto am = address_mode();
    auto [operand, _] = am.load_operand();

    am.store_operand(operand + 1);
    return 0;
};

auto dec = [](auto& cpu, auto address_mode)
{
    auto am = address_mode();
    auto [operand, _] = am.load_operand();

    am.store_operand(operand - 1);
    return 0;
};

auto inx = [](auto& cpu, auto )
{
    cpu.x.assign(cpu.x.value() + 1);
    return 0;
};

auto iny = [](auto& cpu, auto )
{
    cpu.y.assign(cpu.y.value() + 1);
    return 0;
};

auto dex = [](auto& cpu, auto )
{
    cpu.x.assign(cpu.x.value() - 1);
    return 0;
};

auto dey = [](auto& cpu, auto )
{
    cpu.y.assign(cpu.y.value() - 1);
    return 0;
};

auto asl = [](auto& cpu, auto address_mode)
{
    auto am = address_mode();
    auto [operand, _] = am.load_operand();
    am.store_operand(operand << 1);
    return 0;
};

auto ana = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();
    cpu.a.assign(cpu.a.value() & operand);
    return additional_cycles;
};

auto ora = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();
    cpu.a.assign(cpu.a.value() | operand);
    return additional_cycles;
};

auto eor = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();
    cpu.a.assign(cpu.a.value() ^ operand);
    return additional_cycles;
};

auto bit = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();

    [[maybe_unused]]
    auto alu_result = arith_register{&cpu.p};
    alu_result.assign(cpu.a.value() & operand);
    cpu.p.set(cpu_flag::overflow, alu_result.value() & (1 << 6));

    return additional_cycles;
};

auto lda = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();

    cpu.a = operand;
    return additional_cycles;
};

auto ldx = [](auto& cpu, auto address_mode)
{
    auto [operand, additional_cycles] = address_mode().load_operand();
    cpu.x = operand;
    return additional_cycles;
};

auto sta = [](auto& cpu, auto address_mode)
{
    return address_mode().store_operand(cpu.a.value());
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

auto bpl = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (cpu.p.test(cpu_flag::negative)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bmi = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (!cpu.p.test(cpu_flag::negative)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bvc = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (cpu.p.test(cpu_flag::overflow)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bvs = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (!cpu.p.test(cpu_flag::overflow)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bcc = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (cpu.p.test(cpu_flag::carry)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bcs = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (!cpu.p.test(cpu_flag::carry)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto bne = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (cpu.p.test(cpu_flag::zero)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto beq = [](auto& cpu, auto address_mode)
{
    auto [address, additional_cycles] = address_mode().fetch_address();
    if (!cpu.p.test(cpu_flag::zero)) {
        return additional_cycles;
    }
    cpu.pc.assign(address);
    return additional_cycles + 1;
};

auto jmp = [](auto& cpu, auto address_mode)
{
    auto [address, _] = address_mode().fetch_address();
    cpu.pc.assign(address);
    return 0;
};

auto jsr = [](auto& cpu, auto address_mode)
{
    auto [address, _] = address_mode().fetch_address();
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

template <class cpu_t>
struct reset_vector
{
    explicit reset_vector(cpu_t& c): cpu(c){}
    cpu_t& cpu;
    auto fetch_address()
    {
        return std::tuple{cpu.read_word(0xFFFE), 0};
    }
};

auto brk = [](auto& cpu, auto _)
{
    jsr(cpu, [&cpu] {
        return reset_vector{cpu};
    });
    php(cpu, _);
    cpu.p.set(cpu_flag::break_called);
    cpu.p.set(cpu_flag::int_disable);

    return 0;
};

auto nop = [](auto&, auto) { return false; };


// Addressing modes

template <class fettch_addr_t>
struct memory_based_address_mode
{
    memory_based_address_mode(cpu& c, fettch_addr_t fetch_addr) : cpu_(c), fetch_addr_(fetch_addr) {}

    [[nodiscard]]
    auto load_operand()
    {
        fetch_address();
        auto [address, additional_cycles] = result_.value();
        return std::tuple{cpu_.read(address), additional_cycles};
    }

    auto store_operand(uint8_t operand)
    {
        fetch_address();
        auto [address, additional_cycles] = result_.value();
        cpu_.write(address, operand);
        return additional_cycles;
    }

    auto fetch_address()
    {
        if (!result_)
            result_ = fetch_addr_(cpu_);
        return result_.value();
    }

private:
    cpu& cpu_;
    std::optional<std::tuple<uint16_t, int>> result_;
    fettch_addr_t fetch_addr_;
};

struct implied_address_mode {};

auto imm = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu)
    {
        return std::tuple{cpu.pc.advance(), 0};
    }};
};

auto zp =  [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        return std::tuple{cpu.read(cpu.pc.advance()), 0};
    }};
};

auto zpx = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto address = cpu.read(cpu.pc.advance());
        return std::tuple{(cpu.x.value() + address) % 0x100, 0};
    }};
};

auto zpy = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto address = cpu.read(cpu.pc.advance());
        return std::tuple{(cpu.y.value() + address) % 0x100, 0};
    }};
};

auto abs = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto address = cpu.read_word(cpu.pc.advance(2));
        return std::tuple{address, 0};
    }};
};

auto abx = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        return index(cpu.read_word(cpu.pc.advance(2)), cpu.x.value());
    }};
};

auto aby = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        return index(cpu.read_word(cpu.pc.advance(2)), cpu.y.value());
    }};
};

auto ind = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto address = cpu.read_word(cpu.pc.advance(2));
        auto lo = cpu.read(address);
        auto hi = cpu.read((address / 0x100) * 0x100 + (address + 1) % 0x100);
        return std::tuple{static_cast<std::uint16_t>((hi << 8) | lo), 0};
    }};
};

auto izx = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto indexed = cpu.read(cpu.pc.advance()) + cpu.x.value();
        return std::tuple{cpu.read_word(indexed), 0};
    }};
};

auto izy = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto base = cpu.read(cpu.pc.advance());
        return index(cpu.read_word(base), cpu.y.value());
    }};
};

auto rel = [](auto& cpu)
{
    return memory_based_address_mode{cpu, [](auto& cpu )
    {
        auto instruction_address = cpu.pc.value() - 1;
        auto offset = cpu.read_signed(cpu.pc.advance());
        return index(instruction_address, offset);
    }};
};

class accumulator_address_mode
{
public:
    accumulator_address_mode(cpu& c) : cpu_(c) {}

    [[nodiscard]]
    auto load_operand() const
    {
        return std::tuple{cpu_.a.value(), 0};
    }

    auto store_operand(uint8_t operand)
    {
        cpu_.a.assign(operand);
        return 0;
    }

private:
    cpu& cpu_;
};

auto acc = [](auto& cpu)
{
    return accumulator_address_mode{cpu};
};

auto imp = [](auto& )
{
    return implied_address_mode{};
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

        {0xE6, { inc, zp , 5 }},
        {0xF6, { inc, zpx, 6 }},
        {0xEE, { inc, abs, 6 }},
        {0xFE, { inc, abx, 7 }},

        {0xC6, { dec, zp , 5 }},
        {0xD6, { dec, zpx, 6 }},
        {0xCE, { dec, abs, 6 }},
        {0xDE, { dec, abx, 7 }},

        {0xE8, { inx, imp, 2 }},
        {0xC8, { iny, imp, 2 }},

        {0xCA, { dex, imp, 2 }},
        {0x88, { dey, imp, 2 }},

        {0x0A, { asl, acc, 2 }},
        {0x06, { asl, zp , 5 }},
        {0x16, { asl, zpx, 6 }},
        {0x0E, { asl, abs, 6 }},
        {0x1E, { asl, abx, 7 }},

        {0x29, { ana, imm, 2 }},
        {0x25, { ana, zp , 3 }},
        {0x35, { ana, zpx, 4 }},
        {0x2D, { ana, abs, 4 }},
        {0x3D, { ana, abx, 4 }},
        {0x39, { ana, aby, 4 }},
        {0x21, { ana, izx, 6 }},
        {0x31, { ana, izy, 5 }},

        {0x09, { ora, imm, 2 }},
        {0x05, { ora, zp , 3 }},
        {0x15, { ora, zpx, 4 }},
        {0x0D, { ora, abs, 4 }},
        {0x1D, { ora, abx, 4 }},
        {0x19, { ora, aby, 4 }},
        {0x01, { ora, izx, 6 }},
        {0x11, { ora, izy, 5 }},

        {0x49, { eor, imm, 2 }},
        {0x45, { eor, zp , 3 }},
        {0x55, { eor, zpx, 4 }},
        {0x4D, { eor, abs, 4 }},
        {0x5D, { eor, abx, 4 }},
        {0x59, { eor, aby, 4 }},
        {0x41, { eor, izx, 6 }},
        {0x51, { eor, izy, 5 }},

        {0x24, { bit, zp,  3 }},
        {0x2C, { bit, abs, 4 }},

        {0x10, { bpl, rel, 2 }},
        {0x30, { bmi, rel, 2 }},
        {0x50, { bvc, rel, 2 }},
        {0x70, { bvs, rel, 2 }},
        {0x90, { bcc, rel, 2 }},
        {0xB0, { bcs, rel, 2 }},
        {0xD0, { bne, rel, 2 }},
        {0xF0, { beq, rel, 2 }},

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
                [opcode](auto...) -> int { throw unsupported_opcode(opcode); },
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