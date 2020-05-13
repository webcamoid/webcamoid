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

#ifndef AKCOLORIZEDIMAGE_H
#define AKCOLORIZEDIMAGE_H

#include <QQuickItem>

#include "../akcommons.h"

class AkColorizedImagePrivate;

class AKCOMMONS_EXPORT AkColorizedImage: public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(AkColorizedImage)
    Q_ENUMS(FillMode)
    Q_ENUMS(HorizontalAlignment)
    Q_ENUMS(VerticalAlignment)
    Q_ENUMS(Status)
    Q_PROPERTY(QString source
               READ source
               WRITE setSource
               RESET resetSource
               NOTIFY sourceChanged)
    Q_PROPERTY(bool cache
               READ cache
               WRITE setCache
               RESET resetCache
               NOTIFY cacheChanged)
    Q_PROPERTY(QColor color
               READ color
               WRITE setColor
               RESET resetColor
               NOTIFY colorChanged)
    Q_PROPERTY(FillMode fillMode
               READ fillMode
               WRITE setFillMode
               RESET resetFillMode
               NOTIFY fillModeChanged)
    Q_PROPERTY(QSize sourceSize
               READ sourceSize
               WRITE setSourceSize
               RESET resetSourceSize
               NOTIFY sourceSizeChanged)
    Q_PROPERTY(qreal paintedWidth
               READ paintedWidth
               NOTIFY paintedGeometryChanged)
    Q_PROPERTY(qreal paintedHeight
               READ paintedHeight
               NOTIFY paintedGeometryChanged)
    Q_PROPERTY(HorizontalAlignment horizontalAlignment
               READ horizontalAlignment
               WRITE setHorizontalAlignment
               RESET resetHorizontalAlignment
               NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VerticalAlignment verticalAlignment
               READ verticalAlignment
               WRITE setVerticalAlignment
               RESET resetVerticalAlignment
               NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(Status status
               READ status
               NOTIFY statusChanged)
    Q_PROPERTY(bool mirror
               READ mirror
               WRITE setMirror
               RESET resetMirror
               NOTIFY mirrorChanged)
    Q_PROPERTY(bool asynchronous
               READ asynchronous
               WRITE setAsynchronous
               RESET resetAsynchronous
               NOTIFY asynchronousChanged)
    Q_PROPERTY(bool mipmap
               READ mipmap
               WRITE setMipmap
               RESET resetMipmap
               NOTIFY mipmapChanged)
    Q_PROPERTY(qreal progress
               READ progress
               NOTIFY progressChanged)

    public:
        enum FillMode {
            Stretch,
            PreserveAspectFit,
            PreserveAspectCrop,
            Pad
        };

        enum HorizontalAlignment {
            AlignLeft = Qt::AlignLeft,
            AlignRight = Qt::AlignRight,
            AlignHCenter = Qt::AlignHCenter
        };

        enum VerticalAlignment {
            AlignTop = Qt::AlignTop,
            AlignBottom = Qt::AlignBottom,
            AlignVCenter = Qt::AlignVCenter
        };

        enum Status {
            Null,
            Ready,
            Loading,
            Error
        };

        explicit AkColorizedImage(QQuickItem *parent=nullptr);
        ~AkColorizedImage();

        Q_INVOKABLE QString source() const;
        Q_INVOKABLE bool cache() const;
        Q_INVOKABLE QColor color() const;
        Q_INVOKABLE FillMode fillMode() const;
        Q_INVOKABLE QSize sourceSize() const;
        Q_INVOKABLE qreal paintedWidth() const;
        Q_INVOKABLE qreal paintedHeight() const;
        Q_INVOKABLE HorizontalAlignment horizontalAlignment() const;
        Q_INVOKABLE VerticalAlignment verticalAlignment() const;
        Q_INVOKABLE Status status() const;
        Q_INVOKABLE bool mirror() const;
        Q_INVOKABLE bool asynchronous() const;
        Q_INVOKABLE bool mipmap() const;
        Q_INVOKABLE qreal progress() const;

    private:
        AkColorizedImagePrivate *d;

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode,
                                 UpdatePaintNodeData *updatePaintNodeData);


    signals:
        void sourceChanged(const QString &source);
        void cacheChanged(bool cache);
        void colorChanged(const QColor &color);
        void fillModeChanged(FillMode fillMode);
        void sourceSizeChanged(const QSize &sourceSize);
        void paintedGeometryChanged();
        void horizontalAlignmentChanged(HorizontalAlignment horizontalAlignment);
        void verticalAlignmentChanged(VerticalAlignment verticalAlignment);
        void statusChanged(Status status);
        void mirrorChanged(bool mirror);
        void asynchronousChanged(bool asynchronous);
        void mipmapChanged(bool mipmap);
        void progressChanged(qreal progress);

    public slots:
        void setSource(const QString &source);
        void setCache(bool cache);
        void setColor(const QColor &color);
        void setFillMode(FillMode fillMode);
        void setSourceSize(const QSize &sourceSize);
        void setHorizontalAlignment(HorizontalAlignment horizontalAlignment);
        void setVerticalAlignment(VerticalAlignment verticalAlignment);
        void setMirror(bool mirror);
        void setAsynchronous(bool asynchronous);
        void setMipmap(bool mipmap);
        void resetSource();
        void resetCache();
        void resetColor();
        void resetFillMode();
        void resetSourceSize();
        void resetHorizontalAlignment();
        void resetVerticalAlignment();
        void resetMirror();
        void resetAsynchronous();
        void resetMipmap();
        static void registerTypes();

        friend class AkColorizedImagePrivate;
};

Q_DECLARE_METATYPE(AkColorizedImage::FillMode)
Q_DECLARE_METATYPE(AkColorizedImage::HorizontalAlignment)
Q_DECLARE_METATYPE(AkColorizedImage::VerticalAlignment)
Q_DECLARE_METATYPE(AkColorizedImage::Status)

#endif // AKCOLORIZEDIMAGE_H
