/*
 * Thin C-API wrapper that forwards to the existing
 * AcousticImageTexturePluginRegisterWithRegistrar function defined in
 * acoustic_image_texture_plugin.cc.  Keeping it in a separate TU means the
 * aggregator (ext_plugin_registrant.cc) only sees the registrar entrypoint
 * and does not pull in the plugin's internal anonymous-namespace state.
 */

#include "include/acoustic_image_texture/acoustic_image_texture_plugin_c_api.h"

#include <flutter_homescreen.h>

// Defined in acoustic_image_texture_plugin.cc (file-scope, C++ linkage).
void AcousticImageTexturePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);

void AcousticImageTexturePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  AcousticImageTexturePluginRegisterWithRegistrar(registrar);
}
