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

#include <QtDebug>
#include <QMutex>
#include <QVideoFrame>
#include <QWaitCondition>
#include <akvideopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>

#include "videosurface.h"

using QtFmtToAkFmtMap = QMap<QVideoFrame::PixelFormat, AkVideoCaps::PixelFormat>;

inline QtFmtToAkFmtMap initQtFmtToAkFmt()
{
    QtFmtToAkFmtMap qtFmtToAkFmt {
        {QVideoFrame::Format_ARGB32 , AkVideoCaps::Format_argbpack},
        {QVideoFrame::Format_RGB32  , AkVideoCaps::Format_0rgbpack},
        {QVideoFrame::Format_RGB24  , AkVideoCaps::Format_rgb24   },
        {QVideoFrame::Format_RGB565 , AkVideoCaps::Format_rgb565  },
        {QVideoFrame::Format_RGB555 , AkVideoCaps::Format_rgb555  },
        {QVideoFrame::Format_BGRA32 , AkVideoCaps::Format_bgrapack},
        {QVideoFrame::Format_BGR32  , AkVideoCaps::Format_bgr0pack},
        {QVideoFrame::Format_BGR24  , AkVideoCaps::Format_bgr24   },
        {QVideoFrame::Format_BGR565 , AkVideoCaps::Format_bgr565  },
        {QVideoFrame::Format_BGR555 , AkVideoCaps::Format_bgr555  },
        {QVideoFrame::Format_AYUV444, AkVideoCaps::Format_ayuvpack},
        {QVideoFrame::Format_YUV444 , AkVideoCaps::Format_yuv444  },
        {QVideoFrame::Format_YUV420P, AkVideoCaps::Format_yuv420p },
        {QVideoFrame::Format_YV12   , AkVideoCaps::Format_yvu420p },
        {QVideoFrame::Format_UYVY   , AkVideoCaps::Format_uyvy422 },
        {QVideoFrame::Format_YUYV   , AkVideoCaps::Format_yuyv422 },
        {QVideoFrame::Format_NV12   , AkVideoCaps::Format_nv12    },
        {QVideoFrame::Format_NV21   , AkVideoCaps::Format_nv21    },
        {QVideoFrame::Format_Y8     , AkVideoCaps::Format_gray8   },
        {QVideoFrame::Format_Y16    , AkVideoCaps::Format_gray16  },
        {QVideoFrame::Format_ABGR32 , AkVideoCaps::Format_abgrpack},
        {QVideoFrame::Format_YUV422P, AkVideoCaps::Format_yuv422p },
    };

    return qtFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(QtFmtToAkFmtMap, qtFmtToAkFmt, (initQtFmtToAkFmt()))

using QtCompressedFmtToAkFmtMap = QMap<QVideoFrame::PixelFormat, QString>;

inline QtCompressedFmtToAkFmtMap initQtCompressedFmtToAkFmt()
{
    QtCompressedFmtToAkFmtMap qtCompressedFmtToAkFmt {
        {QVideoFrame::Format_Jpeg, "jpeg"},
    };

    return qtCompressedFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(QtCompressedFmtToAkFmtMap,
                          qtCompressedFmtToAkFmt,
                          (initQtCompressedFmtToAkFmt()))

using ImageToPixelFormatMap = QMap<QImage::Format, AkVideoCaps::PixelFormat>;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToAkFormat {
        {QImage::Format_RGB32     , AkVideoCaps::Format_0rgbpack},
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argbpack},
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565  },
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555  },
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444  },
        {QImage::Format_Grayscale8, AkVideoCaps::Format_gray8   }
    };

    return imageToAkFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, imageToAkFormat, (initImageToPixelFormatMap()))

class VideoSurfacePrivate
{
    public:
        qint64 m_id {-1};
        AkFrac m_fps;
        QMutex m_mutex;
        AkPacket m_videoPacket;
        QWaitCondition m_packetReady;
};

VideoSurface::VideoSurface(QObject *parent):
    QAbstractVideoSurface(parent)
{
    this->d = new VideoSurfacePrivate;
}

VideoSurface::~VideoSurface()
{
    delete this->d;
}

