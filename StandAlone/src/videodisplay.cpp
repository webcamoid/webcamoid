/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

#include <QElapsedTimer>
#include <QMutex>
#include <QQuickWindow>
#include <QReadWriteLock>
#include <QSGSimpleTextureNode>
#include <akfrac.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "videodisplay.h"

class VideoDisplayPrivate
{
    public:
        VideoDisplay *self;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QImage m_frame;
        QMutex m_inputMutex;
        QReadWriteLock m_updateMutex;
        QElapsedTimer m_timer;
        qint64 m_lastTime {0};
        int m_frameCount {0};
        qreal m_elapsedTime {0.0};
        bool m_fillDisplay {false};

        VideoDisplayPrivate(VideoDisplay *self);
        QSGTexture *createVideoTexture(const QImage &frame) const;
        QRectF calculateTextureRect(const QSGTexture *texture) const;
};

VideoDisplay::VideoDisplay(QQuickItem *parent):
    QQuickItem(parent)
{
    this->d = new VideoDisplayPrivate(this);
    this->setFlag(ItemHasContents, true);
    this->setImplicitWidth(640);
    this->setImplicitHeight(480);
}

VideoDisplay::~VideoDisplay()
{
    delete this->d;
}

bool VideoDisplay::fillDisplay() const
{
    return this->d->m_fillDisplay;
}

QSGNode *VideoDisplay::updatePaintNode(QSGNode *oldNode,
                                       QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

#if 0
    // Start the timer if it's the first call
    if (!this->d->m_timer.isValid()) {
        this->d->m_timer.start();
        this->d->m_lastTime = this->d->m_timer.nsecsElapsed();
    }

    // Measure current time
    auto currentTime = this->d->m_timer.nsecsElapsed();
    auto deltaTime = (currentTime - this->d->m_lastTime) / 1e9; // Convert to seconds
    this->d->m_lastTime = currentTime;

    // Update counters
    this->d->m_frameCount++;
    this->d->m_elapsedTime += deltaTime;

    // Calculate and display FPS every second
    if (this->d->m_elapsedTime >= 1.0) {
        auto fps = this->d->m_frameCount / this->d->m_elapsedTime;
        qDebug() << "FPS:" << fps;

        // Reset counters
        this->d->m_frameCount = 0;
        this->d->m_elapsedTime = 0.0;
    }
#endif

    this->d->m_updateMutex.lockForRead();
    auto videoFrame = this->d->createVideoTexture(this->d->m_frame);
    this->d->m_updateMutex.unlock();

    if (!videoFrame)
        return nullptr;

    if (videoFrame->textureSize().isEmpty()) {
        delete videoFrame;

        return nullptr;
    }

    auto node = static_cast<QSGSimpleTextureNode *>(oldNode);

    if (!node)
        node = new QSGSimpleTextureNode();

    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(this->d->calculateTextureRect(videoFrame));
    node->setTexture(videoFrame);

    return node;
}

void VideoDisplay::iStream(const AkPacket &packet)
{
    if (this->d->m_inputMutex.tryLock()) {
        this->d->m_videoConverter.begin();
        auto src = this->d->m_videoConverter.convert(packet);
        this->d->m_videoConverter.end();

        this->d->m_updateMutex.lockForWrite();

        if (this->d->m_frame.width() != src.caps().width()
             || this->d->m_frame.height() != src.caps().height())
            this->d->m_frame = QImage(src.caps().width(),
                                      src.caps().height(),
                                      QImage::Format_ARGB32);

        auto lineSize =
            qMin<size_t>(src.lineSize(0), this->d->m_frame.bytesPerLine());

        for (int y = 0; y < src.caps().height(); y++) {
            auto srcLine = src.constLine(0, y);
            auto dstLine = this->d->m_frame.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        this->d->m_updateMutex.unlock();

        QMetaObject::invokeMethod(this, "update");
        this->d->m_inputMutex.unlock();
    }
}

void VideoDisplay::setFillDisplay(bool fillDisplay)
{
    if (this->d->m_fillDisplay == fillDisplay)
        return;

    this->d->m_fillDisplay = fillDisplay;
    emit this->fillDisplayChanged();
}

void VideoDisplay::resetFillDisplay()
{
    this->setFillDisplay(false);
}

void VideoDisplay::registerTypes()
{
    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");
}

VideoDisplayPrivate::VideoDisplayPrivate(VideoDisplay *self):
    self(self)
{

}

QSGTexture *VideoDisplayPrivate::createVideoTexture(const QImage &frame) const
{
    if (frame.isNull())
        return nullptr;

    auto window = self->window();

    if (!window)
        return nullptr;

    auto graphicsApi = window->rendererInterface()->graphicsApi();

    if (graphicsApi == QSGRendererInterface::Software) {
        auto targetSize = self->size().toSize();

        if (frame.size() != targetSize) {
            Qt::TransformationMode mode =
                    self->smooth()?
                        Qt::SmoothTransformation:
                        Qt::FastTransformation;
            auto scaledFrame =
                    frame.scaled(targetSize,
                                 this->m_fillDisplay?
                                     Qt::IgnoreAspectRatio:
                                     Qt::KeepAspectRatio,
                                 mode);

            return window->createTextureFromImage(scaledFrame);
        }
    }

    return window->createTextureFromImage(frame);
}

QRectF VideoDisplayPrivate::calculateTextureRect(const QSGTexture *texture) const
{
    if (this->m_fillDisplay)
        return self->boundingRect();

    QSizeF size(texture->textureSize());
    size.scale(self->boundingRect().size(), Qt::KeepAspectRatio);
    QRectF rect(QPointF(), size);
    rect.moveCenter(self->boundingRect().center());

    return rect;
}

#include "moc_videodisplay.cpp"
