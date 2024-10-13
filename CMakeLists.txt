cmake_minimum_required(VERSION 3.12.4)
project(slc VERSION 0.0.0)

# find flex and bison
find_package(BISON)
find_package(FLEX)

# set CXXFLAGS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb3 -fsanitize=address -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")

# generated files
bison_target(Parser slc.yy ${CMAKE_CURRENT_BINARY_DIR}/slc_bison.cc)
flex_target(Scanner slc.ll ${CMAKE_CURRENT_BINARY_DIR}/slc_flex.cc)

add_flex_bison_dependency(Scanner Parser)

set(sources
  src/main.cpp
) 

add_executable(slc
  ${sources}
  ${BISON_Parser_OUTPUTS}
  ${FLEX_Scanner_OUTPUTS}
)
target_include_directories(slc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# target_link_libraries(slc Threads::Threads)

set_property(TARGET slc PROPERTY CXX_STANDARD 20)

install(TARGETS slc DESTINATION bin)
