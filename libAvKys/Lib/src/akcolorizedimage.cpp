/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
#include <QPainter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include "akcolorizedimage.h"

class AkColorizedImagePrivate
{
    public:
        AkColorizedImage *self;
        QMutex m_mutex;
        QString m_source;
        QImage m_image;
        QColor m_color {qRgba(0, 0, 0, 0)};
        QSize m_sourceSize;
        qreal m_paintedWidth {0};
        qreal m_paintedHeight {0};
        AkColorizedImage::FillMode m_fillMode {AkColorizedImage::Stretch};
        AkColorizedImage::HorizontalAlignment m_horizontalAlignment {AkColorizedImage::AlignHCenter};
        AkColorizedImage::VerticalAlignment m_verticalAlignment {AkColorizedImage::AlignVCenter};
        AkColorizedImage::Status m_status {AkColorizedImage::Null};
        qreal m_progress {0.0};
        bool m_mirror {false};
        bool m_cache {true};
        bool m_asynchronous {false};
        bool m_mipmap {false};
        bool m_sourceChanged {false};

        AkColorizedImagePrivate(AkColorizedImage *self);
        QImage colorizeImage(const QImage &frame) const;
        void loadImage();
};

AkColorizedImage::AkColorizedImage(QQuickItem *parent):
    QQuickItem(parent)
{
    this->d = new AkColorizedImagePrivate(this);
    this->setFlag(ItemHasContents, true);
    this->setImplicitWidth(250);
    this->setImplicitHeight(250);
}

AkColorizedImage::~AkColorizedImage()
{
    delete this->d;
}

QString AkColorizedImage::source() const
{
    return this->d->m_source;
}

bool AkColorizedImage::cache() const
{
    return this->d->m_cache;
}

QColor AkColorizedImage::color() const
{
    return this->d->m_color;
}

AkColorizedImage::FillMode AkColorizedImage::fillMode() const
{
    return this->d->m_fillMode;
}

QSize AkColorizedImage::sourceSize() const
{
    return this->d->m_sourceSize;
}

qreal AkColorizedImage::paintedWidth() const
{
    return this->d->m_paintedWidth;
}

qreal AkColorizedImage::paintedHeight() const
{
    return this->d->m_paintedHeight;
}

AkColorizedImage::HorizontalAlignment AkColorizedImage::horizontalAlignment() const
{
    return this->d->m_horizontalAlignment;
}

AkColorizedImage::VerticalAlignment AkColorizedImage::verticalAlignment() const
{
    return this->d->m_verticalAlignment;
}

AkColorizedImage::Status AkColorizedImage::status() const
{
    return this->d->m_status;
}

bool AkColorizedImage::mirror() const
{
    return this->d->m_mirror;
}

bool AkColorizedImage::asynchronous() const
{
    return this->d->m_asynchronous;
}

bool AkColorizedImage::mipmap() const
{
    return this->d->m_mipmap;
}

qreal AkColorizedImage::progress() const
{
    return this->d->m_progress;
}

