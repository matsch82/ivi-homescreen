#include "include/acoustic_image_texture/acoustic_image_texture_plugin.h"

#include <cmath>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <thread>

#include "include/acoustic_image_texture/Definitions.h"
#include <fcntl.h>
#include <flutter/standard_method_codec.h>
#include <unistd.h>

namespace
{
constexpr char kChannelName[]          = "acoustic_image_texture";
constexpr char kMethodNameInitialize[] = "initialize";
constexpr char kTextureId[]            = "textureId";

using namespace acoustic_image;

AcousticImageTexture::AcousticImageTexture()
{
	pixels_.reset( new uint8_t[ kDefaultBufferSize ] );
	memset( pixels_.get(), 0, kDefaultBufferSize );

	buffer_         = std::make_unique<FlutterDesktopPixelBuffer>();
	buffer_->width  = 100;
	buffer_->height = 75;
	buffer_->buffer = pixels_.get();
}
void AcousticImageTexture::setFlutterBufferResolution( size_t w, size_t h )
{
	buffer_->width  = w;
	buffer_->height = h;
}

const FlutterDesktopPixelBuffer *AcousticImageTexture::CopyBuffer( [[maybe_unused]] size_t width,
                                                                   [[maybe_unused]] size_t height )
{
	auto res = buffer_.get();
	return res;
}

// static
void AcousticImageTexturePlugin::RegisterWithRegistrar( flutter::PluginRegistrar *registrar )
{
	auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
	    registrar->messenger(), kChannelName, &flutter::StandardMethodCodec::GetInstance() );
	auto plugin      = std::make_unique<AcousticImageTexturePlugin>( registrar->texture_registrar() );
	auto callHandler = [ plugin_pointer = plugin.get() ]( const auto &call, auto result )
	{
		plugin_pointer->HandleMethodCall( call, std::move( result ) );
	};
	channel->SetMethodCallHandler( callHandler );
	registrar->AddPlugin( std::move( plugin ) );
	std::cout << " AcousticImageTexturePlugin registered on registrar: " << static_cast<void *>( registrar );
}

void AcousticImageTexturePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>             &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result )
{
	const std::string &method_name = method_call.method_name();

	std::function copyBufferCallback = [ this ]( size_t width, size_t height ) -> const FlutterDesktopPixelBuffer *
	{
		begin                                             = std::chrono::high_resolution_clock::now();
		acoustic_image_texture->buffer_->release_context  = this;
		acoustic_image_texture->buffer_->release_callback = []( void *ctx )
		{
			auto  end      = std::chrono::high_resolution_clock::now();
			auto *thiz     = (AcousticImageTexturePlugin *) ( ctx );
			auto  duration = std::chrono::duration_cast<std::chrono::nanoseconds>( end - thiz->begin ).count();
			thiz->durations.push_back( duration );
			if ( thiz->durations.size() == 50 )
			{
				auto const count                = static_cast<float>( thiz->durations.size() );
				auto const duration_copy_all_ns = std::reduce( thiz->durations.begin(), thiz->durations.end() );
				auto const duration_lastrun_ns
				    = std::chrono::duration_cast<std::chrono::nanoseconds>( end - thiz->lastrun ).count();
				auto const avg = duration_copy_all_ns / count;
				auto       fps
				    = float( thiz->frame_count - thiz->frame_count_old ) / float( duration_lastrun_ns / 1000000000.0 );
				auto fps_drop
				    = float( thiz->drop_count - thiz->drop_count_old ) / float( duration_lastrun_ns / 1000000000. );
				std::cout << "acoustic_image: avg " << int( roundl( avg ) / 1000 )
				          << "µs\tfps: " << std::setprecision( 3 ) << fps << "\tfps drop:" << fps_drop << "\t("
				          << thiz->frame_count << "/" << thiz->drop_count << ")" << std::endl;
				thiz->drop_count_old  = thiz->drop_count;
				thiz->frame_count_old = thiz->frame_count;
				thiz->durations.clear();
				thiz->lastrun = end;
			}
		};
		auto res      = acoustic_image_texture->CopyBuffer( width, height );
		buffer_filled = false;
		return res;
	};

	if ( method_name == kMethodNameInitialize )
	{
		std::cout << "acoustic_image: initialize called, textureRegistered=" << textureRegistered << std::endl;
		if ( !textureRegistered )
		{
			acoustic_image_texture = std::make_unique<AcousticImageTexture>();
			texture   = std::make_unique<flutter::TextureVariant>( flutter::PixelBufferTexture( copyBufferCallback ) );
			textureRegistered = true;
			textureId = textures->RegisterTexture( texture.get() );
			std::cout << "acoustic_image: registered texture id:  "<<textureId<<std::endl;
		}

		OnImageReceived onImageCb = [ this ]( void *data, size_t len )
		{
			this->frame_count++;
			if ( buffer_filled )
			{
				drop_count++;
			}
			else
			{
				memcpy( pixels_.get(), data, len );
				buffer_filled = true;
				textures->MarkTextureFrameAvailable( textureId );
			}
		};
		OnResolutionChangedCallback onResChangCb = [ this ]( uint x, uint y )
		{
			::std::cout << "acoustic_image: on onResChangCb: new x:" << x << " new y:" << y << std::endl;
			this->acoustic_image_texture->setFlutterBufferResolution( x, y );
		};

		if ( receiverTask != nullptr )
		{
			::std::cout << "send signal to terminate thread..." << std::endl;
			pthread_kill( receiverTaskThread->native_handle(), SIGUSR1 );
			receiverTaskThread->join();
			::std::cout << "thread terminated" << std::endl;
		}
		receiverTask       = std::make_unique<ReceiverTask>( "/tmp/imgPipe", onImageCb, onResChangCb );
		receiverTaskThread = std::make_unique<std::thread>( &ReceiverTask::start, receiverTask.get() );

		auto response = flutter::EncodableValue( flutter::EncodableMap{
		    { flutter::EncodableValue( kTextureId ), flutter::EncodableValue( this->textureId ) },
		} );
		result->Success( response );
	}
	else
	{
		result->NotImplemented();
	}
}

} // namespace

void AcousticImageTexturePluginRegisterWithRegistrar( FlutterDesktopPluginRegistrarRef registrar )
{
	AcousticImageTexturePlugin::RegisterWithRegistrar(
	    flutter::PluginRegistrarManager::GetInstance()->GetRegistrar<flutter::PluginRegistrar>( registrar ) );
}
