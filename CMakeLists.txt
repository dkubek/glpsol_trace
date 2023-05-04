cmake_minimum_required(VERSION 3.11)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Setup the project
project(glpsol_trace
    VERSION 0.1
    DESCRIPTION "Solve MCF problems using GLPK"
    LANGUAGES CXX)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options("$<$<CONFIG:RELEASE>:-W;-Wall;-O3;-pedantic>")

# Add external libraries
add_subdirectory(libs/mcf-graph-interface/)

# Add custom GLPK library
add_subdirectory(libs/glpk-simplex-trace/)

# Add helper libraries
add_subdirectory(libs/cxxopts)
add_subdirectory(libs/json)

# $source_files is a space-delimited list of filenames.
add_executable(glpsol_trace src/glpsol_trace.cpp)
add_executable(convert_problem src/convert_problem.cpp)

target_compile_features(glpsol_trace PUBLIC cxx_std_17)

# IMPORTANT: Make sure to link library to executable *after* it has been added
target_link_libraries(glpsol_trace PRIVATE Graph)
target_link_libraries(glpsol_trace PRIVATE gmp)
target_link_libraries(glpsol_trace PRIVATE gmpxx)
target_link_libraries(glpsol_trace PRIVATE glpk-simplex-trace)
target_link_libraries(glpsol_trace PRIVATE cxxopts)
target_include_directories(glpsol_trace PRIVATE cxxopts)

target_link_libraries(convert_problem PRIVATE Graph)
target_link_libraries(convert_problem PRIVATE nlohmann_json)

install(TARGETS glpsol_trace convert_problem RUNTIME DESTINATION bin)
