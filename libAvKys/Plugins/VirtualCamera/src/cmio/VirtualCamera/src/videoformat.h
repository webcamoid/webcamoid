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

#ifndef VIDEOFORMAT_H
#define VIDEOFORMAT_H

#include <vector>
#include <CoreMedia/CMFormatDescription.h>

namespace AkVCam
{
    class VideoFormat
    {
        public:
            VideoFormat();
            VideoFormat(CMVideoCodecType codecType,
                        int32_t width,
                        int32_t height,
                        const std::vector<Float64> &frameRates={});
            VideoFormat(const VideoFormat &other);
            VideoFormat &operator =(const VideoFormat &other);
            ~VideoFormat();

            CMVideoCodecType codecType() const;
            CMVideoCodecType &codecType();
            int32_t width() const;
            int32_t &width();
            int32_t height() const;
            int32_t &height();
            std::vector<Float64> frameRates() const;
            std::vector<Float64> &frameRates();
            std::vector<AudioValueRange> frameRateRanges() const;
            Float64 minimumFrameRate() const;
            CMFormatDescriptionRef create() const;

        private:
            CMVideoCodecType m_codecType;
            int32_t m_width;
            int32_t m_height;
            std::vector<Float64> m_frameRates;
    };
}

#endif // VIDEOFORMAT_H
