add_executable(unit_tests
    unit_tests/cpu_test.cpp
    unit_tests/clock_test.cpp
    unit_tests/ppu_test.cpp
    unit_tests/main.cpp
)

target_link_libraries(unit_tests
    libnes
    Catch2::Catch2
)

catch_discover_tests(unit_tests)

add_executable(integration_tests
    integration_tests/nestest.cpp
)

target_link_libraries(integration_tests
    libnes
    Catch2::Catch2
)

add_custom_command(
    TARGET integration_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/rom"
    "${CMAKE_CURRENT_BINARY_DIR}/rom"
)

catch_discover_tests(integration_tests)