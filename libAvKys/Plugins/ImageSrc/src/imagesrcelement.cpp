/* Webcamoid, camera capture application.
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
        {QImage::Format_RGB32     , AkVideoCaps::Format_xrgbpack},
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argbpack},
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565  },
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555  },
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444  },
        {QImage::Format_Grayscale8, AkVideoCaps::Format_y8      }
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
    QReadLocker locker(&this->d->m_imageReaderMutex);
    auto fileName = this->d->m_imageReader.fileName();

    if (fileName.isEmpty())
        return {};

    return {fileName};
}

QString ImageSrcElement::media() const
{
    QReadLocker locker(&this->d->m_imageReaderMutex);

    return this->d->m_imageReader.fileName();
}

QList<int> ImageSrcElement::streams()
{
    QReadLocker locker(&this->d->m_imageReaderMutex);

    if (this->d->m_imageReader.fileName().isEmpty())
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
    QReadLocker locker(&this->d->m_imageReaderMutex);
    auto fileName = this->d->m_imageReader.fileName();

    if (media.isEmpty() || fileName != media)
        return {};

    return QFileInfo(media).baseName();
}

AkCaps ImageSrcElement::caps(int stream)
{
    if (stream != 0)
        return {};

    QSize size;

    {
        QReadLocker readerLocker(&this->d->m_imageReaderMutex);

        if (this->d->m_imageReader.fileName().isEmpty())
            return {};

        size = this->d->m_imageReader.size();
    }

    QReadLocker fpsLocker(&this->d->m_fpsMutex);
    AkVideoCaps caps(AkVideoCaps::Format_rgb24,
                     size.width(),
                     size.height(),
                     this->d->m_fps);

    return caps;
}

bool ImageSrcElement::isAnimated() const
{
    QReadLocker locker(&this->d->m_imageReaderMutex);

    return this->d->m_imageReader.supportsAnimation();
}

bool ImageSrcElement::forceFps() const
{
    return this->d->m_forceFps;
}

AkFrac ImageSrcElement::fps() const
{
    QReadLocker locker(&this->d->m_fpsMutex);

    return this->d->m_fps;
}

QStringList ImageSrcElement::supportedFormats() const
{
    QStringList supportedFormats;

    for (auto &format: QImageReader::supportedImageFormats())
        supportedFormats << format.toLower();

    return supportedFormats;
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
    {
        QWriteLocker locker(&this->d->m_fpsMutex);

        if (this->d->m_fps == fps)
            return;

        this->d->m_fps = fps;
    }

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
    QString fileName;

    {
        QReadLocker locker(&this->d->m_imageReaderMutex);
        fileName = this->d->m_imageReader.fileName();
    }

    if (fileName == media)
        return;

    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    QSize size;
    bool isAnimation = false;

    {
        QWriteLocker locker(&this->d->m_imageReaderMutex);
        size = this->d->m_imageReader.size();
        isAnimation = this->d->m_imageReader.supportsAnimation();
        this->d->m_imageReader.setFileName(media);
    }

    if (!media.isEmpty())
        this->setState(state);

    emit this->mediaChanged(media);

    QSize curSize;
    bool curIsAnimation = false;

    {
        QReadLocker locker(&this->d->m_imageReaderMutex);
        curSize = this->d->m_imageReader.size();
        curIsAnimation = this->d->m_imageReader.supportsAnimation();
    }

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
    {
        QReadLocker locker(&this->d->m_imageReaderMutex);

        if (this->d->m_imageReader.fileName().isEmpty())
            return false;
    }

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
                                      &ImageSrcElementPrivate::readFrame,
                                      this->d);

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
                                      &ImageSrcElementPrivate::readFrame,
                                      this->d);

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
        AkFrac fps;

        {
            QReadLocker locker(&this->m_fpsMutex);
            fps = this->m_fps;
        }

        QImage image;
        QString error;

        {
            QReadLocker locker(&this->m_imageReaderMutex);
            image = this->m_imageReader.read();
            error = this->m_imageReader.errorString();
        }

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
        packet.setDuration(1);
        packet.setTimeBase(fps.invert());
        packet.setIndex(0);
        packet.setId(this->m_id);

        if (!this->m_threadedRead) {
            emit self->oStream(packet);
        } else if (!this->m_threadStatus.isRunning()) {
            this->m_threadStatus =
                    QtConcurrent::run(&this->m_threadPool,
                                      &ImageSrcElementPrivate::sendPacket,
                                      this,
                                      packet);
        }

        bool isLastFrame = false;

        {
            QReadLocker locker(&this->m_imageReaderMutex);
            isLastFrame = this->m_imageReader.currentImageNumber() >= this->m_imageReader.imageCount() - 1;
        }

        if (isLastFrame) {
            bool supportsAnimation = false;

            {
                QReadLocker locker(&this->m_imageReaderMutex);
                supportsAnimation = this->m_imageReader.supportsAnimation();
            }

            if (!supportsAnimation) {
                auto delay = (1000 / fps).value() + delayDiff;
                delayDiff = delay - qRound(delay);
                QThread::msleep(qRound(delay));
            }

            {
                QWriteLocker locker(&this->m_imageReaderMutex);
                auto fileName = this->m_imageReader.fileName();
                this->m_imageReader.setFileName({});
                this->m_imageReader.setFileName(fileName);
            }
        } else {
            if (this->m_forceFps) {
                auto delay = (1000 / fps).value() + delayDiff;
                delayDiff = delay - qRound(delay);
                QThread::msleep(qRound(delay));
            } else {
                int delay = 0;

                {
                    QReadLocker locker(&this->m_imageReaderMutex);
                    delay = this->m_imageReader.nextImageDelay();
                }

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
