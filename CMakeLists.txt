cmake_minimum_required(VERSION 3.12.4)
project(slc VERSION 0.0.0)

# find flex and bison
find_package(BISON)
find_package(FLEX)

# find LLVM
find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(llvm_libs core native)

# set CXXFLAGS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb3 -fsanitize=address -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")

# generated files
bison_target(
  Parser slc.yy ${CMAKE_CURRENT_BINARY_DIR}/slc_bison.cc
  # COMPILE_FLAGS -Wcounterexamples
)
flex_target(Scanner slc.ll ${CMAKE_CURRENT_BINARY_DIR}/slc_flex.cc)

add_flex_bison_dependency(Scanner Parser)

set(sources
  src/main.cpp
  src/slc_node.cpp
  src/semantics.cpp
  src/llvm_codegen.cpp
  src/list_functions.cpp
)

set(runtime_lib_srcs
  src/runtime/slc_int_list.c
  src/runtime/slc_double_list.c
)

add_library(slc_runtime
  ${runtime_lib_srcs}
)
target_include_directories(slc_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(slc
  ${sources}
  ${BISON_Parser_OUTPUTS}
  ${FLEX_Scanner_OUTPUTS}
)
target_include_directories(slc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
target_link_libraries(slc ${llvm_libs})
set_property(TARGET slc PROPERTY CXX_STANDARD 20)
add_definitions(-DRUNTIME_PREFIX="${CMAKE_INSTALL_PREFIX}/lib")
install(TARGETS slc DESTINATION bin)
install(TARGETS slc_runtime DESTINATION lib)
