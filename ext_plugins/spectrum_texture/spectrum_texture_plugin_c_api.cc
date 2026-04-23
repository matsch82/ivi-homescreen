/*
 * Thin C-API wrapper that forwards to the existing
 * SpectrumTexturePluginRegisterWithRegistrar function defined in
 * spectrum_texture_plugin.cc.
 */

#include "include/spectrum_texture/spectrum_texture_plugin_c_api.h"

#include <flutter_homescreen.h>

void SpectrumTexturePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);

void SpectrumTexturePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  SpectrumTexturePluginRegisterWithRegistrar(registrar);
}