QSGNode *AkColorizedImage::updatePaintNode(QSGNode *oldNode,
                                           QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    if (!this->window())
        return nullptr;

    if (!this->isVisible())
        return nullptr;

    if (!this->d->m_cache
        || (this->d->m_asynchronous && this->d->m_sourceChanged)) {
        if (!qFuzzyCompare(this->d->m_progress, 0.0)) {
            this->d->m_progress = 0.0;
            emit this->progressChanged(this->d->m_progress);
        }

        this->d->m_status = Loading;
        emit this->statusChanged(this->d->m_status);

        this->d->loadImage();
        this->d->m_sourceChanged = false;
    }

    if (this->d->m_status != Ready)
        return nullptr;

    this->d->m_mutex.lock();
    auto image = this->d->colorizeImage(this->d->m_image);
    this->d->m_mutex.unlock();

    if (image.isNull())
        return nullptr;

    QImage frame(this->boundingRect().size().toSize(), QImage::Format_ARGB32);
    frame.fill(0);

    if (!this->d->m_sourceSize.isEmpty())
        image = image.scaled(this->d->m_sourceSize,
                             Qt::KeepAspectRatio,
                             this->smooth() || this->d->m_mipmap?
                                 Qt::SmoothTransformation:
                                 Qt::FastTransformation);

    if (this->d->m_mirror)
        image = image.mirrored(true, false);

    QRect targetRect;
    QRect sourceRect;

    switch (this->d->m_fillMode) {
    case Stretch:
        sourceRect.setSize(image.size());
        targetRect.setSize(frame.size());

        break;

    case PreserveAspectFit: {
        auto size = image.size();
        size.scale(frame.size(), Qt::KeepAspectRatio);
        sourceRect.setSize(image.size());
        targetRect.setSize(size);

        break;
    }

    case PreserveAspectCrop: {
        auto size = frame.size();
        size.scale(image.size(), Qt::KeepAspectRatio);
        sourceRect.setSize(size);
        targetRect.setSize(frame.size());

        break;
    }

    case Pad: {
        QSize size(qMin(image.width(), frame.width()),
                   qMin(image.height(), frame.height()));
        sourceRect.setSize(size);
        targetRect.setSize(size);

        break;
    }
    }

    switch (this->d->m_horizontalAlignment) {
    case AlignRight:
        sourceRect = {image.width() - sourceRect.width(),
                      sourceRect.y(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {frame.width() - targetRect.width(),
                      targetRect.y(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    case AlignHCenter:
        sourceRect = {(image.width() - sourceRect.width()) / 2,
                      sourceRect.y(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {(frame.width() - targetRect.width()) / 2,
                      targetRect.y(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    default:
        break;
    }

    switch (this->d->m_verticalAlignment) {
    case AlignBottom:
        sourceRect = {sourceRect.x(),
                      image.height() - sourceRect.height(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {targetRect.x(),
                      frame.height() - targetRect.height(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    case AlignVCenter:
        sourceRect = {sourceRect.x(),
                      (image.height() - sourceRect.height()) / 2,
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {targetRect.x(),
                      (frame.height() - targetRect.height()) / 2,
                      targetRect.width(),
                      targetRect.height()};

        break;

    default:
        break;
    }

    this->d->m_paintedWidth = targetRect.width();
    this->d->m_paintedHeight = targetRect.height();
    emit this->paintedGeometryChanged();

    QPainter painter;
    painter.begin(&frame);

    if (this->smooth() || this->d->m_mipmap)
        painter.setRenderHints(QPainter::SmoothPixmapTransform
                               | QPainter::LosslessImageRendering
                               | QPainter::Antialiasing
                               | QPainter::HighQualityAntialiasing);

    painter.drawImage(targetRect, image, sourceRect);
    painter.end();

    auto videoFrame = this->window()->createTextureFromImage(frame);

    if (!videoFrame)
        return nullptr;

    if (videoFrame->textureSize().isEmpty()) {
        delete videoFrame;

        return nullptr;
    }

    QSGSimpleTextureNode *node = nullptr;

    if (oldNode)
        node = dynamic_cast<QSGSimpleTextureNode *>(oldNode);
    else
        node = new QSGSimpleTextureNode();

    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(this->boundingRect());
    node->setTexture(videoFrame);

    return node;
}

void AkColorizedImage::setSource(const QString &source)
{
    if (this->d->m_source == source)
        return;

    this->d->m_source = source;
    emit this->sourceChanged(source);

    this->d->m_sourceChanged = true;

    if (this->d->m_cache && !this->d->m_asynchronous)
        this->d->loadImage();

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setCache(bool cache)
{
    if (this->d->m_cache == cache)
        return;

    this->d->m_cache = cache;
    emit this->cacheChanged(cache);
}

void AkColorizedImage::setColor(const QColor &color)
{
    if (this->d->m_color == color)
        return;

    this->d->m_color = color;
    emit this->colorChanged(color);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setFillMode(FillMode fillMode)
{
    if (this->d->m_fillMode == fillMode)
        return;

    this->d->m_fillMode = fillMode;
    emit this->fillModeChanged(this->d->m_fillMode);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setSourceSize(const QSize &sourceSize)
{
    if (this->d->m_sourceSize == sourceSize)
        return;

    this->d->m_sourceSize = sourceSize;
    emit this->sourceSizeChanged(this->d->m_sourceSize);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setHorizontalAlignment(AkColorizedImage::HorizontalAlignment horizontalAlignment)
{
    if (this->d->m_horizontalAlignment == horizontalAlignment)
        return;

    this->d->m_horizontalAlignment = horizontalAlignment;
    emit this->horizontalAlignmentChanged(this->d->m_horizontalAlignment);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setVerticalAlignment(AkColorizedImage::VerticalAlignment verticalAlignment)
{
    if (this->d->m_verticalAlignment == verticalAlignment)
        return;

    this->d->m_verticalAlignment = verticalAlignment;
    emit this->verticalAlignmentChanged(this->d->m_verticalAlignment);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setMirror(bool mirror)
{
    if (this->d->m_mirror == mirror)
        return;

    this->d->m_mirror = mirror;
    emit this->mirrorChanged(this->d->m_mirror);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::setAsynchronous(bool asynchronous)
{
    if (this->d->m_asynchronous == asynchronous)
        return;

    this->d->m_asynchronous = asynchronous;
    emit this->asynchronousChanged(this->d->m_asynchronous);
}

void AkColorizedImage::setMipmap(bool mipmap)
{
    if (this->d->m_mipmap == mipmap)
        return;

    this->d->m_mipmap = mipmap;
    emit this->mipmapChanged(this->d->m_mipmap);

    QMetaObject::invokeMethod(this, "update");
}

void AkColorizedImage::resetSource()
{
    this->setSource("");
}

void AkColorizedImage::resetCache()
{
    this->setCache(true);
}

void AkColorizedImage::resetColor()
{
    this->setColor(qRgba(0, 0, 0, 0));
}

void AkColorizedImage::resetFillMode()
{
    this->setFillMode(Stretch);
}

void AkColorizedImage::resetSourceSize()
{
    this->setSourceSize({});
}

void AkColorizedImage::resetHorizontalAlignment()
{
    this->setHorizontalAlignment(AlignHCenter);
}

void AkColorizedImage::resetVerticalAlignment()
{
    this->setVerticalAlignment(AlignVCenter);
}

void AkColorizedImage::resetMirror()
{
    this->setMirror(false);
}

void AkColorizedImage::resetAsynchronous()
{
    this->setAsynchronous(false);
}

void AkColorizedImage::resetMipmap()
{
    this->setMipmap(false);
}

AkColorizedImagePrivate::AkColorizedImagePrivate(AkColorizedImage *self):
    self(self)
{

}

QImage AkColorizedImagePrivate::colorizeImage(const QImage &frame) const
{
    QImage colorizedFrame(frame.size(), frame.format());

    for (int y = 0; y < frame.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(frame.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(colorizedFrame.scanLine(y));

        for (int x = 0; x < frame.width(); x++) {
            auto gray = qGray(srcLine[x]);
            auto srcR = qRed(srcLine[x]);
            auto srcG = qGreen(srcLine[x]);
            auto srcB = qBlue(srcLine[x]);
            auto dstR = this->m_color.red();
            auto dstG = this->m_color.green();
            auto dstB = this->m_color.blue();
            int r = 0;
            int g = 0;
            int b = 0;

            if (gray < 128) {
                r = gray * dstR / 127;
                g = gray * dstG / 127;
                b = gray * dstB / 127;
            } else {
                r = ((gray - 128) * (255 - dstR) + 127 * dstR) / 127;
                g = ((gray - 128) * (255 - dstG) + 127 * dstG) / 127;
                b = ((gray - 128) * (255 - dstB) + 127 * dstB) / 127;
            }

            r = (this->m_color.alpha() * (r - srcR) + 255 * srcR) / 255;
            g = (this->m_color.alpha() * (g - srcG) + 255 * srcG) / 255;
            b = (this->m_color.alpha() * (b - srcB) + 255 * srcB) / 255;

            auto a = qAlpha(srcLine[x]);
            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return colorizedFrame;
}

void AkColorizedImagePrivate::loadImage()
{
    qreal progress = 0.0;

    this->m_mutex.lock();

    if (this->m_source.isEmpty()) {
        this->m_image = QImage();
        this->m_status = AkColorizedImage::Null;
        progress = 0.0;
    } else {
        auto source = this->m_source;

        if (source.startsWith("image://")) {
            auto providerId = source.section('/', 2, 2);
            auto resourceId = source.section('/', 3);
            auto context = QQmlEngine::contextForObject(self);
            this->m_image = QImage();

            if (context) {
                auto engine = context->engine();

                if (engine) {
                    auto imageProvider = static_cast<QQuickImageProvider *>(engine->imageProvider(providerId));

                    if (imageProvider) {
                        QSize resourceSize = this->m_sourceSize.isEmpty()?
                                                 self->size().toSize():
                                                 this->m_sourceSize;
                        this->m_image =
                                imageProvider->requestImage(resourceId,
                                                            &resourceSize,
                                                            resourceSize);
                    }
                }
            }
        } else {
            if (source.startsWith("file://"))
                source.remove(QRegExp("^file://"));

            this->m_image = QImage(source);
        }

        this->m_status = this->m_image.isNull()?
                             AkColorizedImage::Error:
                             AkColorizedImage::Ready;
        progress = this->m_image.isNull()? 0.0: 1.0;
    }

    this->m_mutex.unlock();

    if (this->m_status == AkColorizedImage::Ready) {
        self->setImplicitWidth(this->m_image.width());
        self->setImplicitHeight(this->m_image.height());
    } else {
        self->setImplicitWidth(250);
        self->setImplicitHeight(250);
    }

    if (!qFuzzyCompare(this->m_progress, progress)) {
        this->m_progress = progress;
        emit self->progressChanged(this->m_progress);
    }

    emit self->statusChanged(this->m_status);
}

#include "moc_akcolorizedimage.cpp"
