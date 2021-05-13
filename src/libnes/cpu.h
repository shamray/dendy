#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <bitset>

namespace nes
{

enum class cpu_flag
{
    carry = 0,
    zero = 1,
    int_disable = 2,
    decimal = 3,
    break1 = 4,
    break2 = 5,
    overflow = 6,
    negative = 7
};

class flags_register
{
    constexpr static auto pos(cpu_flag f) { return static_cast<size_t>(f); }

public:
    void set(cpu_flag f, bool value = true) { bits_.set(pos(f), value); }
    void reset(cpu_flag f) { bits_.reset(pos(f)); }
    auto test(cpu_flag f) { return bits_.test(pos(f)); }

private:
    std::bitset<8> bits_;
};

class arith_register
{
public:
    arith_register() = default;
    arith_register(flags_register* f) : flags_{f} {}
    arith_register(uint8_t val, flags_register* f = nullptr) : val_{val} {}

    operator uint8_t() const { return val_; }

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

using program_counter = uint16_t;
using stack_register = uint8_t;

class cpu
{
public:
    cpu(std::vector<uint8_t>& memory);

    program_counter pc;
    
    stack_register s{0};
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