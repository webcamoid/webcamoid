/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <akutils.h>
#include <akpacket.h>

#include "facedetectelement.h"
#include "haar/haardetector.h"

typedef QMap<FaceDetectElement::MarkerType, QString> MarkerTypeMap;

inline MarkerTypeMap initMarkerTypeMap()
{
    MarkerTypeMap markerTypeToStr {
        {FaceDetectElement::MarkerTypeRectangle, "rectangle"},
        {FaceDetectElement::MarkerTypeEllipse  , "ellipse"  },
        {FaceDetectElement::MarkerTypeImage    , "image"    },
        {FaceDetectElement::MarkerTypePixelate , "pixelate" },
        {FaceDetectElement::MarkerTypeBlur     , "blur"     }
    };

    return markerTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(MarkerTypeMap, markerTypeToStr, (initMarkerTypeMap()))

typedef QMap<Qt::PenStyle, QString> PenStyleMap;

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
        QString m_haarFile;
        FaceDetectElement::MarkerType m_markerType;
        QPen m_markerPen;
        QString m_markerImage;
        QImage m_markerImg;
        QSize m_pixelGridSize;
        QSize m_scanSize;
        AkElementPtr m_blurFilter;
        HaarDetector m_cascadeClassifier;
};

FaceDetectElement::FaceDetectElement(): AkElement()
{
    this->d = new FaceDetectElementPrivate;
    this->d->m_haarFile = ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml";
    this->d->m_cascadeClassifier.loadCascade(this->d->m_haarFile);
    this->d->m_markerType = MarkerTypeRectangle;
    this->d->m_markerPen.setColor(QColor(255, 0, 0));
    this->d->m_markerPen.setWidth(3);
    this->d->m_markerPen.setStyle(Qt::SolidLine);
    this->d->m_markerImage = ":/FaceDetect/share/masks/cow.png";
    this->d->m_markerImg = QImage(this->d->m_markerImage);
    this->d->m_pixelGridSize = QSize(32, 32);
    this->d->m_scanSize = QSize(160, 120);
    this->d->m_blurFilter = AkElement::create("Blur");
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

QString FaceDetectElement::markerType() const
{
    return markerTypeToStr->value(this->d->m_markerType);
}

QRgb FaceDetectElement::markerColor() const
{
    return qRgba(this->d->m_markerPen.color().blue(),
                 this->d->m_markerPen.color().green(),
                 this->d->m_markerPen.color().red(),
                 this->d->m_markerPen.color().alpha());
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

void FaceDetectElement::setMarkerType(const QString &markerType)
{
    MarkerType markerTypeEnum = markerTypeToStr->key(markerType, MarkerTypeRectangle);

    if (this->d->m_markerType == markerTypeEnum)
        return;

    this->d->m_markerType = markerTypeEnum;
    emit this->markerTypeChanged(markerType);
}

void FaceDetectElement::setMarkerColor(QRgb markerColor)
{
    QColor color(qBlue(markerColor),
                 qGreen(markerColor),
                 qRed(markerColor));

    if (this->d->m_markerPen.color() == color)
        return;

    this->d->m_markerPen.setColor(color);
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
    this->setMarkerType("rectangle");
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

AkPacket FaceDetectElement::iStream(const AkPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty()
        || scanSize.isEmpty())
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    qreal scale = 1;

    QImage scanFrame(src.scaled(scanSize, Qt::KeepAspectRatio));

    if (scanFrame.width() == scanSize.width())
        scale = qreal(src.width() / scanSize.width());
    else
        scale = qreal(src.height() / scanSize.height());

    this->d->m_cascadeClassifier.setEqualize(true);
    QVector<QRect> vecFaces = this->d->m_cascadeClassifier.detect(scanFrame);

    if (vecFaces.isEmpty())
        akSend(packet)

    QPainter painter;
    painter.begin(&oFrame);

    for (const QRect &face: vecFaces) {
        QRect rect(int(scale * face.x()),
                   int(scale * face.y()),
                   int(scale * face.width()),
                   int(scale * face.height()));

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
            QImage imagePixelate = src.copy(rect);

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
            AkPacket rectPacket = AkUtils::imageToPacket(src.copy(rect), packet);
            AkPacket blurPacket = this->d->m_blurFilter->iStream(rectPacket);
            QImage blurImage = AkUtils::packetToImage(blurPacket);

            painter.drawImage(rect, blurImage);
        }
    }

    painter.end();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_facedetectelement.cpp"
