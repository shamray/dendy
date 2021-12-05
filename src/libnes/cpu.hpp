#pragma once

#include <libnes/cpu_address_modes.hpp>
#include <libnes/cpu_operations.hpp>
#include <libnes/cpu_registers.hpp>

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <concepts>

namespace nes
{

template <class B>
concept bus = requires(B b, std::uint16_t address, std::uint8_t value) {
    { b.write(address, value) };
    { b.read(address) } -> std::same_as<std::uint8_t>;
    { b.nmi() } -> std::same_as<bool>;
};

template <bus bus_t>
class cpu
{
public:
    explicit cpu(bus_t& bus);

    program_counter pc;
    
    stack_register s{0x0100, 0xFD};
    flags_register p;

    arith_register a{p};
    arith_register x{p};
    arith_register y{p};

    struct instruction
    {
        instruction(auto operation, auto address_mode, int cycles = 1)
            : command_{[operation, address_mode](auto& cpu){ return operation(cpu, address_mode(cpu)); }}
            , c_{cycles}
        {}
        instruction() = default;

        void execute(cpu& cpu) {
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

        [[nodiscard]] bool is_finished() const noexcept { return c_ == 0 && ac_ == 0; }

//        [[nodiscard]] auto save_state() const { return std::tuple{command_, c_, ac_}; }

//        void load_state(std::function<int(cpu&)> command, int c, int ac) { command_ = command; c_ = c; ac_ = ac; }

    private:
        std::function<int(cpu&)> command_;
        int c_{0};
        int ac_{0};
    };

    struct state
    {
        std::uint16_t pc;

        std::uint8_t s;
        std::uint8_t p;

        std::uint8_t a;
        std::uint8_t x;
        std::uint8_t y;

        instruction cix;
    };


    void tick();
    auto is_executing() { return !current_instruction.is_finished();}

    void write(std::uint16_t addr, std::uint8_t value) const { bus_.write(addr, value); }

    [[nodiscard]] auto read(std::uint16_t addr) const { return bus_.read(addr); }
    [[nodiscard]] auto read_signed(std::uint16_t addr) const { return static_cast<std::int8_t>(read(addr)); }
    [[nodiscard]] auto read_word(std::uint16_t addr) const -> std::uint16_t;
    [[nodiscard]] auto read_word_wrapped(std::uint16_t addr) const -> std::uint16_t;

    auto decode(std::uint8_t opcode) -> instruction;

    auto interrupt() -> int;

