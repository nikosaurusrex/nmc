cmake_minimum_required(VERSION 3.10)

project(nmc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "Source/*.cpp" "Source/*.h")
file(GLOB_RECURSE SHADER_GFX_SOURCES "Assets/Shaders/*.gfx.slang")
file(GLOB_RECURSE SHADER_COMP_SOURCES "Assets/Shaders/*.comp.slang")

add_executable(nmc ${SOURCES})

add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

target_link_libraries(nmc ${Vulkan_LIBRARIES})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
endif()

message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")

set(SPIRV_FILES "")
foreach(SHADER_SOURCE ${SHADER_GFX_SOURCES})
    get_filename_component(FILE_NAME ${SHADER_SOURCE} NAME_WE)

    set(VERT_SPV ${CMAKE_BINARY_DIR}/Assets/Shaders/${FILE_NAME}-Vert.spv)
    set(FRAG_SPV ${CMAKE_BINARY_DIR}/Assets/Shaders/${FILE_NAME}-Frag.spv)

    add_custom_command(
        OUTPUT ${VERT_SPV}
        COMMAND slangc ${SHADER_SOURCE} -o ${VERT_SPV} -entry VSMain
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling vertex shader ${SHADER_SOURCE}"
    )

    add_custom_command(
        OUTPUT ${FRAG_SPV}
        COMMAND slangc ${SHADER_SOURCE} -o ${FRAG_SPV} -entry PSMain
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling fragment shader ${SHADER_SOURCE}"
    )
    list(APPEND SPIRV_FILES ${VERT_SPV} ${FRAG_SPV})
endforeach()

foreach(SHADER_SOURCE ${SHADER_COMP_SOURCES})
    get_filename_component(FILE_NAME ${SHADER_SOURCE} NAME_WE)

    set(COMP_SPV ${CMAKE_BINARY_DIR}/Assets/Shaders/${FILE_NAME}-Comp.spv)

    add_custom_command(
        OUTPUT ${COMP_SPV}
        COMMAND slangc ${SHADER_SOURCE} -o ${COMP_SPV} -stage compute -entry CSMain
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling compute shader ${SHADER_SOURCE}"
    )

    list(APPEND SPIRV_FILES ${COMP_SPV})
endforeach()

add_custom_target(
    Shaders DEPENDS ${SPIRV_FILES}
)
add_dependencies(nmc Shaders)
