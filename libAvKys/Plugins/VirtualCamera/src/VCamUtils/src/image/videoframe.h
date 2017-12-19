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

#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <vector>
#include <memory>

namespace AkVCam
{
    class VideoFormatPrivate;
    class VideoFormat;

    class VideoFrame
    {
        public:
            VideoFrame();
            VideoFrame(const VideoFormat &format,
                       const std::shared_ptr<uint8_t> &data,
                       size_t dataSize);
            VideoFrame(const VideoFormat &format,
                       const uint8_t *data,
                       size_t dataSize);
            VideoFrame(const VideoFrame &other);
            VideoFrame &operator =(const VideoFrame &other);
            ~VideoFrame();

            VideoFormat format() const;
            VideoFormat &format();
            std::shared_ptr<uint8_t> data() const;
            std::shared_ptr<uint8_t> &data();
            size_t dataSize() const;
            size_t &dataSize();

        private:
            VideoFormatPrivate *d;
    };
}

#endif // VIDEOFRAME_H
