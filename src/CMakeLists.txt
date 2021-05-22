add_library(libnes
    libnes/cpu.hpp
    libnes/cpu.cpp
    libnes/cpu_registers.hpp
)

target_include_directories(libnes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(libnes PUBLIC cxx_std_20)