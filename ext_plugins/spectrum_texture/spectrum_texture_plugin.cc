#include "include/spectrum_texture/spectrum_texture_plugin.h"

#include <cmath>
#include <cstring>
#include <csignal>
#include <thread>

#include "flutter/standard_method_codec.h"
#include <spectrum_texture/Definitions.h>

namespace
{
constexpr char kChannelName[]          = "spectrum_texture";
constexpr char kMethodNameInitialize[] = "initialize";
constexpr char kTextureId[]            = "textureId";

// Constants
constexpr uint32_t kRedColor            = 0xAA0000FF; // Red in ARGB format
} // namespace

using namespace spectrum;

#include <iostream>
#include <vector>
// Global buffers
std::array<float, kMaxCoefficient>                                rawDataBuffer{ 0 };
std::array<float, kMaxCoefficient>                                smoothedDataBuffer{ 0 };

SpectrumTexture::SpectrumTexture()
{
    std::cout << std::this_thread::get_id() << ": SpectrumTexture constructor start" << std::endl;
    buffer_         = std::make_unique<FlutterDesktopPixelBuffer>();
    buffer_->width  = kXResolution;
    buffer_->height = kYResolution;
    buffer_->buffer = nullptr; // will be set in CopyBuffer based on active buffer
    std::cout << std::this_thread::get_id() << ": SpectrumTexture constructor end" << std::endl;
}

// Refactored renderSpectrum
void SpectrumTexture::renderSpectrum()
{
	// clean present pixels
	std::fill( std::begin( pixels_ ), std::end( pixels_ ), 0 );
	blankOutHearingSpectrum();
	float maxElement = smoothRawData( 0.7f );
	drawSpectrum( maxElement );
}

/**
 * Sets all elements in the `rawDataBuffer` array to zero, effectively
 * blanking out the hearing spectrum 0-18,75kHz
 */
void SpectrumTexture::blankOutHearingSpectrum()
{
	for ( int i = 0; i < kBlankFrequencyCount; ++i )
	{
		rawDataBuffer[ i ] = 0;
	}
}

/**
 * Calculates a sliding (or moving) average over a std::array.
 *
 * @tparam N The size of the input std::array.
 * @param input The input std::array containing the data.
 * @param windowSize The size of the sliding window (must be <= N).
 * @return A new std::array containing the sliding averages. The size of the
 * result is N - windowSize + 1.
 */
template<std::size_t N>
std::array<float, N> computeSlidingAverage( const std::array<float, N> &input, std::size_t windowSize )
{
	if ( windowSize == 0 || windowSize > N )
	{
		throw std::invalid_argument( "Window size must be > 0 and <= size of array." );
	}

	std::array<float, N> slidingAverages{}; // Result array, will only be partially filled
	float                windowSum = 0.0f;

	// Initialize the first window sum
	for ( std::size_t i = 0; i < windowSize; ++i )
	{
		windowSum += input[ i ];
	}

	// Fill the result array with sliding averages
	for ( std::size_t i = 0; i <= N - windowSize; ++i )
	{
		slidingAverages[ i ] = windowSum / windowSize; // Compute average for current window

		// Update the sum for the next window
		if ( i + windowSize < N )
		{
			windowSum += input[ i + windowSize ]; // Add the next element in the window
			windowSum -= input[ i ];              // Remove the first element of the current window
		}
	}

	return slidingAverages;
}

/**
 * Applies smoothing to the raw data buffer using exponential moving average and updates the smoothed data buffer.
 *
 * Additionally, it calculates the maximum value in the smoothed data buffer.
 *
 * @param smoothingAlpha A float value between 0 and 1 that defines the smoothing factor. Higher values give more weight
 *                       to the current raw data value, while lower values give more weight to the previous smoothed
 * value.
 * @return The maximum element value in the updated smoothed data buffer.
 */
float SpectrumTexture::smoothRawData( const float smoothingAlpha )
{
	for ( int i = 0; i < 512; ++i )
	{
		float value             = rawDataBuffer[ i ];
		smoothedDataBuffer[ i ] = smoothingAlpha * value + ( 1 - smoothingAlpha ) * smoothedDataBuffer[ i ];
	}

	// smooth spectrum over frequency range
	smoothedDataBuffer = computeSlidingAverage( smoothedDataBuffer, 5 );

	return *std::max_element( std::begin( smoothedDataBuffer ), std::end( smoothedDataBuffer ) );
}

void SpectrumTexture::drawSpectrum( float maxElement )
{
	for ( int32_t yIndex = 0; yIndex < kYResolution; yIndex++ )
	{
		renderSpectrumLine( maxElement, static_cast<uint32_t>( yIndex ) );
	}
}

