#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <bitset>
#include <iostream>
#include <cassert>

namespace nes
{

enum class cpu_flag
{
    carry = 0,
    zero = 1,
    int_disable = 2,
    decimal = 3,
    break_called = 4,
    _ [[maybe_unused]] = 5,
    overflow = 6,
    negative = 7
};

class flags_register
{
    constexpr static auto pos(cpu_flag f) { return static_cast<size_t>(f); }

public:
    void set(cpu_flag f, bool value = true) { bits_.set(pos(f), value); }
    void reset(cpu_flag f) { bits_.reset(pos(f)); }

    [[nodiscard]]
    auto test(cpu_flag f) const { return bits_.test(pos(f)); }

private:
    std::bitset<8> bits_;
};

class arith_register
{
public:
    arith_register() = default;
    explicit arith_register(flags_register* f) : flags_{f} {}
    explicit arith_register(uint8_t val, flags_register* f = nullptr) : val_{val}, flags_{f} {}

    operator uint8_t() const { return val_; } // NOLINT(google-explicit-constructor)

    arith_register& operator=(uint8_t new_val)
    {
        val_ = new_val;
        if (flags_) {
            flags_->set(cpu_flag::zero, val_ == 0);
            flags_->set(cpu_flag::negative, (val_ & 0x80) != 0);
        }
        return *this;
    }

private:
    uint8_t val_{0};
    flags_register* flags_{nullptr};
};

inline std::ostream& operator<< (std::ostream& s, const arith_register& r)
{
    return s << static_cast<int>(r);
}

using program_counter = uint16_t;
using stack_register = uint8_t;

class cpu
{
public:
    explicit cpu(std::vector<uint8_t>& memory);

    program_counter pc;
    
    stack_register s{0xFD};
    flags_register p;

    arith_register a{&p};
    arith_register x{&p};
    arith_register y{&p};


    struct instruction
    {
        using fetch_address = std::function< uint16_t (cpu&) >;
        using command       = std::function< void (cpu&, fetch_address) >;

        command         operation;
        fetch_address   fetch_operand_address;
        int             cycles {1};
        int             additional_cycles {0};
    };

    class executable_instruction: instruction
    {
    public:
        executable_instruction() = default;
        explicit
        executable_instruction(const instruction& from)
            : instruction(from)
            , c_{cycles}
        {
            assert(c_ != 0);
        }

        executable_instruction& operator=(const instruction& from)
        {
            return *this = executable_instruction{from};
        }

        void execute(cpu& cpu)
        {
            if (is_finished())
                return;

            if (--c_ == 0) {
                operation(cpu, fetch_operand_address);
            }
        }

        [[nodiscard]] bool is_finished() const { return c_ == 0; }

    private:
        int c_{0};
    };

    void tick();
    void tick(int count) { for (auto i = 0; i < count; ++i) tick(); }

    void write(uint16_t addr, uint8_t value) const { memory_[addr] = value; }

    [[nodiscard]] auto read(uint16_t addr) const { return memory_[addr]; }
    [[nodiscard]] auto read_word(uint16_t addr) const -> uint16_t;

    auto decode(uint8_t opcode) -> std::optional<instruction>;

private:
    std::vector<uint8_t>& memory_;
    executable_instruction current_instruction;
    std::unordered_map<uint8_t, cpu::instruction> instruction_set;
};

}