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
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include "akcolorizedimage.h"

#define DEFAULT_WIDTH  16
#define DEFAULT_HEIGHT 16

class AkColorizedImagePrivate
{
    public:
        AkColorizedImage *self;
        QMutex m_mutex;
        QString m_source;
        QString m_origSource;
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

        AkColorizedImagePrivate(AkColorizedImage *self);
        QImage colorizeImage(const QImage &image);
        void scale(const QSize &size,
                   QRectF &sourceRect,
                   QRectF &targetRect) const;
        bool load();
        void loadImage(const QString &source);
};

AkColorizedImage::AkColorizedImage(QQuickItem *parent):
    QQuickItem(parent)
{
    this->d = new AkColorizedImagePrivate(this);
    this->setFlag(ItemHasContents, true);
    this->setImplicitWidth(DEFAULT_WIDTH);
    this->setImplicitHeight(DEFAULT_HEIGHT);
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

    if (!this->d->load())
        return nullptr;

    auto image = this->d->colorizeImage(this->d->m_image);

    if (image.isNull())
        return nullptr;

    auto videoFrame = this->window()->createTextureFromImage(image);

    if (!videoFrame)
        return nullptr;

    if (videoFrame->textureSize().isEmpty()) {
        delete videoFrame;

        return nullptr;
    }

    if (this->smooth())
        videoFrame->setFiltering(QSGTexture::Linear);

    if (this->d->m_mipmap)
        videoFrame->setMipmapFiltering(QSGTexture::Nearest);

    QSGSimpleTextureNode *node = nullptr;

    if (oldNode)
        node = dynamic_cast<QSGSimpleTextureNode *>(oldNode);
    else
        node = new QSGSimpleTextureNode();

    node->setOwnsTexture(true);

    QRectF sourceRect;
    QRectF targetRect;
    this->d->scale(image.size(), sourceRect, targetRect);
    node->setSourceRect(sourceRect);
    node->setRect(targetRect);

    if (!qFuzzyCompare(this->d->m_paintedWidth, targetRect.width())
        || !qFuzzyCompare(this->d->m_paintedHeight, targetRect.height())) {
        this->d->m_paintedWidth = targetRect.width();
        this->d->m_paintedHeight = targetRect.height();
        emit this->paintedGeometryChanged();
    }

    if (this->smooth())
        node->setFiltering(QSGTexture::Linear);

    if (this->d->m_mirror)
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorHorizontally);

    node->setTexture(videoFrame);

    return node;
}