/**
 * Maps a value from 0 to 255 into a heatmap color:
 * - 0 maps to blue (ARGB: 0xFF0000FF)
 * - 255 maps to red (ARGB: 0xFFFF0000)
 * - Intermediate values transition through cyan, green, and yellow.
 *
 * @param value Heatmap value in the range [0, 255].
 * @return ARGB color corresponding to the value.
 */
uint32_t heatmapColor( uint8_t value )
{
	uint8_t r, g, b, a;

	if ( value < 64 )
	{
		r = 0;
		g = value * 4; // Green increases
		b = 255;       // Blue is max
	}
	else if ( value < 128 )
	{
		r = 0;
		g = 255;                      // Green is max
		b = 255 - ( value - 64 ) * 4; // Blue decreases
	}
	else if ( value < 192 )
	{
		r = ( value - 128 ) * 4; // Red increases
		g = 255;                 // Green is max
		b = 0;                   // Blue is 0
	}
	else
	{
		r = 255;                       // Red is max
		g = 255 - ( value - 192 ) * 4; // Green decreases
		b = 0;                         // Blue is 0
	}

	// Combine into ARGB format (fully opaque)
	return ( 0xFF << 24 ) | ( b << 16 ) | ( g << 8 ) | r;
}

void SpectrumTexture::renderSpectrumLine( float maxElement, uint32_t yIndex ) const
{
	// Function to linearly interpolate between 512 and 320
	float originalIndex = ( static_cast<float>( yIndex ) / kMaxCoefficient ) * (kNbSpectrumBins-1); // Scale index to 320-pixel array range
	int   lowerIndex    = static_cast<int>( std::floor( originalIndex ) );
	int   upperIndex    = static_cast<int>( std::ceil( originalIndex ) );

	if ( upperIndex >= kNbSpectrumBins )
	{
		upperIndex = kNbSpectrumBins-1; // Clamp index to avoid going out of bounds
	}

	float weight = originalIndex - lowerIndex;

	auto lowerVal= static_cast<int>( std::round( ( smoothedDataBuffer[ lowerIndex + kFftSpectrumBinMin-1 ] * kXResolution ) / maxElement ) );
	auto upperVal = static_cast<int>( std::round( ( smoothedDataBuffer[ upperIndex + kFftSpectrumBinMin-1 ] * kXResolution ) / maxElement ) );


	auto spectrumLineLength =  static_cast<uint32_t>( ( 1 - weight ) * lowerVal + weight * upperVal );


	//	int spectrumLineLength
//	    = static_cast<int>( std::round( ( smoothedDataBuffer[ binIndex ] * kXResolution ) / maxElement ) );

	auto color = heatmapColor( ( spectrumLineLength * 255 ) / kXResolution );
	for ( int32_t y = 0; y < spectrumLineLength; ++y )
	{
		pixels_[ (kYResolution-1-yIndex) * kXResolution + ( kXResolution - 1 - y ) ] = color;
	}
}

// Copy buffer with double buffering. If a prepared buffer exists, swap it to inflight.
const FlutterDesktopPixelBuffer *SpectrumTexture::CopyBuffer( [[maybe_unused]] size_t width,
                                                              [[maybe_unused]] size_t height )
{
    // Swap prepared to inflight if available
    int prepared = prepared_index_.load( std::memory_order_acquire );
    if ( prepared != -1 )
    {
        std::lock_guard<std::mutex> lk( buffer_mutex_ );
        prepared = prepared_index_.load( std::memory_order_relaxed );
        if ( prepared != -1 )
        {
            inflight_index_.store( prepared, std::memory_order_release );
            prepared_index_.store( -1, std::memory_order_release );
        }
    }

    // Select buffer pointer based on inflight index
    int inflight = inflight_index_.load( std::memory_order_acquire );
    uint8_t *ptr = nullptr;
    if ( inflight == 0 )
    {
        ptr = flutterPixelBufferA_.data();
    }
    else if ( inflight == 1 )
    {
        ptr = flutterPixelBufferB_.data();
    }
    else
    {
        // No inflight yet; return an empty black buffer A
        ptr = flutterPixelBufferA_.data();
    }

    buffer_->buffer = ptr;
    return buffer_.get();
}

// Copy from receiver
void SpectrumTexture::copyFromReceiver( const uint8_t *data, size_t len )
{
    memcpy( rawDataBuffer.data(), data, len );
}

// Prepare a frame by copying pixels_ into the back buffer. Returns true if prepared.
bool SpectrumTexture::prepareFrameFromPixels()
{
    std::lock_guard<std::mutex> lk( buffer_mutex_ );
    if ( prepared_index_.load( std::memory_order_relaxed ) != -1 )
    {
        // Already have a prepared buffer; let the caller know to drop this frame
        return false;
    }

    int inflight = inflight_index_.load( std::memory_order_relaxed );
    int back_idx;
    if ( inflight == 0 )
        back_idx = 1;
    else if ( inflight == 1 )
        back_idx = 0;
    else
        back_idx = 0; // default to A if none in flight

    uint8_t *dst = ( back_idx == 0 ) ? flutterPixelBufferA_.data() : flutterPixelBufferB_.data();
    memcpy( dst, reinterpret_cast<const uint8_t *>( pixels_.data() ), kXResolution * kYResolution * kBytesPerPixel );
    prepared_index_.store( back_idx, std::memory_order_release );
    return true;
}

