if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory.")
endif()

cmake_minimum_required(VERSION 3.10)

project(Brainshock CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

# Add any libraries
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/fmt)
include_directories(${PROJECT_SOURCE_DIR}/lib/fmt/include)

set(SDL_SHARED OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/sdl)
include_directories(${PROJECT_SOURCE_DIR}/lib/sdl/include)

include_directories(${PROJECT_SOURCE_DIR}/lib/argpars)

include_directories(${PROJECT_SOURCE_DIR}/lib/stb_image)

# Add frontend and emu lib
include_directories(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/emulator)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/frontend)