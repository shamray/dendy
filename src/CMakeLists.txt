add_library(libnes
    libnes/color.hpp
    libnes/literals.hpp

    libnes/console.hpp
    libnes/cartridge.hpp

    libnes/cpu.hpp
    libnes/cpu.cpp
    libnes/cpu_registers.hpp
    libnes/cpu_address_modes.hpp
    libnes/cpu_operations.hpp

    libnes/ppu.hpp
    libnes/ppu.cpp
    libnes/ppu_crt_scan.hpp
    libnes/ppu_name_table.hpp
    libnes/ppu_object_attribute_memory.hpp
    libnes/ppu_palette_table.hpp
    libnes/screen.hpp

    libnes/mappers/nrom.hpp
    libnes/mappers/mmc1.hpp
)

target_include_directories(libnes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(libnes PUBLIC cxx_std_20)

target_compile_options(libnes PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

add_executable(nemo_sdl WIN32
    nemo_sdl/main.cpp
)

target_link_libraries(nemo_sdl
    libnes
    ${SDL2_LIBRARIES}
)
