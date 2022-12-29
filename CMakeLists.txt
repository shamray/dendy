cmake_minimum_required (VERSION 3.20)

project ("NEM0")

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
	message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
	file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
		"${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include("${CMAKE_BINARY_DIR}/conan.cmake")
conan_cmake_configure(
	REQUIRES
		catch2/[>=3.0]
		sdl/[>=2.0.14]
	GENERATORS
		cmake_find_package
)
conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE . BUILD missing SETTINGS ${settings})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
find_package(Catch2 3 REQUIRED)
find_package(SDL2 REQUIRED)

include(Catch)
enable_testing()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(tools)