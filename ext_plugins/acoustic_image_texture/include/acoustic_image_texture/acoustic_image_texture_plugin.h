#ifndef PLUGINS_ACOUSTIC_TEXTURE_ELINUX_ACOUSTIC_TEXTURE_PLUGIN_H_
#define PLUGINS_ACOUSTIC_TEXTURE_ELINUX_ACOUSTIC_TEXTURE_PLUGIN_H_

#include "ReceiverTask.h"

#include <csignal>
#include <memory>
#include <thread>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>

namespace
{
std::unique_ptr<uint8_t> pixels_;

class AcousticImageTexture
{
  public:
	AcousticImageTexture();

	virtual ~AcousticImageTexture() = default;

	const FlutterDesktopPixelBuffer *CopyBuffer( [[maybe_unused]] size_t width, [[maybe_unused]] size_t height );

	std::unique_ptr<FlutterDesktopPixelBuffer> buffer_;

	void setFlutterBufferResolution( size_t w, size_t h );

  private:
};

class AcousticImageTexturePlugin : public flutter::Plugin
{
  public:
	static void RegisterWithRegistrar( flutter::PluginRegistrar *registrar );

	explicit AcousticImageTexturePlugin( flutter::TextureRegistrar *textures )
	    : textures( textures )
	    , texture( nullptr )
	    , acoustic_image_texture( nullptr )

	{
	}

	~AcousticImageTexturePlugin() override
	{
		if ( receiverTaskThread && receiverTaskThread->joinable() )
		{
			pthread_kill( receiverTaskThread->native_handle(), SIGUSR1 );
			receiverTaskThread->join();
		}
	}

  private:
	// Called when a method is called on this plugin's channel from Dart.
	void HandleMethodCall( const flutter::MethodCall<flutter::EncodableValue>             &method_call,
	                       std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result );

	bool                                          textureRegistered{false};
	flutter::TextureRegistrar                    *textures;
	std::unique_ptr<flutter::TextureVariant>      texture;
	std::unique_ptr<AcousticImageTexture>         acoustic_image_texture;
	std::unique_ptr<acoustic_image::ReceiverTask> receiverTask       = nullptr;
	std::unique_ptr<std::thread>                  receiverTaskThread = nullptr;
	int64_t                                       textureId{};
	std::chrono::high_resolution_clock::time_point         begin;
	std::vector<long>                             durations;
	bool                                          buffer_filled   = false;
	int                                           drop_count      = 0;
	int                                           drop_count_old  = 0;
	int                                           frame_count     = 0;
	int                                           frame_count_old = 0;
	std::chrono::high_resolution_clock::time_point         lastrun;
};

} // namespace

void AcousticImageTexturePluginRegisterWithRegistrar( FlutterDesktopPluginRegistrarRef registrar );

#endif // PLUGINS_ACOUSTIC_TEXTURE_ELINUX_ACOUSTIC_TEXTURE_PLUGIN_H_
