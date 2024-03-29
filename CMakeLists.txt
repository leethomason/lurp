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
  GIT_REPOSITORY https://github.com/leethomason/ionic.git
  GIT_TAG        a0c225c53092281a5f86c6c7f37a574da1f5e968
)
FetchContent_Declare(
  plog
  GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
  GIT_TAG        199734337bec45e72956c4e82f848a716cbf7ac3
)
FetchContent_MakeAvailable(lua fmt argh ionic plog)

file(GLOB CONSOLE_SRC "drivers/console/*.cpp" "drivers/console/*.h" "drivers/*.cpp" "drivers/*.h")
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
file(GLOB IONIC_SOURCES ${ionic_SOURCE_DIR}/src/*.cpp ${ionic_SOURCE_DIR}/src/*.h)
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/src/lua.c")
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/src/luac.c")
source_group("lua" FILES ${LUA_SOURCES})
source_group("console" FILES ${CONSOLE_SRC})

enable_testing()

add_executable(lurp ${SOURCES} ${HEADERS} ${fmt_SOURCE_DIR}/src/format.cc ${LUA_SOURCES} ${CONSOLE_SRC} ${IONIC_SOURCES})
target_include_directories(lurp PUBLIC src ${argh_SOURCE_DIR} ${fmt_SOURCE_DIR}/include ${ionic_SOURCE_DIR}/include ${lua_SOURCE_DIR}/src ${plog_SOURCE_DIR}/include)
set_target_properties(lurp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "..")

add_test(
  NAME lurp
  COMMAND lurp --log info
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(lurp PROPERTIES PASS_REGULAR_EXPRESSION "LuRP tests run successfully")

add_executable(lua ${LUA_SOURCES} ${lua_SOURCE_DIR}/src/lua.c)
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

install(TARGETS lurp
        EXPORT lurp
        #LIBRARY DESTINATION lib
        #ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        #INCLUDES DESTINATION include
        )
