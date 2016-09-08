/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#import <AVFoundation/AVFoundation.h>

#include "capture.h"

class CapturePrivate
{
    public:
        static inline QString fourccToStr(FourCharCode format)
        {
            char fourcc[5];
            memcpy(fourcc, &format, sizeof(FourCharCode));
            fourcc[4] = 0;

            return QString(fourcc);
        }

        static inline FourCharCode strToFourCC(const QString &format)
        {
            FourCharCode fourcc;
            memcpy(&fourcc, format.toStdString().c_str(), sizeof(FourCharCode));

            return fourcc;
        }
};

Capture::Capture(): QObject()
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodUnknown;
    this->m_nBuffers = 32;
    this->m_webcams = this->webcams();
    this->m_device = this->m_webcams.value(0, "");
}

Capture::~Capture()
{
}

QStringList Capture::webcams() const
{
    QStringList webcams;
    NSArray *cameras = [AVCaptureDevice devicesWithMediaType: AVMediaTypeVideo];

    for (AVCaptureDevice *camera in cameras)
        webcams << camera.uniqueID.UTF8String;

    return webcams;
}

QString Capture::device() const
{
    return this->m_device;
}

QList<int> Capture::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> Capture::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return this->m_nBuffers;
}

QString Capture::description(const QString &webcam) const
{
    NSString *uniqueID = [[NSString alloc]
                          initWithUTF8String: webcam.toStdString().c_str()];
    AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];

    if (!camera)
        return QString();

    return QString(camera.localizedName.UTF8String);
}

QVariantList Capture::caps(const QString &webcam) const
{
    QVariantList caps;
    NSString *uniqueID = [[NSString alloc]
                          initWithUTF8String: webcam.toStdString().c_str()];
    AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];

    if (!camera)
        return caps;

    for (AVCaptureDeviceFormat *format in camera.formats) {
        if ([format.mediaType isEqualToString: AVMediaTypeVideo] == NO)
            continue;

        FourCharCode fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
        CMVideoDimensions size =
                CMVideoFormatDescriptionGetDimensions(format.formatDescription);

        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", CapturePrivate::fourccToStr(fourCC));
        videoCaps.setProperty("width", size.width);
        videoCaps.setProperty("height", size.height);

        for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
            videoCaps.setProperty("fps", AkFrac(1e3 * fpsRange.maxFrameRate, 1e3).toString());
            caps << QVariant::fromValue(videoCaps);
        }
    }

    return caps;
}

QString Capture::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList Capture::imageControls() const
{
    return QVariantList();
}

bool Capture::setImageControls(const QVariantMap &imageControls) const
{
    return false;
}

bool Capture::resetImageControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->imageControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList Capture::cameraControls() const
{
    return QVariantList();
}

bool Capture::setCameraControls(const QVariantMap &cameraControls) const
{
    return false;
}

bool Capture::resetCameraControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket Capture::readFrame()
{
    return AkPacket();
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
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void Capture::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    QVariantList supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void Capture::setIoMethod(const QString &ioMethod)
{
}

void Capture::setNBuffers(int nBuffers)
{
    if (this->m_nBuffers == nBuffers)
        return;

    this->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void Capture::resetDevice()
{
    this->setDevice(this->m_webcams.value(0, ""));
}

void Capture::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void Capture::resetIoMethod()
{
    this->setIoMethod("any");
}

void Capture::resetNBuffers()
{
    this->setNBuffers(32);
}

void Capture::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}
