cmake_minimum_required (VERSION 3.8)

project(LegacyOpenSource CXX)

set(PLATFORM_WIN32             FALSE CACHE INTERNAL "")
set(PLATFORM_UNIVERSAL_WINDOWS FALSE CACHE INTERNAL "")
set(PLATFORM_ANDROID           FALSE CACHE INTERNAL "")
set(PLATFORM_LINUX             FALSE CACHE INTERNAL "")
set(PLATFORM_MACOS             FALSE CACHE INTERNAL "")
set(PLATFORM_IOS               FALSE CACHE INTERNAL "")
set(PLATFORM_TVOS              FALSE CACHE INTERNAL "")
set(PLATFORM_EMSCRIPTEN        FALSE CACHE INTERNAL "")
set(D3D11_SUPPORTED            FALSE CACHE INTERNAL "D3D11 is not supported")
set(D3D12_SUPPORTED            FALSE CACHE INTERNAL "D3D12 is not supported")
set(GL_SUPPORTED               FALSE CACHE INTERNAL "GL is not supported")
set(GLES_SUPPORTED             FALSE CACHE INTERNAL "GLES is not supported")
set(VULKAN_SUPPORTED           FALSE CACHE INTERNAL "Vulkan is not supported")
set(METAL_SUPPORTED            FALSE CACHE INTERNAL "Metal is not supported")
set(ARCHIVER_SUPPORTED         FALSE CACHE INTERNAL "Archiver is not supported")

if(WIN32)
    set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32") #WIN32 is a variable, so we cannot use string "WIN32"
    message("Target platform: Win32 " ${ARCH} ". SDK Version: " ${CMAKE_SYSTEM_VERSION})
else()
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(PLATFORM_LINUX TRUE CACHE INTERNAL "Target platform: Linux")
        message("Target platform: Linux " ${ARCH})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform: MacOS")
        message("Target platform: MacOS " ${ARCH})
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif(WIN32)

add_library(Diligent-PublicBuildSettings INTERFACE)

if(PLATFORM_WIN32)      #DX11 isn't suported
    if(MSVC)
        if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
            set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported on Win32 platform")
        endif()
    else()
        message("Building with MinGW")
        set(MINGW_BUILD TRUE CACHE INTERNAL "Building with MinGW")
        set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 requires compiling with MSVC")
    endif()

    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Win32 platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Win32 platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Win32 platform")
    add_compile_definitions(PLATFORM_WIN32=1)
elseif(PLATFORM_LINUX)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on Linux platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is supported on Linux platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on Linux platform")
    add_compile_definitions(PLATFORM_LINUX=1)
