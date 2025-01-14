cmake_minimum_required(VERSION 3.21)
project(v2d CXX)

option(V2D_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(V2D_BUILD_EXAMPLE "Build example" OFF)

if(V2D_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
endif()
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)
find_program(GLSLC glslc REQUIRED)

function(target_add_shader target shader)
    set(source ${CMAKE_CURRENT_SOURCE_DIR}/${shader})
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${GLSLC} -O -g ${source} -o ${output}
        DEPENDS ${source}
        VERBATIM)
    set_property(TARGET ${target} APPEND PROPERTY SOURCES ${output})
endfunction()

add_library(v2d)
target_add_shader(v2d shaders/main.frag)
target_add_shader(v2d shaders/main.vert)
add_subdirectory(sources)
target_compile_features(v2d PRIVATE cxx_std_20)
target_include_directories(v2d PUBLIC include)
target_include_directories(v2d SYSTEM PUBLIC third-party)
target_link_libraries(v2d PUBLIC Vulkan::Vulkan X11::xcb X11::xcb_util)

if(V2D_BUILD_BENCHMARKS)
    add_executable(v2d-benchmarks)
    add_subdirectory(benchmarks)
    target_compile_features(v2d-benchmarks PRIVATE cxx_std_20)
    target_link_libraries(v2d-benchmarks PRIVATE benchmark::benchmark_main v2d)
endif()

if(V2D_BUILD_EXAMPLE)
    add_executable(v2d-example)
    add_subdirectory(example)
    target_compile_features(v2d-example PRIVATE cxx_std_20)
    target_link_libraries(v2d-example PRIVATE v2d)
endif()
