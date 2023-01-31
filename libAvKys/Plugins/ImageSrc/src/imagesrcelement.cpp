/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QQmlContext>
#include <QSettings>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "imagesrcelement.h"

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

class ImageSrcElementPrivate
{
    public:
        ImageSrcElement *self;
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_framesThreadStatus;
        QFuture<void> m_threadStatus;
        QImageReader m_imageReader;
        QReadWriteLock m_fpsMutex;
        QReadWriteLock m_imageReaderMutex;
        bool m_forceFps {false};
        bool m_threadedRead {true};
        bool m_run {false};

        explicit ImageSrcElementPrivate(ImageSrcElement *self);
        void readFrame();
        void sendPacket(const AkPacket &packet);
};

ImageSrcElement::ImageSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new ImageSrcElementPrivate(this);
}

ImageSrcElement::~ImageSrcElement()
{
    delete this->d;
}

QStringList ImageSrcElement::medias()
{
    QStringList medias;
    this->d->m_imageReaderMutex.lockForRead();

    if (!this->d->m_imageReader.fileName().isEmpty())
        medias << this->d->m_imageReader.fileName();

    this->d->m_imageReaderMutex.unlock();

    return medias;
}

QString ImageSrcElement::media() const
{
    this->d->m_imageReaderMutex.lockForRead();
    auto fileName = this->d->m_imageReader.fileName();
    this->d->m_imageReaderMutex.unlock();

    return fileName;
}

QList<int> ImageSrcElement::streams()
{
    this->d->m_imageReaderMutex.lockForRead();
    auto isFileNameEmpty = this->d->m_imageReader.fileName().isEmpty();
    this->d->m_imageReaderMutex.unlock();

    if (isFileNameEmpty)
        return {};

    return {0};
}

int ImageSrcElement::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString ImageSrcElement::description(const QString &media)
{
    this->d->m_imageReaderMutex.lockForRead();
    auto fileName = this->d->m_imageReader.fileName();
    this->d->m_imageReaderMutex.unlock();

    if (media.isEmpty() || fileName != media)
        return {};

    return QFileInfo(media).baseName();
}

AkCaps ImageSrcElement::caps(int stream)
{
    this->d->m_imageReaderMutex.lockForRead();
    auto isFileNameEmpty = this->d->m_imageReader.fileName().isEmpty();
    this->d->m_imageReaderMutex.unlock();

    if (stream != 0 || isFileNameEmpty)
        return {};

    this->d->m_imageReaderMutex.lockForRead();
    auto size = this->d->m_imageReader.size();
    this->d->m_imageReaderMutex.unlock();

    this->d->m_fpsMutex.lockForRead();
    AkVideoCaps caps(AkVideoCaps::Format_rgb24,
                     size.width(),
                     size.height(),
                     this->d->m_fps);
    this->d->m_fpsMutex.unlock();

    return caps;
}

bool ImageSrcElement::isAnimated() const
{
    this->d->m_imageReaderMutex.lockForRead();
    auto supportsAnimation = this->d->m_imageReader.supportsAnimation();
    this->d->m_imageReaderMutex.unlock();

    return supportsAnimation;
}

bool ImageSrcElement::forceFps() const
{
    return this->d->m_forceFps;
}

AkFrac ImageSrcElement::fps() const
{
    this->d->m_fpsMutex.lockForRead();
    auto fps = this->d->m_fps;
    this->d->m_fpsMutex.unlock();

    return fps;
}

QStringList ImageSrcElement::supportedFormats() const
{
    auto formats = QImageReader::supportedImageFormats();

    return QStringList(formats.begin(), formats.end());
}

QString ImageSrcElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ImageSrc/share/qml/main.qml");
}

void ImageSrcElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ImageSrc", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void ImageSrcElement::setForceFps(bool forceFps)
{
    if (this->d->m_forceFps == forceFps)
        return;

    this->d->m_forceFps = forceFps;
    emit this->forceFpsChanged(forceFps);
}

void ImageSrcElement::setFps(const AkFrac &fps)
{
    this->d->m_fpsMutex.lockForWrite();

    if (this->d->m_fps == fps) {
        this->d->m_fpsMutex.unlock();

        return;
    }

    this->d->m_fps = fps;
    this->d->m_fpsMutex.unlock();

    QSettings settings;
    settings.beginGroup("ImageSrc");
    settings.setValue("fps", fps.toString());
    settings.endGroup();

    emit this->fpsChanged(fps);
}

void ImageSrcElement::resetForceFps()
{
    this->setForceFps(false);
}

void ImageSrcElement::resetFps()
{
    this->setFps({});
}

