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
#include <QPainterPath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "facedetect.h"
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

class RadiusCallbacks: public IAkNumericPropertyCallbacks<qint32>
{
    public:
        RadiusCallbacks(FaceDetect *self):
              self(self)
        {
        }

        void valueChanged(qint32 value) override
        {
            emit self->blurRadiusChanged(value);
        }

    private:
        FaceDetect *self;
};

class FaceDetectPrivate
{
    public:
        FaceDetect *self {nullptr};
        QString m_description {QObject::tr("Face detection")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QString m_haarFile {":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml"};
        FaceDetect::MarkerType m_markerType {FaceDetect::MarkerTypeRectangle};
        QPen m_markerPen;
        QString m_markerImage {":/FaceDetect/share/masks/cow.png"};
        QString m_backgroundImage {":/FaceDetect/share/background/black_square.png"};
        QImage m_markerImg;
        QImage m_backgroundImg;
        QSize m_pixelGridSize {32, 32};
        QSize m_scanSize {160, 120};
        IAkVideoFilterPtr m_blurFilter {akPluginManager->create<IAkVideoFilter>("VideoFilter/Blur")};
        RadiusCallbacks *m_radiusCallbacks {nullptr};
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

        explicit FaceDetectPrivate(FaceDetect *self);
};

FaceDetect::FaceDetect(QObject *parent):
      QObject(parent)
{
    this->d = new FaceDetectPrivate(this);
    this->d->m_radiusCallbacks = new RadiusCallbacks(this);
    this->d->m_cascadeClassifier.loadCascade(this->d->m_haarFile);
    this->d->m_markerPen.setColor(QColor(255, 0, 0));
    this->d->m_markerPen.setWidth(3);
    this->d->m_markerPen.setStyle(Qt::SolidLine);
    this->d->m_markerImg = QImage(this->d->m_markerImage);
    this->d->m_backgroundImg = QImage(this->d->m_backgroundImage);
    auto radiusProperty = this->d->m_blurFilter->property<IAkPropertyInt>("radius");
    radiusProperty->setValue(32);
    radiusProperty->subscribe(this->d->m_radiusCallbacks);
}

FaceDetect::~FaceDetect()
{
    delete this->d->m_radiusCallbacks;
    delete this->d;
}

QString FaceDetect::description() const
{
    return this->d->m_description;
}

AkElementType FaceDetect::type() const
{
    return this->d->m_type;
}

AkElementCategory FaceDetect::category() const
{
    return this->d->m_category;
}

void *FaceDetect::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *FaceDetect::create(const QString &id)
{
    Q_UNUSED(id)

    return new FaceDetect;
}

int FaceDetect::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/FaceDetect",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

QString FaceDetect::haarFile() const
{
    return this->d->m_haarFile;
}

FaceDetect::MarkerType FaceDetect::markerType() const
{
    return this->d->m_markerType;
}

QRgb FaceDetect::markerColor() const
{
    return this->d->m_markerPen.color().rgba();
}

int FaceDetect::markerWidth() const
{
    return this->d->m_markerPen.width();
}

QString FaceDetect::markerStyle() const
{
    return markerStyleToStr->value(this->d->m_markerPen.style());
}

QString FaceDetect::markerImage() const
{
    return this->d->m_markerImage;
}

QString FaceDetect::backgroundImage() const
{
    return this->d->m_backgroundImage;
}

qreal FaceDetect::scale() const
{
    return this->d->m_scale;
}

qreal FaceDetect::rScale() const
{
    return this->d->m_rScale;
}

bool FaceDetect::smootheEdges() const
{
    return this->d->m_smootheEdges;
}

int FaceDetect::hOffset() const
{
    return this->d->m_hOffset;
}

int FaceDetect::vOffset() const
{
    return this->d->m_vOffset;
}

int FaceDetect::wAdjust() const
{
    return this->d->m_wAdjust;
}

int FaceDetect::hAdjust() const
{
    return this->d->m_hAdjust;
}

int FaceDetect::rWAdjust() const
{
    return this->d->m_rWAdjust;
}

int FaceDetect::rHAdjust() const
{
    return this->d->m_rHAdjust;
}

int FaceDetect::rHRadius() const
{
    return this->d->m_rHRadius;
}

int FaceDetect::rVRadius() const
{
    return this->d->m_rVRadius;
}

void FaceDetect::deleteThis(void *userData) const
{
    delete reinterpret_cast<FaceDetect *>(userData);
}

QSize FaceDetect::pixelGridSize() const
{
    return this->d->m_pixelGridSize;
}

int FaceDetect::blurRadius() const
{
    return this->d->m_blurFilter->property<IAkPropertyInt>("radius")->value();
}

QSize FaceDetect::scanSize() const
{
    return this->d->m_scanSize;
}

QVector<QRect> FaceDetect::detectFaces(const AkVideoPacket &packet)
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

QString FaceDetect::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FaceDetect/share/qml/main.qml");
}

