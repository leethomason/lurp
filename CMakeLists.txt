cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(lurp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        e57ca2e3685b160617d3d95fcd9e789c4e06ca88 # 10.1.0
)
FetchContent_Declare(
  argh
  GIT_REPOSITORY https://github.com/adishavit/argh.git
  GIT_TAG        d17062c870b5919f6d1eae7fe12879869a893b32
)
FetchContent_Declare(
  lua
  URL      https://www.lua.org/ftp/lua-5.4.6.tar.gz
  URL_HASH SHA256=7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88
)
FetchContent_Declare(
  ionic
  EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/leethomason/ionic.git
  GIT_TAG        74cc38510f5054d1475530b591351630adacd19a
)
FetchContent_Declare(
  plog
  GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
  GIT_TAG        199734337bec45e72956c4e82f848a716cbf7ac3
)
FetchContent_Declare(
  md4c
  EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/mity/md4c.git
  GIT_TAG        481fbfbdf72daab2912380d62bb5f2187d438408
)
FetchContent_Declare(
    enkiTS
    EXCLUDE_FROM_ALL
    GIT_REPOSITORY  https://github.com/dougbinks/enkiTS.git
    GIT_TAG         6ffccbdb1000253d8d513dd7a5ae9226e5023a5c   
)
FetchContent_Declare(
    nuklear
    EXCLUDE_FROM_ALL
    GIT_REPOSITORY  https://github.com/Immediate-Mode-UI/Nuklear.git
    GIT_TAG         37e54da201cc5aad78ebcf621f6dc161407f4713
)
# FIXME: windws & MSVC only
FetchContent_Declare(
    SDL2
    URL         https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-devel-2.30.3-VC.zip
)
# FIXME: windws & MSVC only
FetchContent_Declare(
    SDL2_IMAGE
    URL      https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-devel-2.8.2-VC.zip
)
# FIXME: windws & MSVC only
FetchContent_Declare(
    SDL2_TTF
    URL      https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-devel-2.22.0-VC.zip
)
FetchContent_MakeAvailable(lua fmt argh ionic plog md4c enkiTS nuklear SDL2 SDL2_image SDL2_ttf)

file(GLOB SOURCES "src/*.cpp" "src/*.c")
file(GLOB HEADERS "src/*.h")
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
file(GLOB IONIC_SOURCES ${ionic_SOURCE_DIR}/src/*.cpp ${ionic_SOURCE_DIR}/src/*.h)
set(MD4C_SOURCES ${md4c_SOURCE_DIR}/src/md4c.c ${md4c_SOURCE_DIR}/src/md4c.h)
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/src/lua.c")
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/src/luac.c")
set(ENKITS_SOURCES ${enkiTS_SOURCE_DIR}/src/TaskScheduler.cpp ${enkiTS_SOURCE_DIR}/src/TaskScheduler.h)
set(NUKLEAR_SOURCES ${nuklear_SOURCE_DIR}/nuklear.h ${nuklear_SOURCE_DIR}/demo/sdl_renderer/nuklear_sdl_renderer.h)

source_group("lua" FILES ${LUA_SOURCES})

enable_testing()

# ------- LuRP ---------------
file(GLOB CONSOLE_SRC "drivers/console/*.cpp" "drivers/console/*.h" "drivers/*.cpp" "drivers/*.h")
source_group("console" FILES ${CONSOLE_SRC})

add_executable(lurp 
  ${SOURCES} 
  ${HEADERS} 
  ${fmt_SOURCE_DIR}/src/format.cc 
  ${LUA_SOURCES} 
  ${CONSOLE_SRC} 
  ${IONIC_SOURCES}
  ${MD4C_SOURCES}
)

target_include_directories(lurp PUBLIC src 
  ${argh_SOURCE_DIR} ${fmt_SOURCE_DIR}/include 
  ${ionic_SOURCE_DIR}/include 
  ${lua_SOURCE_DIR}/src 
  ${plog_SOURCE_DIR}/include
  ${md4c_SOURCE_DIR}/src
)
set_target_properties(lurp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "..")

add_test(
  NAME lurp
  COMMAND lurp --log info --noScan
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(lurp PROPERTIES PASS_REGULAR_EXPRESSION "LuRP tests run successfully")

# ------- Lua ---------------

if (MSVC)
  add_executable(lua ${LUA_SOURCES} ${lua_SOURCE_DIR}/src/lua.c)
  target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
endif()

# ------- LuRP2D ---------------
file(GLOB TWOD_SRC "drivers/twod/*.cpp" "drivers/twod/*.h" "drivers/*.cpp" "drivers/*.h")
source_group("twoD" FILES ${TWOD_SRC})

list(APPEND CMAKE_PREFIX_PATH ${sdl2_SOURCE_DIR}/cmake ${sdl2_image_SOURCE_DIR}/cmake ${sdl2_ttf_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# https://stackoverflow.com/questions/28395833/using-sdl2-with-cmake
#find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
#find_library(SDL2_LIBRARY NAME SDL2)

add_executable(lurp2d 
  ${SOURCES} 
  ${HEADERS} 
  ${fmt_SOURCE_DIR}/src/format.cc 
  ${LUA_SOURCES} 
  ${MD4C_SOURCES}
  ${TWOD_SRC}
  ${ENKITS_SOURCES}
  ${NUKLEAR_SOURCES}
)

#FIXME: windws & MSVC only
# Gettnig the SDL2/SDL.h working is a giant PITA. I don't see how to do that at all
# with the given directory structure. Create a duplicate header directory that can
# be included in the project.
file(COPY ${SDL2_INCLUDE_DIRS}/SDL.h DESTINATION ${FETCHCONTENT_BASE_DIR}/SDL2/SDL2)

target_include_directories(lurp2d PUBLIC src 
  ${argh_SOURCE_DIR} ${fmt_SOURCE_DIR}/include 
  ${ionic_SOURCE_DIR}/include 
  ${lua_SOURCE_DIR}/src 
  ${plog_SOURCE_DIR}/include
  ${md4c_SOURCE_DIR}/src
  ${SDL2_INCLUDE_DIRS}              # Reasonable place
  ${FETCHCONTENT_BASE_DIR}/SDL2     # Duplicate of SDL2_INCLUDE_DIRS in a weird but required place
  ${SDL2_image_SOURCE_DIR}/include
  ${SDL2_ttf_SOURCE_DIR}/include
  ${enkiTS_SOURCE_DIR}/src
  ${nuklear_SOURCE_DIR}
  ${nuklear_SOURCE_DIR}/demo/sdl_renderer
)

target_link_libraries(lurp2d SDL2::SDL2main)
target_link_libraries(lurp2d SDL2::SDL2)
target_link_libraries(lurp2d SDL2_image::SDL2_image)
target_link_libraries(lurp2d SDL2_ttf::SDL2_ttf)

set_target_properties(lurp2d PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "..")
set_target_properties(lurp2d PROPERTIES VS_DPI_AWARE "PerMonitor")

# FIXME: windws & MSVC only
# copy the dll to the working directory
file(COPY ${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll DESTINATION ${CMAKE_HOME_DIRECTORY})
file(COPY ${sdl2_image_SOURCE_DIR}/lib/x64/SDL2_image.dll DESTINATION ${CMAKE_HOME_DIRECTORY})
file(COPY ${sdl2_ttf_SOURCE_DIR}/lib/x64/SDL2_ttf.dll DESTINATION ${CMAKE_HOME_DIRECTORY})

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
