/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef AKVCAMUTILS_VIDEOFORMAT_H
#define AKVCAMUTILS_VIDEOFORMAT_H

#include <cstdint>
#include <vector>

#define MKFOURCC(a, b, c, d) \
    (((a & 0xff) << 24) | ((b & 0xff) << 16) | ((c & 0xff) << 8) | (d & 0xff))

namespace AkVCam
{
    typedef uint32_t FourCC;

    enum PixelFormat
    {
        // RGB formats
        PixelFormatRGB32 = MKFOURCC('R', 'G', 'B',  32),
        PixelFormatRGB24 = MKFOURCC('R', 'G', 'B',  24),
        PixelFormatRGB16 = MKFOURCC('R', 'G', 'B',  16),
        PixelFormatRGB15 = MKFOURCC('R', 'G', 'B',  15),

        // BGR formats
        PixelFormatBGR32 = MKFOURCC('B', 'G', 'R',  32),
        PixelFormatBGR24 = MKFOURCC('B', 'G', 'R',  24),
        PixelFormatBGR16 = MKFOURCC('B', 'G', 'R',  16),
        PixelFormatBGR15 = MKFOURCC('B', 'G', 'R',  15),

        // Luminance+Chrominance formats
        PixelFormatUYVY = MKFOURCC('U', 'Y', 'V', 'Y'),
        PixelFormatYUY2 = MKFOURCC('Y', 'U', 'Y', '2'),

        // two planes -- one Y, one Cr + Cb interleaved
        PixelFormatNV12 = MKFOURCC('N', 'V', '1', '2'),
        PixelFormatNV21 = MKFOURCC('N', 'V', '2', '1')
    };

    class VideoFormat
    {
        public:
            VideoFormat();
            VideoFormat(FourCC fourcc,
                        int width,
                        int height,
                        const std::vector<double> &frameRates={});
            VideoFormat(const VideoFormat &other);
            VideoFormat &operator =(const VideoFormat &other);
            ~VideoFormat();

            FourCC fourcc() const;
            FourCC &fourcc();
            int width() const;
            int &width();
            int height() const;
            int &height();
            std::vector<double> frameRates() const;
            std::vector<double> &frameRates();
            std::vector<std::pair<double, double>> frameRateRanges() const;
            double minimumFrameRate() const;
            void clear();

            static void roundNearest(int width, int height,
                                     int *owidth, int *oheight,
                                     int align=32);

        private:
            FourCC m_fourcc;
            int m_width;
            int m_height;
            std::vector<double> m_frameRates;
    };
}

#endif // AKVCAMUTILS_VIDEOFORMAT_H