void FaceDetect::controlInterfaceConfigure(QQmlContext *context,
                                           const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FaceDetect", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket FaceDetect::iVideoStream(const AkVideoPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty()
        || scanSize.isEmpty()) {
        if (packet)
            this->oStream(packet);

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
            this->oStream(packet);

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
        this->oStream(dst);

    return dst;
}

void FaceDetect::setHaarFile(const QString &haarFile)
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

void FaceDetect::setMarkerType(MarkerType markerType)
{
    if (this->d->m_markerType == markerType)
        return;

    this->d->m_markerType = markerType;
    emit this->markerTypeChanged(markerType);
}

void FaceDetect::setMarkerColor(QRgb markerColor)
{
    if (this->d->m_markerPen.color() == QColor(markerColor))
        return;

    this->d->m_markerPen.setColor(QColor(markerColor));
    emit this->markerColorChanged(markerColor);
}

void FaceDetect::setMarkerWidth(int markerWidth)
{
    if (this->d->m_markerPen.width() == markerWidth)
        return;

    this->d->m_markerPen.setWidth(markerWidth);
    emit this->markerWidthChanged(markerWidth);
}

void FaceDetect::setMarkerStyle(const QString &markerStyle)
{
    Qt::PenStyle penStyle = markerStyleToStr->key(markerStyle, Qt::SolidLine);

    if (this->d->m_markerPen.style() == penStyle)
        return;

    this->d->m_markerPen.setStyle(penStyle);
    emit this->markerStyleChanged(markerStyle);
}

void FaceDetect::setMarkerImage(const QString &markerImage)
{
    if (this->d->m_markerImage == markerImage)
        return;

    this->d->m_markerImage = markerImage;

    if (!markerImage.isEmpty())
        this->d->m_markerImg = QImage(markerImage);

    emit this->markerImageChanged(markerImage);
}

void FaceDetect::setBackgroundImage(const QString &backgroundImage)
{
    if (this->d->m_backgroundImage == backgroundImage)
        return;

    this->d->m_backgroundImage = backgroundImage;

    if (!backgroundImage.isEmpty())
        this->d->m_backgroundImg = QImage(backgroundImage);

    emit this->backgroundImageChanged(backgroundImage);
}

void FaceDetect::setScale(qreal scale)
{
    if (this->d->m_scale == scale)
        return;

    this->d->m_scale = scale;
    emit this->scaleChanged(scale);
}

void FaceDetect::setRScale(qreal rScale)
{
    if (this->d->m_rScale == rScale)
        return;

    this->d->m_rScale = rScale;
    emit this->rScaleChanged(rScale);
}

void FaceDetect::setSmootheEdges(bool smootheEdges)
{
    if (this->d->m_smootheEdges == smootheEdges)
        return;

    this->d->m_smootheEdges = smootheEdges;
    emit this->smootheEdgesChanged(smootheEdges);
}

void FaceDetect::setHOffset(int hOffset)
{
    if (this->d->m_hOffset == hOffset)
        return;

    this->d->m_hOffset = hOffset;
    emit this->hOffsetChanged(hOffset);
}

void FaceDetect::setVOffset(int vOffset)
{
    if (this->d->m_vOffset == vOffset)
        return;

    this->d->m_vOffset = vOffset;
    emit this->vOffsetChanged(vOffset);
}

void FaceDetect::setWAdjust(int wAdjust)
{
    if (this->d->m_wAdjust == wAdjust)
        return;

    this->d->m_wAdjust = wAdjust;
    emit this->wAdjustChanged(wAdjust);
}

void FaceDetect::setHAdjust(int hAdjust)
{
    if (this->d->m_hAdjust == hAdjust)
        return;

    this->d->m_hAdjust = hAdjust;
    emit this->hAdjustChanged(hAdjust);
}

