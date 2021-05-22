cmake_minimum_required (VERSION 3.20)

project ("dendy")

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
	message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
	file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
		"${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include("${CMAKE_BINARY_DIR}/conan.cmake")
conan_cmake_configure(REQUIRES catch2/[>=2.13] GENERATORS cmake_find_package)
conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE . BUILD missing REMOTE conan-center SETTINGS ${settings})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
find_package(Catch2 REQUIRED)

include(Catch)
enable_testing()

add_library(libnes
	src/libnes/cpu.hpp
	src/libnes/cpu.cpp
	src/libnes/cpu_registers.hpp
)

target_include_directories(libnes PUBLIC src)
target_compile_features(libnes PUBLIC cxx_std_20)

add_executable(unit_tests
	test/unit_tests/cpu_test.cpp
)

target_link_libraries(unit_tests
	libnes
	Catch2::Catch2)

catch_discover_tests(unit_tests)

add_executable(integration_tests
	test/integration_tests/nestest.cpp
)

target_link_libraries(integration_tests
	libnes
	Catch2::Catch2)

add_custom_command(
	TARGET integration_tests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_SOURCE_DIR}/rom"
	"${CMAKE_CURRENT_BINARY_DIR}/rom"
)

catch_discover_tests(integration_tests)