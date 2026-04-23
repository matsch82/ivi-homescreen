#
# In-tree acoustic_image_texture plugin — follows the static-library pattern
# of ivi-homescreen-plugins/plugins/url_launcher/.
#
add_library(plugin_acoustic_image_texture STATIC
        acoustic_image_texture_plugin.cc
        acoustic_image_texture_plugin_c_api.cc
        ReceiverTask.cc
)

# Public header search path: allows ../ext_plugin_registrant.cc to
#   #include "acoustic_image_texture/acoustic_image_texture_plugin_c_api.h"
target_include_directories(plugin_acoustic_image_texture PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Private include path so plugin source files can keep their existing
# `#include "include/acoustic_image_texture/..."` lines unchanged.
target_include_directories(plugin_acoustic_image_texture PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(plugin_acoustic_image_texture PUBLIC
        flutter
        platform_homescreen
)
