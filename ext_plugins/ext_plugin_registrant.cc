/*
 * Aggregator implementation.  Dispatches to every enabled in-tree "ext"
 * plugin's C-API register function; the compile-time ENABLE_EXT_PLUGIN_*
 * defines are set by ext_plugins/CMakeLists.txt based on the
 * BUILD_EXT_PLUGIN_* CMake options (driven from Yocto PACKAGECONFIG flags).
 */

#include "ext_plugin_registrant.h"

#include <flutter_homescreen.h>

#ifdef ENABLE_EXT_PLUGIN_ACOUSTIC_IMAGE_TEXTURE
#include "acoustic_image_texture/acoustic_image_texture_plugin_c_api.h"
#endif

#ifdef ENABLE_EXT_PLUGIN_SPECTRUM_TEXTURE
#include "spectrum_texture/spectrum_texture_plugin_c_api.h"
#endif

void ExtPluginsApiRegisterPlugins(FlutterDesktopEngineRef engine) {
  (void)engine;
#ifdef ENABLE_EXT_PLUGIN_ACOUSTIC_IMAGE_TEXTURE
  AcousticImageTexturePluginCApiRegisterWithRegistrar(
      FlutterDesktopGetPluginRegistrar(engine, "acoustic_image_texture"));
#endif
#ifdef ENABLE_EXT_PLUGIN_SPECTRUM_TEXTURE
  SpectrumTexturePluginCApiRegisterWithRegistrar(
      FlutterDesktopGetPluginRegistrar(engine, "spectrum_texture"));
#endif
}
