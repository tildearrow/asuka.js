cmake_minimum_required(VERSION 3.0)

project(asuka)

set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_VIDEO OFF)
set(SDL_RENDER OFF)
set(SDL_EVENTS OFF)
set(SDL_POWER OFF)
set(SDL_TIMERS OFF)
set(SDL_FILE OFF)
set(SDL_FILESYSTEM OFF)
set(SDL_SENSOR OFF)

set(DISKAUDIO OFF)
set(DUMMYAUDIO OFF)
add_subdirectory(SDL2)

set(BUILD_TESTING OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_PROGRAMS OFF)
add_subdirectory(libsndfile)
include_directories(libsndfile/include)
include_directories(SDL2/include)

add_library(iamtired STATIC src/iamtired.c)

add_executable(asuka src/objman.cpp src/sizeman.cpp src/netfile.cpp src/music.cpp src/sound.cpp src/asuka.cpp)
target_link_libraries(asuka SDL2-static sndfile iamtired)

set(CMAKE_CXX_FLAGS "--bind")
set_target_properties(asuka PROPERTIES LINK_FLAGS "-s FETCH=1 --bind -s ALLOW_MEMORY_GROWTH=1 -s ELIMINATE_DUPLICATE_FUNCTIONS=1 -s FILESYSTEM=0 -s EXPORT_NAME=\"'Asuka'\" --closure 1")

# --closure 1