elseif(PLATFORM_MACOS)
    set(GL_SUPPORTED       TRUE CACHE INTERNAL "OpenGL is supported on MacOS platform")
    set(VULKAN_SUPPORTED   TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on MacOS platform")
    set(ARCHIVER_SUPPORTED TRUE CACHE INTERNAL "Archiver is supported on MacOS platform")
    add_compile_definitions(PLATFORM_MACOS=1)
else()
    message(FATAL_ERROR "No PLATFORM variable defined.")
endif()

if(NOT (${D3D11_SUPPORTED} OR ${D3D12_SUPPORTED} OR ${GL_SUPPORTED} OR ${GLES_SUPPORTED} OR ${VULKAN_SUPPORTED} OR ${METAL_SUPPORTED}))
    message(FATAL_ERROR "No rendering backends are select to build")
endif()

message("D3D12_SUPPORTED:  " ${D3D12_SUPPORTED})
message("GL_SUPPORTED:     " ${GL_SUPPORTED})
message("GLES_SUPPORTED:   " ${GLES_SUPPORTED})
message("VULKAN_SUPPORTED: " ${VULKAN_SUPPORTED})

add_compile_definitions(
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    GLES_SUPPORTED=$<BOOL:${GLES_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
)

set(SOURCES
    src/BaseEngine.cpp
    src/BaseEngine.hpp
    src/legacyoss.cpp
    src/legacyoss.hpp
    src/ImGuiImplGLFW.cpp
    src/ImGuiImplGLFW.hpp
    src/ThirdParty/imgui_impl_glfw.cpp
    src/ThirdParty/imgui_impl_glfw.h
    src/FirstPersonCamera.cpp
    src/FirstPersonCamera.hpp
)
if(PLATFORM_MACOS)
    list(APPEND SOURCES src/SurfaceHelper.mm)
endif()

set(ASSETS ${SHADERS} ${RENDER_STATES})

set_source_files_properties(${RENDER_STATES} PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties(${SHADERS}       PROPERTIES VS_TOOL_OVERRIDE "None")

include_directories(DilligentEngine/include/DiligentCore DilligentEngine/include/DiligentFX DilligentEngine/include/DiligentTools src/BaseEngine/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG=1)
    link_directories(DilligentEngine/lib/DiligentCore/Debug DilligentEngine/lib/DiligentFX/Debug DilligentEngine/lib/DiligentTools/Debug)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(DEBUG=0)
    link_directories(DilligentEngine/lib/DiligentCore/Release DilligentEngine/lib/DiligentFX/Release DilligentEngine/lib/DiligentTools/Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_definitions(DEBUG=0)
    link_directories(DilligentEngine/lib/DiligentCore/Release DilligentEngine/lib/DiligentFX/Release DilligentEngine/lib/DiligentTools/Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(DEBUG=1)
    link_directories(DilligentEngine/lib/DiligentCore/Debug DilligentEngine/lib/DiligentFX/Debug DilligentEngine/lib/DiligentTools/Debug)
else()
    message("Unknown build type, defaulting to debug.")
    add_compile_definitions(DEBUG=1)
    link_directories(DilligentEngine/lib/DiligentCore/Debug DilligentEngine/lib/DiligentFX/Debug DilligentEngine/lib/DiligentTools/Debug)
endif()

if(PLATFORM_MACOS)
    add_executable(LegacyOpenSource MACOSX_BUNDLE ${SOURCES} ${ASSETS})
else()
    add_executable(LegacyOpenSource ${SOURCES} ${ASSETS})
endif()

target_link_libraries(LegacyOpenSource INTERFACE Diligent-PublicBuildSettings)

set_target_properties(LegacyOpenSource PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_compile_features(LegacyOpenSource PUBLIC cxx_std_23)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${ASSETS})
target_include_directories(LegacyOpenSource PRIVATE -DENGINE_DLL)

target_compile_definitions(LegacyOpenSource PRIVATE UNICODE)

target_compile_definitions(LegacyOpenSource PRIVATE ENGINE_DLL=1)
target_link_libraries(LegacyOpenSource
PRIVATE
    Common
    GraphicsTools
    RenderStateNotation
    DiligentCore
    DiligentTools
    TextureLoader
    MachineIndependent
    GenericCodeGen
    OSDependent
    OGLCompiler
    SPIRV
    SPIRV-Tools-opt
    SPIRV-Tools
    spirv-cross-core
    glslang-default-resource-limits
    glslang
    HLSL
    glfw3
    glew-static
    Imgui
    Diligent-Imgui
)

if(D3D12_SUPPORTED)
    target_link_libraries(LegacyOpenSource PRIVATE GraphicsEngineD3D12)
endif()
if(GL_SUPPORTED)
    target_link_libraries(LegacyOpenSource PRIVATE GraphicsEngineOpenGL)
endif()
if(VULKAN_SUPPORTED)
    target_link_libraries(LegacyOpenSource PRIVATE GraphicsEngineVk)
endif()

if(PLATFORM_WIN32)
    copy_required_dlls(LegacyOpenSource)
endif()

if(MSVC)
    set_target_properties(LegacyOpenSource PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    )
endif()

if(PLATFORM_WIN32 OR PLATFORM_LINUX)
    # Copy assets to target folder
    add_custom_command(TARGET LegacyOpenSource POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "\"$<TARGET_FILE_DIR:LegacyOpenSource>\"")
endif()

if(PLATFORM_MACOS AND VULKAN_LIB_PATH)
    # Configure rpath so that executables can find vulkan library
    set_target_properties(LegacyOpenSource PROPERTIES
        BUILD_RPATH "${VULKAN_LIB_PATH}"
    )
endif()

set_source_files_properties(${ASSETS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "."
    MACOSX_PACKAGE_LOCATION "Resources"
)

if(PLATFORM_LINUX)
    target_link_libraries(LegacyOpenSource
    PRIVATE
        GL
        X11
    )
endif()
if(PLATFORM_MACOS)
    message( "MacOS isnt yet tested to work")
endif()
if(PLATFORM_WIN32)
    message( "Windows isnt yet tested to work")
endif()