void SpectrumTexture::onFrameReleased()
{
    // Mark that there is no inflight buffer anymore
    inflight_index_.store( -1, std::memory_order_release );
}

// static
void SpectrumTexturePlugin::RegisterWithRegistrar( flutter::PluginRegistrar *registrar )
{
	auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
	    registrar->messenger(), kChannelName, &flutter::StandardMethodCodec::GetInstance() );
	auto plugin      = std::make_unique<SpectrumTexturePlugin>( registrar->texture_registrar() );
	auto callHandler = [ plugin_pointer = plugin.get() ]( const auto &call, auto result )
	{
		plugin_pointer->HandleMethodCall( call, std::move( result ) );
	};
	channel->SetMethodCallHandler( callHandler );
	registrar->AddPlugin( std::move( plugin ) );
	std::cout << std::this_thread::get_id()
	          << ": "
	             " SpectrumTexturePlugin registered on registrar: "
	          << static_cast<void *>( registrar );
}

void SpectrumTexturePlugin::HandleMethodCall( const flutter::MethodCall<flutter::EncodableValue> &method_call,
                                              std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result )
{
	const std::string &method_name = method_call.method_name();

	std::function copyBufferCallback = [ this ]( size_t width, size_t height ) -> const FlutterDesktopPixelBuffer *
	{
		begin                                  = std::chrono::high_resolution_clock::now();
		specTexture->buffer_->release_context  = this;
		specTexture->buffer_->release_callback = []( void *ctx )
		{
			auto  end      = std::chrono::high_resolution_clock::now();
			auto *thiz     = (SpectrumTexturePlugin *) ( ctx );
			auto  duration = std::chrono::duration_cast<std::chrono::nanoseconds>( end - thiz->begin ).count();
			thiz->durations.push_back( duration );
			// notify texture that frame is released so it can reuse buffers
			if ( thiz->specTexture )
			{
				thiz->specTexture->onFrameReleased();
			}
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
				std::cout << "spectrum: avg " << int( roundl( avg ) / 1000 ) << "µs\tfps: " << std::setprecision( 3 )
				          << fps << "\tfps drop:" << fps_drop << "\t(" << thiz->frame_count << "/" << thiz->drop_count
				          << ")" << std::endl;
				thiz->drop_count_old  = thiz->drop_count;
				thiz->frame_count_old = thiz->frame_count;
				thiz->durations.clear();
				thiz->lastrun = end;
			}
		};
		auto res      = specTexture->CopyBuffer( width, height );
		return res;
	};

	if ( method_name == kMethodNameInitialize )
	{
		std::cout << std::this_thread::get_id() << ": "
		          << "spectrum: initialize called, textureRegistered=" << textureRegistered << std::endl;
		if ( !textureRegistered )
		{
			specTexture = std::make_unique<SpectrumTexture>();
			texture = std::make_unique<flutter::TextureVariant>( flutter::PixelBufferTexture( copyBufferCallback ) );
			textureRegistered = true;
			textureId         = textures->RegisterTexture( texture.get() );
			std::cout << std::this_thread::get_id() << ": " << "spectrum: registered texture id:  " << textureId
			          << std::endl;
		}

		OnSpectrumReceived onSpectrumReceivedCb = [ this ]( const uint8_t *data, size_t len )
		{
			this->frame_count++;
			specTexture.get()->copyFromReceiver( data, len );
			specTexture.get()->renderSpectrum();
			if ( specTexture.get()->prepareFrameFromPixels() )
			{
				textures->MarkTextureFrameAvailable( textureId );
			}
			else
			{
				drop_count++;
			}
		};
		if ( receiverTask != nullptr )
		{
			::std::cout << "spectrum: send signal to terminate thread..." << std::endl;
			pthread_kill( receiverTaskThread->native_handle(), SIGUSR1 );
			receiverTaskThread->join();
			::std::cout << "spectrum: thread terminated" << std::endl;
		}
		receiverTask       = std::make_unique<ReceiverTask>( "/tmp/specPipe", onSpectrumReceivedCb );
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
void SpectrumTexturePluginRegisterWithRegistrar( FlutterDesktopPluginRegistrarRef registrar )
{
	SpectrumTexturePlugin::RegisterWithRegistrar(
	    flutter::PluginRegistrarManager::GetInstance()->GetRegistrar<flutter::PluginRegistrar>( registrar ) );
}
