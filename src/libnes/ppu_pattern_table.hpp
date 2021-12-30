#pragma once

#include <libnes/literals.hpp>

#include <array>
#include <stdexcept>

namespace nes {

class pattern_table
{
public:
    using memory_bank = std::array<std::uint8_t, 8_Kb>;

    pattern_table() = default;
    explicit pattern_table(const memory_bank* bank)
        : bank_{bank}
    {}

    constexpr void connect(const memory_bank* new_bank) {
        bank_ = new_bank;
    }

    [[nodiscard]] constexpr auto is_connected() const { return bank_ != nullptr; }

    [[nodiscard]] constexpr auto read(std::uint16_t addr) const {
        if (bank_ == nullptr)
            throw std::range_error("No CHR bank connected");

        return bank_->at(addr);
    }
    constexpr void write(std::uint16_t, std::uint8_t )    {}

private:
    const memory_bank* bank_{nullptr};
};

}
