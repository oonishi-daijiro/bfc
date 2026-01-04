cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(bfc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SAN "Enable sanitizer" OFF)

find_package(LLVM REQUIRED CONFIG)
find_package(LLD REQUIRED CONFIG)

add_executable(bfc
   main.cpp
   bfc.cpp
   irc.cpp
   utils.cpp
   cli.cpp
   jit.cpp
   stdbfjit/stdbfjit.cpp
)

target_include_directories(bfc PRIVATE
   ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(bfc PRIVATE
   LLVM
   lldCommon
   lldCOFF
)

if(ENABLE_SAN)
   target_compile_options(bfc PRIVATE -fsanitize=address,undefined -g -O1)
   target_link_options(bfc PRIVATE -fsanitize=address,undefined)
   target_compile_definitions(bfc PRIVATE ASAN_ENABLED)
endif()