void AkColorizedImage::setSource(const QString &source)
{
    if (this->d->m_source == source)
        return;

    this->d->m_mutex.lock();
    this->d->m_source = source;
    this->d->m_mutex.unlock();
    emit this->sourceChanged(source);

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

    this->d->m_mutex.lock();
    this->d->m_color = color;
    this->d->m_mutex.unlock();
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

    this->d->m_mutex.lock();
    this->d->m_sourceSize = sourceSize;
    this->d->m_mutex.unlock();
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

void AkColorizedImage::registerTypes()
{
    qmlRegisterType<AkColorizedImage>("Ak", 1, 0, "AkColorizedImage");
    qRegisterMetaType<FillMode>("FillMode");
    qRegisterMetaType<HorizontalAlignment>("HorizontalAlignment");
    qRegisterMetaType<VerticalAlignment>("VerticalAlignment");
    qRegisterMetaType<Status>("Status");
}

AkColorizedImagePrivate::AkColorizedImagePrivate(AkColorizedImage *self):
    self(self)
{

}

QImage AkColorizedImagePrivate::colorizeImage(const QImage &image)
{
    QImage colorizedImage(image.size(), image.format());

    this->m_mutex.lock();
    auto color = this->m_color;
    this->m_mutex.unlock();

    for (int y = 0; y < image.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(colorizedImage.scanLine(y));

        for (int x = 0; x < image.width(); x++) {
            auto gray = qGray(srcLine[x]);
            auto srcR = qRed(srcLine[x]);
            auto srcG = qGreen(srcLine[x]);
            auto srcB = qBlue(srcLine[x]);
            auto dstR = color.red();
            auto dstG = color.green();
            auto dstB = color.blue();
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

            r = (color.alpha() * (r - srcR) + 255 * srcR) / 255;
            g = (color.alpha() * (g - srcG) + 255 * srcG) / 255;
            b = (color.alpha() * (b - srcB) + 255 * srcB) / 255;

            auto a = qAlpha(srcLine[x]);
            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return colorizedImage;
}

void AkColorizedImagePrivate::scale(const QSize &size,
                                    QRectF &sourceRect,
                                    QRectF &targetRect) const
{
    auto boundingSize = self->boundingRect().size();

    switch (this->m_fillMode) {
    case AkColorizedImage::Stretch:
        sourceRect.setSize(size);
        targetRect.setSize(boundingSize);

        break;

    case AkColorizedImage::PreserveAspectFit: {
        QSizeF tmpSize = size;
        tmpSize.scale(boundingSize, Qt::KeepAspectRatio);
        sourceRect.setSize(size);
        targetRect.setSize(tmpSize);

        break;
    }

    case AkColorizedImage::PreserveAspectCrop: {
        auto tmpSize = boundingSize;
        tmpSize.scale(size, Qt::KeepAspectRatio);
        sourceRect.setSize(tmpSize);
        targetRect.setSize(boundingSize);

        break;
    }

    case AkColorizedImage::Pad: {
        QSizeF tmpSize(qMin(qreal(size.width()) , boundingSize.width()),
                       qMin(qreal(size.height()), boundingSize.height()));
        sourceRect.setSize(tmpSize);
        targetRect.setSize(tmpSize);

        break;
    }
    }

    switch (this->m_horizontalAlignment) {
    case AkColorizedImage::AlignRight:
        sourceRect = {size.width() - sourceRect.width(),
                      sourceRect.y(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {boundingSize.width() - targetRect.width(),
                      targetRect.y(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    case AkColorizedImage::AlignHCenter:
        sourceRect = {(size.width() - sourceRect.width()) / 2,
                      sourceRect.y(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {(boundingSize.width() - targetRect.width()) / 2,
                      targetRect.y(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    default:
        break;
    }

    switch (this->m_verticalAlignment) {
    case AkColorizedImage::AlignBottom:
        sourceRect = {sourceRect.x(),
                      size.height() - sourceRect.height(),
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {targetRect.x(),
                      boundingSize.height() - targetRect.height(),
                      targetRect.width(),
                      targetRect.height()};

        break;

    case AkColorizedImage::AlignVCenter:
        sourceRect = {sourceRect.x(),
                      (size.height() - sourceRect.height()) / 2,
                      sourceRect.width(),
                      sourceRect.height()};
        targetRect = {targetRect.x(),
                      (boundingSize.height() - targetRect.height()) / 2,
                      targetRect.width(),
                      targetRect.height()};

        break;

    default:
        break;
    }
}

bool AkColorizedImagePrivate::load()
{
    this->m_mutex.lock();
    auto source = this->m_source;
    this->m_mutex.unlock();

    if (!source.isEmpty()) {
        if (!this->m_cache || source != this->m_origSource) {
            if (!qFuzzyCompare(this->m_progress, 0.0)) {
                this->m_progress = 0.0;
                emit self->progressChanged(this->m_progress);
            }

            this->m_status = AkColorizedImage::Loading;
            emit self->statusChanged(this->m_status);

            this->loadImage(source);
            this->m_origSource = this->m_image.isNull()? "": source;

            if (!this->m_image.isNull()) {
                this->m_progress = 1.0;
                emit self->progressChanged(this->m_progress);
            }

            this->m_status = this->m_image.isNull()?
                                 AkColorizedImage::Error:
                                 AkColorizedImage::Ready;
            emit self->statusChanged(this->m_status);

            if (this->m_status == AkColorizedImage::Ready) {
                self->setImplicitWidth(this->m_image.width());
                self->setImplicitHeight(this->m_image.height());
            }
        }

        return this->m_status == AkColorizedImage::Ready;
    }

    this->m_image = {};
    this->m_origSource = "";

    if (this->m_status != AkColorizedImage::Null) {
        this->m_status = AkColorizedImage::Null;
        emit self->statusChanged(this->m_status);
    }

    if (!qFuzzyCompare(this->m_progress, 0.0)) {
        this->m_progress = 0.0;
        emit self->progressChanged(this->m_progress);
    }

    self->setImplicitWidth(DEFAULT_WIDTH);
    self->setImplicitHeight(DEFAULT_HEIGHT);

    return false;
}

void AkColorizedImagePrivate::loadImage(const QString &source)
{
    if (source.startsWith("image://")) {
        auto providerId = source.section('/', 2, 2);
        auto resourceId = source.section('/', 3);
        auto context = QQmlEngine::contextForObject(self);
        this->m_image = {};

        if (!context)
            return;

        auto engine = context->engine();

        if (!engine)
            return;

        auto imageProvider =
                static_cast<QQuickImageProvider *>(engine->imageProvider(providerId));

        if (!imageProvider)
            return;

        this->m_mutex.lock();
        auto sourceSize = this->m_sourceSize;
        this->m_mutex.unlock();

        QSize resourceSize = sourceSize.isEmpty()?
                                 self->size().toSize():
                                 sourceSize;
        this->m_image =
                imageProvider->requestImage(resourceId,
                                            &resourceSize,
                                            resourceSize);
    } else {
        auto tmpSource = source;

        if (tmpSource.startsWith("file://"))
            tmpSource.remove(QRegExp("^file://"));

        this->m_image = QImage(tmpSource);
    }
}

#include "moc_akcolorizedimage.cpp"