void ImageSrcElement::setMedia(const QString &media)
{
    this->d->m_imageReaderMutex.lockForRead();
    auto fileName = this->d->m_imageReader.fileName();
    this->d->m_imageReaderMutex.unlock();

    if (fileName == media)
        return;

    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_imageReaderMutex.lockForWrite();
    auto size = this->d->m_imageReader.size();
    auto isAnimation = this->d->m_imageReader.supportsAnimation();
    this->d->m_imageReader.setFileName(media);
    this->d->m_imageReaderMutex.unlock();

    if (!media.isEmpty())
        this->setState(state);

    emit this->mediaChanged(media);

    this->d->m_imageReaderMutex.lockForRead();
    auto curSize = this->d->m_imageReader.size();
    auto curIsAnimation = this->d->m_imageReader.supportsAnimation();
    this->d->m_imageReaderMutex.unlock();

    if (size != curSize)
        emit this->sizeChanged(curSize);

    if (isAnimation != curIsAnimation)
        emit this->isAnimatedChanged(curIsAnimation);
}

void ImageSrcElement::resetMedia()
{
    this->setMedia({});
}

bool ImageSrcElement::setState(AkElement::ElementState state)
{
    this->d->m_imageReaderMutex.lockForRead();
    auto isFileNameEmpty = this->d->m_imageReader.fileName().isEmpty();
    this->d->m_imageReaderMutex.unlock();

    if (isFileNameEmpty)
        return false;

    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_id = Ak::id();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_id = Ak::id();
            this->d->m_run = true;
            this->d->m_framesThreadStatus =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &ImageSrcElementPrivate::readFrame);

            return AkElement::setState(state);
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_run = true;
            this->d->m_framesThreadStatus =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &ImageSrcElementPrivate::readFrame);

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
        case AkElement::ElementStatePaused:
            this->d->m_run = false;
            this->d->m_framesThreadStatus.waitForFinished();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

ImageSrcElementPrivate::ImageSrcElementPrivate(ImageSrcElement *self):
    self(self)
{
    this->m_threadPool.setMaxThreadCount(4);
}

void ImageSrcElementPrivate::readFrame()
{
    qreal delayDiff = 0.0;

    while (this->m_run) {
        this->m_fpsMutex.lockForRead();
        auto fps = this->m_fps;
        this->m_fpsMutex.unlock();

        this->m_imageReaderMutex.lockForRead();
        auto image = this->m_imageReader.read();
        auto error = this->m_imageReader.errorString();
        this->m_imageReaderMutex.unlock();

        if (image.isNull()) {
            qDebug() << "Error reading image:" << error;

            auto delay = (1000 / fps).value() + delayDiff;
            delayDiff = delay - qRound(delay);
            QThread::msleep(qRound(delay));

            continue;
        }

        if (!imageToAkFormat->contains(image.format()))
            image = image.convertToFormat(QImage::Format_ARGB32);

        AkVideoCaps caps(imageToAkFormat->value(image.format()),
                         image.width(),
                         image.height(),
                         fps);
        AkVideoPacket packet(caps);
        auto lineSize = qMin<size_t>(image.bytesPerLine(), packet.lineSize(0));

        for (int y = 0; y < image.height(); ++y) {
            auto srcLine = image.constScanLine(y);
            auto dstLine = packet.line(0, y);
            memcpy(dstLine, srcLine, lineSize);
        }

        auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                            * fps.value() / 1e3);
        packet.setPts(pts);
        packet.setTimeBase(fps.invert());
        packet.setIndex(0);
        packet.setId(this->m_id);

        if (!this->m_threadedRead) {
            emit self->oStream(packet);
        } else if (!this->m_threadStatus.isRunning()) {
            this->m_threadStatus =
                    QtConcurrent::run(&this->m_threadPool,
                                      this,
                                      &ImageSrcElementPrivate::sendPacket,
                                      packet);
        }

        this->m_imageReaderMutex.lockForRead();
        auto isLastFrame =
                this->m_imageReader.currentImageNumber() >= this->m_imageReader.imageCount() - 1;
        this->m_imageReaderMutex.unlock();

        if (isLastFrame) {
            this->m_imageReaderMutex.lockForRead();
            auto supportsAnimation = this->m_imageReader.supportsAnimation();
            this->m_imageReaderMutex.unlock();

            if (!supportsAnimation) {
                auto delay = (1000 / fps).value() + delayDiff;
                delayDiff = delay - qRound(delay);
                QThread::msleep(qRound(delay));
            }

            this->m_imageReaderMutex.lockForWrite();
            auto fileName = this->m_imageReader.fileName();
            this->m_imageReader.setFileName({});
            this->m_imageReader.setFileName(fileName);
            this->m_imageReaderMutex.unlock();
        } else {
            if (this->m_forceFps) {
                auto delay = (1000 / fps).value() + delayDiff;
                delayDiff = delay - qRound(delay);
                QThread::msleep(qRound(delay));
            } else {
                this->m_imageReaderMutex.lockForRead();
                auto delay = this->m_imageReader.nextImageDelay();
                this->m_imageReaderMutex.unlock();

                if (delay > 0)
                    QThread::msleep(delay);
            }
        }
    }
}

void ImageSrcElementPrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
}

#include "moc_imagesrcelement.cpp"