    [[nodiscard]] auto save_state() const -> state;
    void load_state(state state);


private:
    class hasher
    {
    public:
        [[nodiscard]] constexpr auto operator()(std::uint8_t v) const noexcept {
            return v;
        }
    };
    bus_t& bus_;
    instruction current_instruction;
    static const std::unordered_map<std::uint8_t, cpu::instruction, hasher> instruction_set;
};


class unsupported_opcode : public std::runtime_error
{
public:
    explicit unsupported_opcode(std::uint8_t opcode);
};


template <bus bus_t> cpu<bus_t>::cpu(bus_t& bus)
    : bus_{bus}
{
    pc.assign(read_word(0xFFFC));
}

template <bus bus_t> void cpu<bus_t>::tick()
{
    if (current_instruction.is_finished()) { [[likely]]
        if (not bus_.nmi()) {

            auto opcode = read(pc.advance());
            current_instruction = decode(opcode);

        } else {
            current_instruction = cpu::instruction{
                [this](auto...) -> int { return interrupt(); },
                imp
            };
        }
    }

    current_instruction.execute(*this);
}

template <bus bus_t> auto cpu<bus_t>::read_word(std::uint16_t addr) const -> std::uint16_t {
    auto lo = read(addr);
    auto hi = read(addr + 1);

    return (hi << 8) | lo;
}

template <bus bus_t> auto cpu<bus_t>::read_word_wrapped(std::uint16_t addr) const -> std::uint16_t {
    auto lo = read(addr);
    auto hi = read((addr / 0x100) * 0x100 + (addr + 1) % 0x100);
    return (hi << 8) | lo;
}

template <bus bus_t> auto cpu<bus_t>::decode(std::uint8_t opcode) -> instruction {
    auto found = instruction_set.find(opcode);
    if (found == std::end(instruction_set))
        return instruction{
            [opcode](auto...) -> int { throw unsupported_opcode(opcode); },
            imp
        };

    return found->second;
}

template<bus bus_t>
auto cpu<bus_t>::interrupt() -> int {
    write(s.push(), pc.hi());
    write(s.push(), pc.lo());
    pc.assign(read_word(0xFFFA));

    write(s.push(), p.value());
    p.set(cpu_flag::int_disable);

    return 7;
}

template <bus bus_t>
auto cpu<bus_t>::save_state() const -> state
{
    return state{
        pc.value(),
        s.value(),
        p.value(),
        a.value(),
        x.value(),
        y.value(),
        current_instruction
    };
}

template <bus bus_t>
void  cpu<bus_t>::load_state(state state)
{
    pc.assign(state.pc);
    s.assign(state.s);
    p.assign(state.p);
    a.assign(state.a);
    x.assign(state.x);
    y.assign(state.y);
    current_instruction = state.cix;
}

template <bus bus_t>
const std::unordered_map<std::uint8_t, typename cpu<bus_t>::instruction, typename cpu<bus_t>::hasher> cpu<bus_t>::instruction_set{
    {0xEA, { nop, imp, 2 }},

    {0x1A, { nop, imp, 2 }},
    {0x3A, { nop, imp, 2 }},
    {0x5A, { nop, imp, 2 }},
    {0x7A, { nop, imp, 2 }},
    {0xDA, { nop, imp, 2 }},
    {0xFA, { nop, imp, 2 }},
    {0x82, { nop, imp, 2 }},

    {0x04, { i_n, zp , 3 }},
    {0x44, { i_n, zp , 3 }},
    {0x64, { i_n, zp , 3 }},
    {0x0C, { i_n, abs, 4 }},

    {0x14, { i_n, zpx, 4 }},
    {0x34, { i_n, zpx, 4 }},
    {0x54, { i_n, zpx, 4 }},
    {0x74, { i_n, zpx, 4 }},
    {0xD4, { i_n, zpx, 4 }},
    {0xF4, { i_n, zpx, 4 }},

    {0x1C, { i_n, abx, 4 }},
    {0x3C, { i_n, abx, 4 }},
    {0x5C, { i_n, abx, 4 }},
    {0x7C, { i_n, abx, 4 }},
    {0xDC, { i_n, abx, 4 }},
    {0xFC, { i_n, abx, 4 }},

    {0x80, { i_n, imm, 2 }},
    {0x89, { i_n, imm, 2 }},

    {0xA7, { lax, zp , 3 }},
    {0xB7, { lax, zpy, 4 }},
    {0xAF, { lax, abs, 4 }},
    {0xBF, { lax, aby, 4 }},
    {0xA3, { lax, izx, 6 }},
    {0xB3, { lax, izy, 5 }},

    {0x87, { sax, zp , 3 }},
    {0x97, { sax, zpy, 4 }},
    {0x8F, { sax, abs, 4 }},
    {0x83, { sax, izx, 6 }},

    {0xC7, { dcp, zp , 5 }},
    {0xD7, { dcp, zpx, 6 }},
    {0xCF, { dcp, abs, 6 }},
    {0xDF, { dcp, abx, 7 }},
    {0xDB, { dcp, aby, 7 }},
    {0xC3, { dcp, izx, 8 }},
    {0xD3, { dcp, izy, 8 }},

    {0xE7, { isc, zp , 5 }},
    {0xF7, { isc, zpx, 6 }},
    {0xEF, { isc, abs, 6 }},
    {0xFF, { isc, abx, 7 }},
    {0xFB, { isc, aby, 7 }},
    {0xE3, { isc, izx, 8 }},
    {0xF3, { isc, izy, 8 }},

    {0x07, { slo, zp , 5 }},
    {0x17, { slo, zpx, 6 }},
    {0x0F, { slo, abs, 6 }},
    {0x1F, { slo, abx, 7 }},
    {0x1B, { slo, aby, 7 }},
    {0x03, { slo, izx, 8 }},
    {0x13, { slo, izy, 8 }},

    {0x47, { sre, zp , 5 }},
    {0x57, { sre, zpx, 6 }},
    {0x4F, { sre, abs, 6 }},
    {0x5F, { sre, abx, 7 }},
    {0x5B, { sre, aby, 7 }},
    {0x43, { sre, izx, 8 }},
    {0x53, { sre, izy, 8 }},

    {0x27, { rla, zp , 5 }},
    {0x37, { rla, zpx, 6 }},
    {0x2F, { rla, abs, 6 }},
    {0x3F, { rla, abx, 7 }},
    {0x3B, { rla, aby, 7 }},
    {0x23, { rla, izx, 8 }},
    {0x33, { rla, izy, 8 }},

    {0x67, { rra, zp , 5 }},
    {0x77, { rra, zpx, 6 }},
    {0x6F, { rra, abs, 6 }},
    {0x7F, { rra, abx, 7 }},
    {0x7B, { rra, aby, 7 }},
    {0x63, { rra, izx, 8 }},
    {0x73, { rra, izy, 8 }},

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

    {0xA2, { ldx, imm, 2 }},
    {0xA6, { ldx, zp , 3 }},
    {0xB6, { ldx, zpy, 4 }},
    {0xAE, { ldx, abs, 4 }},
    {0xBE, { ldx, aby, 4 }},

    {0x86, { stx, zp , 3 }},
    {0x96, { stx, zpy, 4 }},
    {0x8E, { stx, abs, 4 }},

    {0xA0, { ldy, imm, 2 }},
    {0xA4, { ldy, zp , 3 }},
    {0xB4, { ldy, zpx, 4 }},
    {0xAC, { ldy, abs, 4 }},
    {0xBC, { ldy, abx, 4 }},

    {0x84, { sty, zp , 3 }},
    {0x94, { sty, zpx, 4 }},
    {0x8C, { sty, abs, 4 }},

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

    {0x69, { adc, imm, 2 }},
    {0x65, { adc, zp , 3 }},
    {0x75, { adc, zpx, 4 }},
    {0x6D, { adc, abs, 4 }},
    {0x7D, { adc, abx, 4 }},
    {0x79, { adc, aby, 4 }},
    {0x61, { adc, izx, 6 }},
    {0x71, { adc, izy, 5 }},

    {0xE9, { sbc, imm, 2 }},
    {0xEB, { sbc, imm, 2 }},
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

    {0xE0, { cpx, imm, 2 }},
    {0xE4, { cpx, zp , 3 }},
    {0xEC, { cpx, abs, 4 }},

    {0xC0, { cpy, imm, 2 }},
    {0xC4, { cpy, zp , 3 }},
    {0xCC, { cpy, abs, 4 }},

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

    {0x4A, { lsr, acc, 2 }},
    {0x46, { lsr, zp , 5 }},
    {0x56, { lsr, zpx, 6 }},
    {0x4E, { lsr, abs, 6 }},
    {0x5E, { lsr, abx, 7 }},

    {0x2A, { rol, acc, 2 }},
    {0x26, { rol, zp , 5 }},
    {0x36, { rol, zpx, 6 }},
    {0x2E, { rol, abs, 6 }},
    {0x3E, { rol, abx, 7 }},

    {0x6A, { ror, acc, 2 }},
    {0x66, { ror, zp , 5 }},
    {0x76, { ror, zpx, 6 }},
    {0x6E, { ror, abs, 6 }},
    {0x7E, { ror, abx, 7 }},

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

    {0x18, { clc, imp, 2 }},
    {0x38, { sec, imp, 2 }},
    {0xD8, { cld, imp, 2 }},
    {0xF8, { sed, imp, 2 }},
    {0x58, { cli, imp, 2 }},
    {0x78, { sei, imp, 2 }},
    {0xB8, { clv, imp, 2 }},

    {0x4C, { jmp, abs, 3 }},
    {0x6C, { jmp, ind, 5 }},
    {0x20, { jsr, abs, 6 }},
    {0x60, { rts, imp, 6 }},
    {0x40, { rti, imp, 6 }},
    {0x00, { brk, imp, 7 }}
};

}