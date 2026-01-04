cmake_minimum_required(VERSION 3.25)

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
   bfrtjit/bfrtjit.cpp
)

target_include_directories(bfc PRIVATE
   ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(bfc PRIVATE
   LLVM
   lldCommon
   lldCOFF
   lldMinGW
)

target_compile_definitions(bfc PRIVATE ${LLVM_DEFINITIONS})

# build for brainf**k runtime

find_program(LLVM_LIB_TOOL llvm-lib REQUIRED)
find_program(LLVM_DLLTOOL llvm-dlltool REQUIRED)
find_program(CLANG_CC clang REQUIRED)

set(RUNTIME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/bfrt/bfrt.c")
set(KERNEL32_DEF "${CMAKE_CURRENT_SOURCE_DIR}/bfrt/tiny_kernel32.def")
set(OUTPUT_LIB "${CMAKE_CURRENT_BINARY_DIR}/bfrt.a")

add_custom_command(
   OUTPUT ${OUTPUT_LIB}
   COMMAND ${CLANG_CC} -c ${RUNTIME_SRC} -o bfrt.o
   COMMAND ${LLVM_DLLTOOL} -d ${KERNEL32_DEF} -l tkernel32.a -m i386:x86-64
   COMMAND ${LLVM_LIB_TOOL} /OUT:${OUTPUT_LIB} bfrt.o tkernel32.a
   DEPENDS ${RUNTIME_SRC} ${KERNEL32_DEF}
   COMMENT "Generating merged runtime library: ${OUTPUT_LIB}"
)

add_custom_target(bfrt ALL DEPENDS ${OUTPUT_LIB})
add_dependencies(bfc bfrt)

if(NOT LLVM_TOOLS_BINARY_DIR)
   get_filename_component(LLVM_BIN_DIR ${CLANG_CC} DIRECTORY)
else()
   set(LLVM_BIN_DIR ${LLVM_TOOLS_BINARY_DIR})
endif()

message(STATUS "LLVM DLL Directory: ${LLVM_BIN_DIR}")

install(TARGETS bfc
   DESTINATION bin
   RUNTIME_DEPENDENCIES
   DIRECTORIES ${LLVM_BIN_DIR}
   PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
   POST_EXCLUDE_REGEXES ".*system32/.*" ".*SYSTEM32/.*"
)

install(FILES ${OUTPUT_LIB} DESTINATION lib)

# address/undefined sanitizer enablation

if(ENABLE_SAN)
   target_compile_options(bfc PRIVATE -fsanitize=address,undefined -g -O1)
   target_link_options(bfc PRIVATE -fsanitize=address,undefined)
   target_compile_definitions(bfc PRIVATE ASAN_ENABLED)
endif()

