add_library(libnes
    libnes/color.hpp
    libnes/clock.hpp
    libnes/literals.hpp

    libnes/cpu.hpp
    libnes/cpu.cpp
    libnes/cpu_registers.hpp
    libnes/cpu_address_modes.hpp
    libnes/cpu_operations.hpp

    libnes/ppu.hpp
    libnes/ppu.cpp
    libnes/ppu_name_table.hpp)

target_include_directories(libnes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(libnes PUBLIC cxx_std_20)

add_executable(nemo_sdl WIN32
    nemo_sdl/main.cpp
)

target_link_libraries(nemo_sdl
    libnes
    SDL2::SDL2
)
