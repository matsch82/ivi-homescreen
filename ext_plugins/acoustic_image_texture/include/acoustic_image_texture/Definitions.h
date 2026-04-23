//
// Created by matthias on 8/19/24.
//

#ifndef FLUTTER_ELINUX_DEFINITIONS_H
#define FLUTTER_ELINUX_DEFINITIONS_H

#include <cstdint>
#include <functional>

namespace acoustic_image
{

typedef ::std::function<void( uint xRes, uint yRes )>   OnResolutionChangedCallback;
typedef ::std::function<void( void *data, size_t len )> OnImageReceived;

constexpr auto kDefaultWidth               = 1280;
constexpr auto kDefaultHeight              = 720;
constexpr auto kBytesPerPixel              = 4;
constexpr auto kDefaultBufferSize          = kDefaultWidth * kDefaultHeight * kBytesPerPixel;
constexpr char kAcousticImageHeaderIdent[] = { 'L', 'E', 'A', 'K', 'C', 'A', 'M', '_', 'A', 'C', 'I', 'M',
	                                           '_', 'B', 'L', 'O', 'C', 'K', '_', '0', '0', '0', '1', 0};

typedef struct __attribute__( ( packed ) )
{
	uint8_t headerIdent[ 24 ];
	uint16_t xResolution;
	uint16_t yResolution;
} AcousticImageDataHeader_t;

} // namespace

#endif // FLUTTER_ELINUX_DEFINITIONS_H
