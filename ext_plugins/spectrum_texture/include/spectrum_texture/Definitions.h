//
// Created by matthias on 8/19/24.
//

#ifndef FLUTTER_ELINUX_SPECTRUM_DEFINITIONS_H
#define FLUTTER_ELINUX_SPECTRUM_DEFINITIONS_H

#include <cstdint>
#include <functional>

namespace spectrum
{

    typedef ::std::function<void( uint8_t *data, size_t len )> OnSpectrumReceived;

    //  constexpr auto kMaxWidth = 1280;
    //  constexpr auto kMaxHeight = 720;
    constexpr auto kBytesPerPixel          = 4;
    constexpr auto kNumberFFTSupportPoints = 512U;
    constexpr auto kXResolution            = 100U;
    constexpr auto kFreqResolution		   = 286.1023; // freq per support point
    constexpr auto kFftSpectrumBinMin      = 70U;  // bin starts at 70 * kFreqResolution -> 20027,161 kHz
    constexpr auto kFftSpectrumBinMax      = 454U; // bin ends 454 * kFreqResolution -> 129890,4442 kHz
    constexpr auto kBlankFrequencyCount    = 63U; // blank out all freq below 18024.4449 Khz
    constexpr auto kNbSpectrumBins         = kFftSpectrumBinMax - kFftSpectrumBinMin; // equals 109863.2832 kHz bandwidth
    constexpr auto kYResolution            = 512;
    constexpr auto kBytesPerCoefficient    = 4;
    constexpr auto kMaxCoefficient         = 512;
    constexpr auto kDefaultBufferByteSize  = kMaxCoefficient * kBytesPerCoefficient * 4;
    constexpr char kSpectrumHeaderIdent[]  = { 'L', 'E', 'A', 'K', 'C', 'A', 'M', '_', 'S', 'P', 'E', 'C',
                                               '_', 'B', 'L', 'O', 'C', 'K', '_', '0', '0', '0', '1', 0 };

    typedef struct __attribute__( ( packed ) )
    {
        uint8_t  headerIdent[ 24 ];
        uint16_t numberCoefficients;
    } SpectrumDataHeader_t;

} // namespace spectrum

#endif // FLUTTER_ELINUX_SPECTRUM_DEFINITIONS_H
