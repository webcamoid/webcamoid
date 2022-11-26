/* Webcamoid, webcam capture application.
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

#include <QVariant>
#include <QMap>
#include <QDir>
#include <QStandardPaths>
#include <QPainter>
#include <QQmlContext>
#include <QPainterPath>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "facedetectelement.h"
#include "haar/haardetector.h"

using PenStyleMap = QMap<Qt::PenStyle, QString>;

inline PenStyleMap initPenStyleMap()
{
    PenStyleMap markerStyleToStr {
        {Qt::SolidLine     , "solid"     },
        {Qt::DashLine      , "dash"      },
        {Qt::DotLine       , "dot"       },
        {Qt::DashDotLine   , "dashDot"   },
        {Qt::DashDotDotLine, "dashDotDot"}
    };

    return markerStyleToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(PenStyleMap, markerStyleToStr, (initPenStyleMap()))

class FaceDetectElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QString m_haarFile {":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml"};
        FaceDetectElement::MarkerType m_markerType {FaceDetectElement::MarkerTypeRectangle};
        QPen m_markerPen;
        QString m_markerImage {":/FaceDetect/share/masks/cow.png"};
        QString m_backgroundImage {":/FaceDetect/share/background/black_square.png"};
        QImage m_markerImg;
        QImage m_backgroundImg;
        QSize m_pixelGridSize {32, 32};
        QSize m_scanSize {160, 120};
        AkElementPtr m_blurFilter {akPluginManager->create<AkElement>("VideoFilter/Blur")};
        HaarDetector m_cascadeClassifier;
        qreal m_scale {1.0};
        qreal m_rScale {1.0};
        bool m_smootheEdges {false};
        int m_hOffset {0};
        int m_vOffset {0};
        int m_wAdjust {100};
        int m_hAdjust {100};
        int m_rWAdjust {100};
        int m_rHAdjust {100};
        int m_rHRadius {100};
        int m_rVRadius {100};
};

FaceDetectElement::FaceDetectElement(): AkElement()
{
    this->d = new FaceDetectElementPrivate;
    this->d->m_cascadeClassifier.loadCascade(this->d->m_haarFile);
    this->d->m_markerPen.setColor(QColor(255, 0, 0));
    this->d->m_markerPen.setWidth(3);
    this->d->m_markerPen.setStyle(Qt::SolidLine);
    this->d->m_markerImg = QImage(this->d->m_markerImage);
    this->d->m_backgroundImg = QImage(this->d->m_backgroundImage);
    this->d->m_blurFilter->setProperty("radius", 32);

    QObject::connect(this->d->m_blurFilter.data(),
                     SIGNAL(radiusChanged(int)),
                     this,
                     SIGNAL(blurRadiusChanged(int)));
}

FaceDetectElement::~FaceDetectElement()
{
    delete this->d;
}

QString FaceDetectElement::haarFile() const
{
    return this->d->m_haarFile;
}

FaceDetectElement::MarkerType FaceDetectElement::markerType() const
{
    return this->d->m_markerType;
}

QRgb FaceDetectElement::markerColor() const
{
    return this->d->m_markerPen.color().rgba();
}

int FaceDetectElement::markerWidth() const
{
    return this->d->m_markerPen.width();
}

QString FaceDetectElement::markerStyle() const
{
    return markerStyleToStr->value(this->d->m_markerPen.style());
}

QString FaceDetectElement::markerImage() const
{
    return this->d->m_markerImage;
}

QString FaceDetectElement::backgroundImage() const
{
    return this->d->m_backgroundImage;
}

qreal FaceDetectElement::scale() const
{
    return this->d->m_scale;
}

qreal FaceDetectElement::rScale() const
{
    return this->d->m_rScale;
}

bool FaceDetectElement::smootheEdges() const
{
    return this->d->m_smootheEdges;
}

int FaceDetectElement::hOffset() const
{
    return this->d->m_hOffset;
}

int FaceDetectElement::vOffset() const
{
    return this->d->m_vOffset;
}

int FaceDetectElement::wAdjust() const
{
    return this->d->m_wAdjust;
}

int FaceDetectElement::hAdjust() const
{
    return this->d->m_hAdjust;
}

int FaceDetectElement::rWAdjust() const
{
    return this->d->m_rWAdjust;
}

int FaceDetectElement::rHAdjust() const
{
    return this->d->m_rHAdjust;
}

int FaceDetectElement::rHRadius() const
{
    return this->d->m_rHRadius;
}

int FaceDetectElement::rVRadius() const
{
    return this->d->m_rVRadius;
}

QSize FaceDetectElement::pixelGridSize() const
{
    return this->d->m_pixelGridSize;
}

int FaceDetectElement::blurRadius() const
{
    return this->d->m_blurFilter->property("radius").toInt();
}

QSize FaceDetectElement::scanSize() const
{
    return this->d->m_scanSize;
}

QVector<QRect> FaceDetectElement::detectFaces(const AkVideoPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty() || scanSize.isEmpty())
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QImage iFrame(src.caps().width(), src.caps().height(), QImage::Format_ARGB32);
    auto lineSize = qMin<size_t>(src.lineSize(0), iFrame.bytesPerLine());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = src.constLine(0, y);
        auto dstLine = iFrame.scanLine(y);
        memcpy(dstLine, srcLine, lineSize);
    }

    auto scanFrame = iFrame.scaled(scanSize, Qt::KeepAspectRatio);

    return this->d->m_cascadeClassifier.detect(scanFrame);
}

QString FaceDetectElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FaceDetect/share/qml/main.qml");
}

void FaceDetectElement::controlInterfaceConfigure(QQmlContext *context,
                                                  const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FaceDetect", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket FaceDetectElement::iVideoStream(const AkVideoPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty()
        || scanSize.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QImage iFrame(src.caps().width(),
                  src.caps().height(),
                  QImage::Format_ARGB32);
    auto lineSize = qMin<size_t>(src.lineSize(0), iFrame.bytesPerLine());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = src.constLine(0, y);
        auto dstLine = iFrame.scanLine(y);
        memcpy(dstLine, srcLine, lineSize);
    }

    auto oFrame = iFrame.copy();
    auto scanFrame = iFrame.scaled(scanSize, Qt::KeepAspectRatio);
    qreal scale = 1;

    if (scanFrame.width() == scanSize.width())
        scale = qreal(iFrame.width()) / scanSize.width();
    else
        scale = qreal(iFrame.height()) / scanSize.height();

    this->d->m_cascadeClassifier.setEqualize(true);
    auto vecFaces = this->d->m_cascadeClassifier.detect(scanFrame);

    if (vecFaces.isEmpty()
        && this->d->m_markerType != MarkerTypeBlurOuter
        && this->d->m_markerType != MarkerTypeImageOuter) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    QPainter painter;
    painter.begin(&oFrame);

    /* Many users will want to blur even if no faces were detected! */
    if (this->d->m_markerType == MarkerTypeBlurOuter) {
        AkVideoPacket blurPacket = this->d->m_blurFilter->iStream(packet);
        QImage blurImage(blurPacket.caps().width(),
                         blurPacket.caps().height(),
                         QImage::Format_ARGB32);
        auto lineSize = qMin<size_t>(blurPacket.lineSize(0), blurImage.bytesPerLine());

        for (int y = 0; y < blurPacket.caps().height(); y++) {
            auto srcLine = blurPacket.constLine(0, y);
            auto dstLine = blurImage.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        painter.drawImage(0, 0, blurImage);
        /* for a better effect, we could add a second (weaker) blur */
        /* and copy this to larger boxes around all faces */
    } else if (this->d->m_markerType == MarkerTypeImageOuter) {
        QRect all(0, 0, iFrame.width(), iFrame.height());
        painter.drawImage(all, this->d->m_backgroundImg);
    }

    for (auto &face: vecFaces) {
        QRect rect(int(scale * face.x() + this->d->m_hOffset),
                   int(scale * face.y() + this->d->m_vOffset),
                   int(scale * face.width()),
                   int(scale * face.height()));
        QPoint center = rect.center();
        rect.setWidth(int(rect.width() * this->d->m_scale) * (this->d->m_wAdjust / 100.0));
        rect.setHeight(int(rect.height() * this->d->m_scale) * (this->d->m_hAdjust / 100.0));
        rect.moveCenter(center);

        if (this->d->m_markerType == MarkerTypeRectangle) {
            painter.setPen(this->d->m_markerPen);
            painter.drawRect(rect);
        } else if (this->d->m_markerType == MarkerTypeEllipse) {
            painter.setPen(this->d->m_markerPen);
            painter.drawEllipse(rect);
        } else if (this->d->m_markerType == MarkerTypeImage)
            painter.drawImage(rect, this->d->m_markerImg);
        else if (this->d->m_markerType == MarkerTypePixelate) {
            qreal sw = 1.0 / this->d->m_pixelGridSize.width();
            qreal sh = 1.0 / this->d->m_pixelGridSize.height();
            auto imagePixelate = iFrame.copy(rect);

            imagePixelate = imagePixelate.scaled(int(sw * imagePixelate.width()),
                                                 int(sh * imagePixelate.height()),
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::FastTransformation)
                                         .scaled(imagePixelate.width(),
                                                 imagePixelate.height(),
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::FastTransformation);

            painter.drawImage(rect, imagePixelate);
        } else if (this->d->m_markerType == MarkerTypeBlur) {
            auto subFrame = iFrame.copy(rect);
            auto caps = src.caps();
            caps.setWidth(subFrame.width());
            caps.setHeight(subFrame.height());
            AkVideoPacket rectPacket(caps);
            rectPacket.copyMetadata(src);
            auto lineSize = qMin<size_t>(subFrame.bytesPerLine(), rectPacket.lineSize(0));

            for (int y = 0; y < subFrame.height(); y++) {
                auto srcLine = subFrame.constScanLine(y);
                auto dstLine = rectPacket.line(0, y);
                memcpy(dstLine, srcLine, lineSize);
            }

            AkVideoPacket blurPacket = this->d->m_blurFilter->iStream(rectPacket);
            QImage blurImage(blurPacket.caps().width(),
                             blurPacket.caps().height(),
                             QImage::Format_ARGB32);
            lineSize = qMin<size_t>(blurPacket.lineSize(0), blurImage.bytesPerLine());

            for (int y = 0; y < blurPacket.caps().height(); y++) {
                auto srcLine = blurPacket.constLine(0, y);
                auto dstLine = blurImage.scanLine(y);
                memcpy(dstLine, srcLine, lineSize);
            }

            painter.drawImage(rect, blurImage);
        } else if (this->d->m_markerType == MarkerTypeBlurOuter
                   || this->d->m_markerType == MarkerTypeImageOuter) {
            if (this->d->m_smootheEdges) {
                QPainterPath path;
                QRectF rRect((scale * face.x() + this->d->m_hOffset),
                             (scale * face.y() + this->d->m_vOffset),
                             (scale * face.width()),
                             (scale * face.height()));
                QPointF rCenter = rRect.center();
                rRect.setWidth(rRect.width() * this->d->m_rScale * (this->d->m_rWAdjust / 100.0));
                rRect.setHeight(rRect.height() * this->d->m_rScale * (this->d->m_rHAdjust / 100.0));
                rRect.moveCenter(rCenter);
                path.addRoundedRect(rRect,
                                    this->d->m_rHRadius,
                                    this->d->m_rVRadius,
                                    Qt::RelativeSize);
                painter.setClipPath(path);
            }

            painter.drawImage(rect, iFrame.copy(rect));
        }
    }

    painter.end();

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    lineSize = qMin<size_t>(oFrame.bytesPerLine(), dst.lineSize(0));

    for (int y = 0; y < dst.caps().height(); y++) {
        auto srcLine = oFrame.constScanLine(y);
        auto dstLine = dst.line(0, y);
        memcpy(dstLine, srcLine, lineSize);
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void FaceDetectElement::setHaarFile(const QString &haarFile)
{
    if (this->d->m_haarFile == haarFile)
        return;

    if (this->d->m_cascadeClassifier.loadCascade(haarFile)) {
        this->d->m_haarFile = haarFile;
        emit this->haarFileChanged(haarFile);
    } else if (this->d->m_haarFile != "") {
        this->d->m_haarFile = "";
        emit this->haarFileChanged(this->d->m_haarFile);
    }
}

void FaceDetectElement::setMarkerType(MarkerType markerType)
{
    if (this->d->m_markerType == markerType)
        return;

    this->d->m_markerType = markerType;
    emit this->markerTypeChanged(markerType);
}

void FaceDetectElement::setMarkerColor(QRgb markerColor)
{
    if (this->d->m_markerPen.color() == QColor(markerColor))
        return;

    this->d->m_markerPen.setColor(QColor(markerColor));
    emit this->markerColorChanged(markerColor);
}

void FaceDetectElement::setMarkerWidth(int markerWidth)
{
    if (this->d->m_markerPen.width() == markerWidth)
        return;

    this->d->m_markerPen.setWidth(markerWidth);
    emit this->markerWidthChanged(markerWidth);
}

void FaceDetectElement::setMarkerStyle(const QString &markerStyle)
{
    Qt::PenStyle penStyle = markerStyleToStr->key(markerStyle, Qt::SolidLine);

    if (this->d->m_markerPen.style() == penStyle)
        return;

    this->d->m_markerPen.setStyle(penStyle);
    emit this->markerStyleChanged(markerStyle);
}

void FaceDetectElement::setMarkerImage(const QString &markerImage)
{
    if (this->d->m_markerImage == markerImage)
        return;

    this->d->m_markerImage = markerImage;

    if (!markerImage.isEmpty())
        this->d->m_markerImg = QImage(markerImage);

    emit this->markerImageChanged(markerImage);
}

void FaceDetectElement::setBackgroundImage(const QString &backgroundImage)
{
    if (this->d->m_backgroundImage == backgroundImage)
        return;

    this->d->m_backgroundImage = backgroundImage;

    if (!backgroundImage.isEmpty())
        this->d->m_backgroundImg = QImage(backgroundImage);

    emit this->backgroundImageChanged(backgroundImage);
}

void FaceDetectElement::setScale(qreal scale)
{
    if (this->d->m_scale == scale)
        return;

    this->d->m_scale = scale;
    emit this->scaleChanged(scale);
}

void FaceDetectElement::setRScale(qreal rScale)
{
    if (this->d->m_rScale == rScale)
        return;

    this->d->m_rScale = rScale;
    emit this->rScaleChanged(rScale);
}

void FaceDetectElement::setSmootheEdges(bool smootheEdges)
{
    if (this->d->m_smootheEdges == smootheEdges)
        return;

    this->d->m_smootheEdges = smootheEdges;
    emit this->smootheEdgesChanged(smootheEdges);
}

void FaceDetectElement::setHOffset(int hOffset)
{
    if (this->d->m_hOffset == hOffset)
        return;

    this->d->m_hOffset = hOffset;
    emit this->hOffsetChanged(hOffset);
}

void FaceDetectElement::setVOffset(int vOffset)
{
    if (this->d->m_vOffset == vOffset)
        return;

    this->d->m_vOffset = vOffset;
    emit this->vOffsetChanged(vOffset);
}

void FaceDetectElement::setWAdjust(int wAdjust)
{
    if (this->d->m_wAdjust == wAdjust)
        return;

    this->d->m_wAdjust = wAdjust;
    emit this->wAdjustChanged(wAdjust);
}

void FaceDetectElement::setHAdjust(int hAdjust)
{
    if (this->d->m_hAdjust == hAdjust)
        return;

    this->d->m_hAdjust = hAdjust;
    emit this->hAdjustChanged(hAdjust);
}

void FaceDetectElement::setRWAdjust(int rWAdjust)
{
    if (this->d->m_rWAdjust == rWAdjust)
        return;

    this->d->m_rWAdjust = rWAdjust;
    emit this->rWAdjustChanged(rWAdjust);
}

void FaceDetectElement::setRHAdjust(int rHAdjust)
{
    if (this->d->m_rHAdjust == rHAdjust)
        return;

    this->d->m_rHAdjust = rHAdjust;
    emit this->rHAdjustChanged(rHAdjust);
}

void FaceDetectElement::setRHRadius(int rHRadius)
{
    if (this->d->m_rHRadius == rHRadius)
        return;

    this->d->m_rHRadius = rHRadius;
    emit this->rHRadiusChanged(rHRadius);
}

void FaceDetectElement::setRVRadius(int rVRadius)
{
    if (this->d->m_rVRadius == rVRadius)
        return;

    this->d->m_rVRadius = rVRadius;
    emit this->rVRadiusChanged(rVRadius);
}

void FaceDetectElement::setPixelGridSize(const QSize &pixelGridSize)
{
    if (this->d->m_pixelGridSize == pixelGridSize)
        return;

    this->d->m_pixelGridSize = pixelGridSize;
    emit this->pixelGridSizeChanged(pixelGridSize);
}

void FaceDetectElement::setBlurRadius(int blurRadius)
{
    this->d->m_blurFilter->setProperty("radius", blurRadius);
}

void FaceDetectElement::setScanSize(const QSize &scanSize)
{
    if (this->d->m_scanSize == scanSize)
        return;

    this->d->m_scanSize = scanSize;
    emit this->scanSizeChanged(scanSize);
}

void FaceDetectElement::resetHaarFile()
{
    this->setHaarFile(":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml");
}

void FaceDetectElement::resetMarkerType()
{
    this->setMarkerType(FaceDetectElement::MarkerTypeRectangle);
}

void FaceDetectElement::resetMarkerColor()
{
    this->setMarkerColor(qRgb(255, 0, 0));
}

void FaceDetectElement::resetMarkerWidth()
{
    this->setMarkerWidth(3);
}

void FaceDetectElement::resetMarkerStyle()
{
    this->setMarkerStyle("solid");
}

void FaceDetectElement::resetMarkerImage()
{
    this->setMarkerImage(":/FaceDetect/share/masks/cow.png");
}

void FaceDetectElement::resetBackgroundImage()
{
    this->setBackgroundImage(":/FaceDetect/share/backgrounds/black_square.png");
}

void FaceDetectElement::resetScale()
{
    this->setScale(1.0);
}

void FaceDetectElement::resetRScale()
{
    this->setRScale(1.0);
}

void FaceDetectElement::resetSmootheEdges()
{
    this->setSmootheEdges(false);
}

void FaceDetectElement::resetHOffset()
{
    this->setHOffset(0);
}

void FaceDetectElement::resetVOffset()
{
    this->setVOffset(0);
}

void FaceDetectElement::resetWAdjust()
{
    this->setWAdjust(100);
}

void FaceDetectElement::resetHAdjust()
{
    this->setHAdjust(100);
}

void FaceDetectElement::resetRWAdjust()
{
    this->setRWAdjust(100);
}

void FaceDetectElement::resetRHAdjust()
{
    this->setRHAdjust(100);
}

void FaceDetectElement::resetRHRadius()
{
    this->setRHRadius(0);
}

void FaceDetectElement::resetRVRadius()
{
    this->setRVRadius(0);
}

void FaceDetectElement::resetPixelGridSize()
{
    this->setPixelGridSize(QSize(32, 32));
}

void FaceDetectElement::resetBlurRadius()
{
    this->setBlurRadius(32);
}

void FaceDetectElement::resetScanSize()
{
    this->setScanSize(QSize(160, 120));
}

QDataStream &operator >>(QDataStream &istream, FaceDetectElement::MarkerType &markerType)
{
    int markerTypeInt;
    istream >> markerTypeInt;
    markerType = static_cast<FaceDetectElement::MarkerType>(markerTypeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, FaceDetectElement::MarkerType markerType)
{
    ostream << static_cast<int>(markerType);

    return ostream;
}

#include "moc_facedetectelement.cpp"
