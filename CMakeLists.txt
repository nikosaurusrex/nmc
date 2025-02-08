cmake_minimum_required(VERSION 3.10)

project(nmc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})

set(SOURCES
    Source/Main.cpp
    Source/General.h
    Source/Platform/PlatformContext.h
    Source/Platform/Platform.h
    Source/Platform/Platform.cpp
    Source/Platform/PlatformWindows.cpp
    Source/DataStructures/Arena.cpp
    Source/DataStructures/Arena.h
    Source/DataStructures/Array.cpp
    Source/DataStructures/Array.h
    Source/DataStructures/String.h
    Source/Window/Keys.h
    Source/Window/Window.h
    Source/Window/WindowWindows.cpp
    Source/Math/Mat.cpp
    Source/Math/Mat.h
    Source/Math/NMath.cpp
    Source/Math/NMath.h
    Source/Math/Quat.cpp
    Source/Math/Quat.h
    Source/Math/SIMD.cpp
    Source/Math/SIMD.h
    Source/Math/Vec.cpp
    Source/Math/Vec.h
    Source/ThirdParty/ThirdPartyBuild.cpp
    Source/ThirdParty/stb_sprintf.h
)

add_executable(nmc ${SOURCES})

target_link_libraries(nmc ${Vulkan_LIBRARIES})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
endif()

message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
