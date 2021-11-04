cmake_minimum_required(VERSION 3.21)
project(v2d CXX)

find_package(glfw3 QUIET)
find_package(Vulkan REQUIRED)
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

add_executable(v2d
    main.cc
    Window.cc)
target_add_shader(v2d main.frag)
target_add_shader(v2d main.vert)
target_compile_definitions(v2d PRIVATE GLFW_INCLUDE_VULKAN)
target_compile_features(v2d PRIVATE cxx_std_20)
target_link_libraries(v2d PRIVATE glfw Vulkan::Vulkan)