void FaceDetect::setRWAdjust(int rWAdjust)
{
    if (this->d->m_rWAdjust == rWAdjust)
        return;

    this->d->m_rWAdjust = rWAdjust;
    emit this->rWAdjustChanged(rWAdjust);
}

void FaceDetect::setRHAdjust(int rHAdjust)
{
    if (this->d->m_rHAdjust == rHAdjust)
        return;

    this->d->m_rHAdjust = rHAdjust;
    emit this->rHAdjustChanged(rHAdjust);
}

void FaceDetect::setRHRadius(int rHRadius)
{
    if (this->d->m_rHRadius == rHRadius)
        return;

    this->d->m_rHRadius = rHRadius;
    emit this->rHRadiusChanged(rHRadius);
}

void FaceDetect::setRVRadius(int rVRadius)
{
    if (this->d->m_rVRadius == rVRadius)
        return;

    this->d->m_rVRadius = rVRadius;
    emit this->rVRadiusChanged(rVRadius);
}

void FaceDetect::setPixelGridSize(const QSize &pixelGridSize)
{
    if (this->d->m_pixelGridSize == pixelGridSize)
        return;

    this->d->m_pixelGridSize = pixelGridSize;
    emit this->pixelGridSizeChanged(pixelGridSize);
}

void FaceDetect::setBlurRadius(int blurRadius)
{
    this->d->m_blurFilter->property<IAkPropertyInt>("radius")->setValue(blurRadius);
}

void FaceDetect::setScanSize(const QSize &scanSize)
{
    if (this->d->m_scanSize == scanSize)
        return;

    this->d->m_scanSize = scanSize;
    emit this->scanSizeChanged(scanSize);
}

void FaceDetect::resetHaarFile()
{
    this->setHaarFile(":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml");
}

void FaceDetect::resetMarkerType()
{
    this->setMarkerType(FaceDetect::MarkerTypeRectangle);
}

void FaceDetect::resetMarkerColor()
{
    this->setMarkerColor(qRgb(255, 0, 0));
}

void FaceDetect::resetMarkerWidth()
{
    this->setMarkerWidth(3);
}

void FaceDetect::resetMarkerStyle()
{
    this->setMarkerStyle("solid");
}

void FaceDetect::resetMarkerImage()
{
    this->setMarkerImage(":/FaceDetect/share/masks/cow.png");
}

void FaceDetect::resetBackgroundImage()
{
    this->setBackgroundImage(":/FaceDetect/share/backgrounds/black_square.png");
}

void FaceDetect::resetScale()
{
    this->setScale(1.0);
}

void FaceDetect::resetRScale()
{
    this->setRScale(1.0);
}

void FaceDetect::resetSmootheEdges()
{
    this->setSmootheEdges(false);
}

void FaceDetect::resetHOffset()
{
    this->setHOffset(0);
}

void FaceDetect::resetVOffset()
{
    this->setVOffset(0);
}

void FaceDetect::resetWAdjust()
{
    this->setWAdjust(100);
}

void FaceDetect::resetHAdjust()
{
    this->setHAdjust(100);
}

void FaceDetect::resetRWAdjust()
{
    this->setRWAdjust(100);
}

void FaceDetect::resetRHAdjust()
{
    this->setRHAdjust(100);
}

void FaceDetect::resetRHRadius()
{
    this->setRHRadius(0);
}

void FaceDetect::resetRVRadius()
{
    this->setRVRadius(0);
}

void FaceDetect::resetPixelGridSize()
{
    this->setPixelGridSize(QSize(32, 32));
}

void FaceDetect::resetBlurRadius()
{
    this->setBlurRadius(32);
}

void FaceDetect::resetScanSize()
{
    this->setScanSize(QSize(160, 120));
}

QDataStream &operator >>(QDataStream &istream, FaceDetect::MarkerType &markerType)
{
    int markerTypeInt;
    istream >> markerTypeInt;
    markerType = static_cast<FaceDetect::MarkerType>(markerTypeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, FaceDetect::MarkerType markerType)
{
    ostream << static_cast<int>(markerType);

    return ostream;
}

FaceDetectPrivate::FaceDetectPrivate(FaceDetect *self):
    self(self)
{

}

#include "moc_facedetect.cpp"
