#
# ext_plugins — in-tree Flutter plugins built into the homescreen binary.
#
# Each sub-plugin produces a static library `plugin_<name>` which is linked
# into the `ext_plugins` aggregator library.  The aggregator defines
# `ExtPluginsApiRegisterPlugins(FlutterDesktopEngineRef)`, which is called
# from `shell/view/flutter_view.cc` right after the upstream
# `PluginsApiRegisterPlugins()` call.
#

include_guard()

# CMP0079 lets us call target_link_libraries() from this directory
# against the homescreen executable defined in shell/CMakeLists.txt.
if (POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif ()

# Per-plugin switches (default OFF; Yocto PACKAGECONFIG flips them ON).
option(BUILD_EXT_PLUGIN_ACOUSTIC_IMAGE_TEXTURE
        "Build in-tree acoustic_image_texture plugin" OFF)
option(BUILD_EXT_PLUGIN_SPECTRUM_TEXTURE
        "Build in-tree spectrum_texture plugin" OFF)

# Aggregator library — one translation unit that dispatches to every enabled
# sub-plugin's C-API register function.
add_library(ext_plugins STATIC ext_plugin_registrant.cc)

target_include_directories(ext_plugins PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ext_plugins PUBLIC
        flutter
        platform_homescreen
)

if (BUILD_EXT_PLUGIN_ACOUSTIC_IMAGE_TEXTURE)
    message(STATUS "Ext plugin: acoustic_image_texture")
    add_subdirectory(acoustic_image_texture)
    target_link_libraries(ext_plugins PUBLIC plugin_acoustic_image_texture)
    target_compile_definitions(ext_plugins PUBLIC
            ENABLE_EXT_PLUGIN_ACOUSTIC_IMAGE_TEXTURE)
endif ()

if (BUILD_EXT_PLUGIN_SPECTRUM_TEXTURE)
    message(STATUS "Ext plugin: spectrum_texture")
    add_subdirectory(spectrum_texture)
    target_link_libraries(ext_plugins PUBLIC plugin_spectrum_texture)
    target_compile_definitions(ext_plugins PUBLIC
            ENABLE_EXT_PLUGIN_SPECTRUM_TEXTURE)
endif ()

# Wire the aggregator into the homescreen executable produced by
# shell/CMakeLists.txt (target name == ${PROJECT_NAME} == "ivi-homescreen").
target_link_libraries(${PROJECT_NAME} PRIVATE ext_plugins)
target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_EXT_PLUGINS)
