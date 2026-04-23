#
# In-tree spectrum_texture plugin.
#
add_library(plugin_spectrum_texture STATIC
        spectrum_texture_plugin.cc
        spectrum_texture_plugin_c_api.cc
        ReceiverTask.cc
)

target_include_directories(plugin_spectrum_texture PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(plugin_spectrum_texture PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(plugin_spectrum_texture PUBLIC
        flutter
        platform_homescreen
)
