/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <QVariant>
#include <QtConcurrent>
#include <QtMath>
#include <qrgb.h>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "capture.h"

#define VALUE_SHIFT 8

class CapturePrivate
{
    public:
        QThreadPool m_threadPool;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgba, 0, 0, {}}};

        void updateRotateMatrix(qreal angle);
};

Capture::Capture(QObject *parent):
    QObject(parent)
{
    this->d = new CapturePrivate;
    this->d->m_threadPool.setMaxThreadCount(16);
}

Capture::~Capture()
{
    delete this->d;
}

QString Capture::error() const
{
    return {};
}

QStringList Capture::webcams() const
{
    return QStringList();
}

QString Capture::device() const
{
    return QString();
}

QList<int> Capture::streams()
{
    return {};
}

QList<int> Capture::listTracks(AkCaps::CapsType type)
{
    Q_UNUSED(type)

    return {};
}

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return 0;
}

QString Capture::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}

AkCapsList Capture::caps(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return {};
}

QVariantList Capture::imageControls() const
{
    return QVariantList();
}

bool Capture::setImageControls(const QVariantMap &imageControls)
{
    Q_UNUSED(imageControls)

    return false;
}

bool Capture::resetImageControls()
{
    return false;
}

QVariantList Capture::cameraControls() const
{
    return QVariantList();
}

bool Capture::setCameraControls(const QVariantMap &cameraControls)
{
    Q_UNUSED(cameraControls)

    return false;
}

bool Capture::resetCameraControls()
{
    return false;
}

bool Capture::isTorchSupported() const
{
    return false;
}

Capture::TorchMode Capture::torchMode() const
{
    return Torch_Off;
}

Capture::PermissionStatus Capture::permissionStatus() const
{
    return PermissionStatus_Granted;
}

AkPacket Capture::readFrame()
{
    return AkPacket();
}

int Capture::nearestResolution(const QSize &resolution,
                               const AkFrac &fps,
                               const AkCapsList &caps)
{
    if (caps.isEmpty())
        return -1;

    int index = -1;
    qreal q = std::numeric_limits<qreal>::max();

    for (int i = 0; i < caps.size(); ++i) {
        auto c = caps.value(i);
        AkVideoCaps videoCaps = c.type() == AkCaps::CapsVideo?
                                    AkVideoCaps(c):
                                    AkCompressedVideoCaps(c).rawCaps();
        qreal dw = videoCaps.width() - resolution.width();
        qreal dh = videoCaps.height() - resolution.height();
        qreal df =  (videoCaps.fps() - fps).value();
        qreal k = dw * dw + dh * dh + df * df;

        if (k < q) {
            index = i;
            q = k;

            if (k == 0.0)
                break;
        }
    }

    return index;
}

AkPacket Capture::rotate(const AkPacket &packet, qreal angle)
{
    // Normalize angle to [0, 360)
    int normalizedAngle = int(angle) % 360;

    if (normalizedAngle < 0)
        normalizedAngle += 360;

    // 0°: return original packet without any processing
    if (normalizedAngle == 0)
        return packet;

    // Convert to RGBA format (required for pixel manipulation)
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return packet;

    int srcW = src.caps().width();
    int srcH = src.caps().height();
    auto caps = src.caps();
    AkVideoPacket dst;

    if (normalizedAngle == 180) {
        // 180°: same dimensions, reverse both axes
        dst = AkVideoPacket(caps);
        dst.copyMetadata(src);

        for (int y = 0; y < srcH; ++y) {
            auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
            auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, srcH - 1 - y));
            std::reverse_copy(srcLine, srcLine + srcW, dstLine);
        }
    } else {
        // 90° or 270°: swap width and height
        caps.setWidth(srcH);
        caps.setHeight(srcW);
        dst = AkVideoPacket(caps);
        dst.copyMetadata(src);

        if (normalizedAngle == 90) {
            // 90° clockwise: dst(x, y) = src(srcH - 1 - x, y)
            QVector<const QRgb *> srcLines(srcH);

            for (int row = 0; row < srcH; ++row)
                srcLines[row] = reinterpret_cast<const QRgb *>(src.constLine(0, row));

            for (int y = 0; y < srcW; ++y) {
                auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

                for (int x = 0; x < srcH; ++x)
                    dstLine[x] = srcLines[srcH - 1 - x][y];
            }
        } else { // 270°
            // 270° clockwise = 90° counter-clockwise
            // dst(x, y) = src(x, srcW - 1 - y)
            for (int y = 0; y < srcW; ++y) {
                auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));
                auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, srcW - 1 - y));
                memcpy(dstLine, srcLine, srcH * sizeof(QRgb));
            }
        }
    }

    return dst;
}

bool Capture::init()
{
    return false;
}

void Capture::uninit()
{
}

void Capture::setDevice(const QString &device)
{
    Q_UNUSED(device)
}

void Capture::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void Capture::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void Capture::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void Capture::setTorchMode(TorchMode mode)
{
    Q_UNUSED(mode)
}

void Capture::resetDevice()
{
}

void Capture::resetStreams()
{
}

void Capture::resetIoMethod()
{
}

void Capture::resetNBuffers()
{
}

void Capture::resetTorchMode()
{

}

void Capture::reset()
{
}

void Capture::takePictures(int count, int delayMsecs)
{
    auto result =
        QtConcurrent::run(&this->d->m_threadPool,
                          [this, count, delayMsecs] () {
            for (int i = 0; i < count; i++) {
                auto frame = this->readFrame();
                emit this->pictureTaken(i, frame);
                QThread::msleep(delayMsecs);
            }
        });
    Q_UNUSED(result)
}

#include "moc_capture.cpp"
