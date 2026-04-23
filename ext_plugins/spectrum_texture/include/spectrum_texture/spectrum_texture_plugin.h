#ifndef PLUGINS_ACOUSTIC_TEXTURE_ELINUX_SPECTRUM_TEXTURE_PLUGIN_H_
#define PLUGINS_ACOUSTIC_TEXTURE_ELINUX_SPECTRUM_TEXTURE_PLUGIN_H_

#include "ReceiverTask.h"

#include <csignal>
#include <memory>
#include <thread>
#include <array>
#include <atomic>
#include <mutex>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>


namespace spectrum {
  typedef std::array<uint32_t, kXResolution * kNumberFFTSupportPoints> SpectrumPixels_t;
  SpectrumPixels_t pixels_{};

  class SpectrumTexture {
  public:
    SpectrumTexture();

    virtual ~SpectrumTexture() = default;

    const FlutterDesktopPixelBuffer *CopyBuffer([[maybe_unused]] size_t width, [[maybe_unused]] size_t height);

    std::unique_ptr<FlutterDesktopPixelBuffer> buffer_;

    void renderSpectrum();

    void copyFromReceiver(const uint8_t *string, size_t i);
    // Prepare a frame by copying current rendered pixels into a back buffer
    bool prepareFrameFromPixels();

	// Mark that the in-flight buffer was released (called from release callback)
	void onFrameReleased();
	void renderSpectrumLine( float maxElement, uint32_t yIndex ) const;
	float smoothRawData( float alpha );
	void  blankOutHearingSpectrum();
	void  drawSpectrum( float maxElement );

  private:
    // Double buffering for pixel data returned to Flutter
    std::array<uint8_t, kXResolution * kYResolution * kBytesPerPixel> flutterPixelBufferA_{};
    std::array<uint8_t, kXResolution * kYResolution * kBytesPerPixel> flutterPixelBufferB_{};

    // Index of buffer currently in-flight (0 or 1), -1 if none
    std::atomic<int> inflight_index_{-1};
    // Index of prepared buffer waiting to be displayed (0 or 1), -1 if none
    std::atomic<int> prepared_index_{-1};
    // Simple mutex to guard prepare/swap operations
    std::mutex buffer_mutex_;
  };

  class SpectrumTexturePlugin : public flutter::Plugin {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar);

    explicit SpectrumTexturePlugin(flutter::TextureRegistrar *textures)
        : textures(textures), texture(nullptr), specTexture(nullptr) {
    }

    ~SpectrumTexturePlugin() override
    {
      if ( receiverTaskThread && receiverTaskThread->joinable() )
      {
        pthread_kill( receiverTaskThread->native_handle(), SIGUSR1 );
        receiverTaskThread->join();
      }
    }

  private:
    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                          std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    bool textureRegistered{false};
    flutter::TextureRegistrar *textures;
    std::unique_ptr<flutter::TextureVariant> texture;
    std::unique_ptr<SpectrumTexture> specTexture;
    std::unique_ptr<spectrum::ReceiverTask> receiverTask = nullptr;
    std::unique_ptr<std::thread> receiverTaskThread = nullptr;
    int64_t textureId{};
    std::chrono::system_clock::time_point begin;
    std::vector<long> durations;
    bool buffer_filled = false;
    int drop_count = 0;
    int drop_count_old = 0;
    int frame_count = 0;
    int frame_count_old = 0;
    std::chrono::system_clock::time_point lastrun;
  };

} // namespace

void SpectrumTexturePluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar);

#endif // PLUGINS_ACOUSTIC_TEXTURE_ELINUX_SPECTRUM_TEXTURE_PLUGIN_H_
