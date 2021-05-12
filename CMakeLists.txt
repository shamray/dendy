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

add_library(libnes
	"src/libnes/mos6502.h" 
	"src/libnes/mos6502.cpp"
)

target_include_directories(libnes PUBLIC src)
target_compile_features(libnes PUBLIC cxx_std_20)

include(Catch)
enable_testing()
add_executable(unit_tests
	"test/cpu.cpp"
)

target_link_libraries(unit_tests
	libnes
	Catch2::Catch2)

catch_discover_tests(unit_tests)