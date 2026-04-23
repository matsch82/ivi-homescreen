/*
 * Public C-style entrypoint for the in-tree acoustic_image_texture plugin,
 * consumed by ext_plugins/ext_plugin_registrant.cc.
 *
 * Naming follows the upstream ivi-homescreen-plugins convention
 * (<PluginName>CApiRegisterWithRegistrar).
 */

#ifndef EXT_PLUGINS_ACOUSTIC_IMAGE_TEXTURE_PLUGIN_C_API_H_
#define EXT_PLUGINS_ACOUSTIC_IMAGE_TEXTURE_PLUGIN_C_API_H_

#include <flutter_homescreen.h>

void AcousticImageTexturePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);

#endif  // EXT_PLUGINS_ACOUSTIC_IMAGE_TEXTURE_PLUGIN_C_API_H_
