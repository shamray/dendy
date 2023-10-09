#include <libnes/cpu.hpp>

#include <string>

using namespace std::string_literals;

namespace nes
{

unsupported_opcode::unsupported_opcode(std::uint8_t opcode)
    : std::runtime_error("Unsupported opcode: "s + std::to_string(opcode)) {}
}