bool VideoSurface::present(const QVideoFrame &frame)
{
    QVideoFrame videoFrame(frame);

    if (qtFmtToAkFmt->contains(videoFrame.pixelFormat())) {
        AkVideoCaps videoCaps(qtFmtToAkFmt->value(videoFrame.pixelFormat()),
                              videoFrame.width(),
                              videoFrame.height(),
                              this->d->m_fps);
        AkVideoPacket packet(videoCaps);
        packet.setPts(videoFrame.startTime());
        packet.setTimeBase({1, 1000000});
        packet.setIndex(0);
        packet.setId(this->d->m_id);

        videoFrame.map(QAbstractVideoBuffer::ReadOnly);

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto line = videoFrame.bits(plane);
            auto srcLineSize = videoFrame.bytesPerLine(plane);
            auto lineSize = qMin<size_t>(packet.lineSize(plane), srcLineSize);
            auto heightDiv = packet.heightDiv(plane);

            for (int y = 0; y < videoFrame.height(); ++y) {
                auto ys = y >> heightDiv;
                memcpy(packet.line(plane, y),
                       line + ys * srcLineSize,
                       lineSize);
            }
        }

        videoFrame.unmap();

        this->d->m_mutex.lock();
        this->d->m_videoPacket = packet;
        this->d->m_packetReady.wakeAll();
        this->d->m_mutex.unlock();
    } else if (qtCompressedFmtToAkFmt->contains(videoFrame.pixelFormat())) {
        auto image = videoFrame.image();

        if (imageToAkFormat->contains(image.format())) {
            AkVideoCaps videoCaps(imageToAkFormat->value(image.format()),
                                  videoFrame.width(),
                                  videoFrame.height(),
                                  this->d->m_fps);
            AkVideoPacket packet(videoCaps);
            packet.setPts(videoFrame.startTime());
            packet.setTimeBase({1, 1000000});
            packet.setIndex(0);
            packet.setId(this->d->m_id);

            auto lineSize = qMin<size_t>(packet.lineSize(0),
                                         image.bytesPerLine());

            for (int y = 0; y < videoFrame.height(); ++y)
                memcpy(packet.line(0, y),
                       image.constScanLine(y),
                       lineSize);

            this->d->m_mutex.lock();
            this->d->m_videoPacket = packet;
            this->d->m_packetReady.wakeAll();
            this->d->m_mutex.unlock();
        }
    }

    return true;
}

QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
    if (type != QAbstractVideoBuffer::NoHandle)
        return {};

    return qtFmtToAkFmt->keys() + qtCompressedFmtToAkFmt->keys();
}

bool VideoSurface::isRaw(QVideoFrame::PixelFormat format)
{
    return qtFmtToAkFmt->contains(format);
}

bool VideoSurface::isCompessed(QVideoFrame::PixelFormat format)
{
    return qtCompressedFmtToAkFmt->contains(format);
}

AkVideoCaps::PixelFormat VideoSurface::rawFormat(QVideoFrame::PixelFormat format)
{
    return qtFmtToAkFmt->value(format, AkVideoCaps::Format_none);
}

QString VideoSurface::compressedFormat(QVideoFrame::PixelFormat format)
{
    return qtCompressedFmtToAkFmt->value(format, "");
}

QVideoFrame::PixelFormat VideoSurface::fromRaw(AkVideoCaps::PixelFormat format)
{
    return qtFmtToAkFmt->key(format, QVideoFrame::Format_Invalid);
}

QVideoFrame::PixelFormat VideoSurface::fromCompressed(const QString &format)
{
    return qtCompressedFmtToAkFmt->key(format, QVideoFrame::Format_Invalid);
}

AkPacket VideoSurface::readFrame() const
{
    this->d->m_mutex.lock();

    if (!this->d->m_videoPacket)
        if (!this->d->m_packetReady.wait(&this->d->m_mutex, 1000)) {
            this->d->m_mutex.unlock();

            return {};
        }

    auto packet = this->d->m_videoPacket;
    this->d->m_videoPacket = {};
    this->d->m_mutex.unlock();

    return packet;
}

void VideoSurface::setId(qint64 id)
{
    this->d->m_id = id;
}

void VideoSurface::setFps(const AkFrac &fps)
{
    this->d->m_fps = fps;
}

#include "moc_videosurface.cpp"
