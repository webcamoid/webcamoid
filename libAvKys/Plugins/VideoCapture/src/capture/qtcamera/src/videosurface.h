/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QAbstractVideoSurface>
#include <akvideocaps.h>

class VideoSurfacePrivate;
class AkFrac;
class AkPacket;

class VideoSurface: public QAbstractVideoSurface
{
    Q_OBJECT

    public:
        VideoSurface(QObject *parent=nullptr);
        ~VideoSurface();

        Q_INVOKABLE bool present(const QVideoFrame &frame);
        Q_INVOKABLE QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type=QAbstractVideoBuffer::NoHandle) const;
        Q_INVOKABLE static bool isRaw(QVideoFrame::PixelFormat format);
        Q_INVOKABLE static bool isCompessed(QVideoFrame::PixelFormat format);
        Q_INVOKABLE static AkVideoCaps::PixelFormat rawFormat(QVideoFrame::PixelFormat format);
        Q_INVOKABLE static QString compressedFormat(QVideoFrame::PixelFormat format);
        Q_INVOKABLE static QVideoFrame::PixelFormat fromRaw(AkVideoCaps::PixelFormat format);
        Q_INVOKABLE static QVideoFrame::PixelFormat fromCompressed(const QString &format);
        Q_INVOKABLE AkPacket readFrame() const;

    private:
        VideoSurfacePrivate *d;

    public slots:
        void setId(qint64 id);
        void setFps(const AkFrac &fps);
};

#endif // VIDEOSURFACE_H
