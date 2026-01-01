# cmake_minimum_required(VERSION 3.16)

# set(CMAKE_CXX_COMPILER clang++)
# set(CMAKE_C_COMPILER clang)

# project(bfc)

# set(CMAKE_BUILD_TYPE Release)

# set(CMAKE_CXX_STANDARD 23)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(ENABLE_SAN NO)

# find_package(LLVM REQUIRED CONFIG)
# find_package(LLD REQUIRED CONFIG)

# separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
# add_definitions(${LLVM_DEFINITIONS_LIST})

# file(GLOB src ./*.cpp)

# add_executable(bfc ${src} ./stdbfjit/stdbfjit.cpp)
# target_link_libraries(bfc PUBLIC ${LLVM} lldCommon lldDriver lldCOFF)
# target_include_directories(bfc PUBLIC ${LLVM})

# add_library(stdbf ./stdbf/stdbf.c)

# if(ENABLE_SAN)
# target_compile_options(bfc PRIVATE
# -fsanitize=address,undefined
# -g
# -O1
# )

# target_link_options(bfc PRIVATE
# -fsanitize=address,undefined
# )

# target_compile_definitions(bfc PUBLIC ASAN_ENABLED)
# endif()

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
   stdbfjit/stdbfjit.cpp
)

target_include_directories(bfc PRIVATE
   ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(bfc PRIVATE
   LLVM
   lldCommon
)

if(ENABLE_SAN)
   target_compile_options(bfc PRIVATE -fsanitize=address,undefined -g -O1)
   target_link_options(bfc PRIVATE -fsanitize=address,undefined)
   target_compile_definitions(bfc PRIVATE ASAN_ENABLED)
endif()
