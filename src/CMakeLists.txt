include_directories(commands)

file(GLOB_RECURSE FILENAMES *.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(also also.cpp ${FILENAMES})
target_link_libraries(also alice mockturtle)

# STP bi-decomposition resynthesis headers live under lib/stp/src/include
# Add the include directory so the also executable can find them when the
# lut_resyn command is built with STP support.
target_include_directories(also PRIVATE ${CMAKE_SOURCE_DIR}/lib/stp/src/include)