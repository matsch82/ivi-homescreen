/*
 * Aggregator header for in-tree "ext" plugins.  Called by
 * shell/view/flutter_view.cc once per engine start.
 */

#ifndef EXT_PLUGINS_EXT_PLUGIN_REGISTRANT_H_
#define EXT_PLUGINS_EXT_PLUGIN_REGISTRANT_H_

typedef struct FlutterDesktopEngineState* FlutterDesktopEngineRef;

void ExtPluginsApiRegisterPlugins(FlutterDesktopEngineRef engine);

#endif  // EXT_PLUGINS_EXT_PLUGIN_REGISTRANT_H_
