/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QStandardPaths>
#include <QDir>

#include "facedetectelement.h"

typedef QMap<FaceDetectElement::MarkerType, QString> MarkerTypeMap;

inline MarkerTypeMap initMarkerTypeMap()
{
    MarkerTypeMap markerTypeToStr;
    markerTypeToStr[FaceDetectElement::MarkerTypeRectangle] = "rectangle";
    markerTypeToStr[FaceDetectElement::MarkerTypeEllipse] = "ellipse";
    markerTypeToStr[FaceDetectElement::MarkerTypeImage] = "image";
    markerTypeToStr[FaceDetectElement::MarkerTypePixelate] = "pixelate";
    markerTypeToStr[FaceDetectElement::MarkerTypeBlur] = "blur";

    return markerTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(MarkerTypeMap, markerTypeToStr, (initMarkerTypeMap()))

typedef QMap<Qt::PenStyle, QString> PenStyleMap;

inline PenStyleMap initPenStyleMap()
{
    PenStyleMap markerStyleToStr;
    markerStyleToStr[Qt::SolidLine] = "solid";
    markerStyleToStr[Qt::DashLine] = "dash";
    markerStyleToStr[Qt::DotLine] = "dot";
    markerStyleToStr[Qt::DashDotLine] = "dashDot";
    markerStyleToStr[Qt::DashDotDotLine] = "dashDotDot";

    return markerStyleToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(PenStyleMap, markerStyleToStr, (initPenStyleMap()))

FaceDetectElement::FaceDetectElement(): QbElement()
{
    this->m_haarFile = ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml";
    this->m_cascadeClassifier.loadCascade(this->m_haarFile);
    this->m_markerType = MarkerTypeRectangle;
    this->m_markerPen.setColor(QColor(255, 0, 0));
    this->m_markerPen.setWidth(3);
    this->m_markerPen.setStyle(Qt::SolidLine);
    this->m_markerImage = ":/FaceDetect/share/masks/cow.png";
    this->m_markerImg = QImage(this->m_markerImage);
    this->m_pixelGridSize = QSize(32, 32);
    this->m_blurRadius = 32;
    this->m_scanSize = QSize(160, 120);
}

QObject *FaceDetectElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/FaceDetect/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("FaceDetect", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QString FaceDetectElement::haarFile() const
{
    return this->m_haarFile;
}

QString FaceDetectElement::markerType() const
{
    return markerTypeToStr->value(this->m_markerType);
}

QRgb FaceDetectElement::markerColor() const
{
    return qRgba(this->m_markerPen.color().blue(),
                 this->m_markerPen.color().green(),
                 this->m_markerPen.color().red(),
                 this->m_markerPen.color().alpha());
}

int FaceDetectElement::markerWidth() const
{
    return this->m_markerPen.width();
}

QString FaceDetectElement::markerStyle() const
{
    return markerStyleToStr->value(this->m_markerPen.style());
}

QString FaceDetectElement::markerImage() const
{
    return this->m_markerImage;
}

QSize FaceDetectElement::pixelGridSize() const
{
    return this->m_pixelGridSize;
}

int FaceDetectElement::blurRadius() const
{
    return this->m_blurRadius;
}

QSize FaceDetectElement::scanSize() const
{
    return this->m_scanSize;
}

void FaceDetectElement::setHaarFile(const QString &haarFile)
{
    if (this->m_haarFile == haarFile)
        return;

    if (this->m_cascadeClassifier.loadCascade(haarFile)) {
        this->m_haarFile = haarFile;
        emit this->haarFileChanged(haarFile);
    } else if (this->m_haarFile != "") {
        this->m_haarFile = "";
        emit this->haarFileChanged(this->m_haarFile);
    }
}

void FaceDetectElement::setMarkerType(const QString &markerType)
{
    MarkerType markerTypeEnum = markerTypeToStr->key(markerType, MarkerTypeRectangle);

    if (this->m_markerType == markerTypeEnum)
        return;

    this->m_markerType = markerTypeEnum;
    emit this->markerTypeChanged(markerType);
}

void FaceDetectElement::setMarkerColor(QRgb markerColor)
{
    QColor color(qBlue(markerColor),
                 qGreen(markerColor),
                 qRed(markerColor));

    if (this->m_markerPen.color() == color)
        return;

    this->m_markerPen.setColor(color);
    emit this->markerColorChanged(markerColor);
}

void FaceDetectElement::setMarkerWidth(int markerWidth)
{
    if (this->m_markerPen.width() == markerWidth)
        return;

    this->m_markerPen.setWidth(markerWidth);
    emit this->markerWidthChanged(markerWidth);
}

void FaceDetectElement::setMarkerStyle(const QString &markerStyle)
{
    Qt::PenStyle penStyle = markerStyleToStr->key(markerStyle, Qt::SolidLine);

    if (this->m_markerPen.style() == penStyle)
        return;

    this->m_markerPen.setStyle(penStyle);
    emit this->markerStyleChanged(markerStyle);
}

void FaceDetectElement::setMarkerImage(const QString &markerImage)
{
    if (this->m_markerImage == markerImage)
        return;

    this->m_markerImage = markerImage;

    if (!markerImage.isEmpty())
        this->m_markerImg = QImage(markerImage);

    emit this->markerImageChanged(markerImage);
}

void FaceDetectElement::setPixelGridSize(const QSize &pixelGridSize)
{
    if (this->m_pixelGridSize == pixelGridSize)
        return;

    this->m_pixelGridSize = pixelGridSize;
    emit this->pixelGridSizeChanged(pixelGridSize);
}

void FaceDetectElement::setBlurRadius(int blurRadius)
{
    if (this->m_blurRadius == blurRadius)
        return;

    this->m_blurRadius = blurRadius;
    emit this->blurRadiusChanged(blurRadius);
}

void FaceDetectElement::setScanSize(const QSize &scanSize)
{
    if (this->m_scanSize == scanSize)
        return;

    this->m_scanSize = scanSize;
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

QbPacket FaceDetectElement::iStream(const QbPacket &packet)
{
    QSize scanSize(this->m_scanSize);

    if (this->m_haarFile.isEmpty()
        || scanSize.isEmpty())
        qbSend(packet)

    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    qreal scale = 1;

    QImage scanFrame(src.scaled(scanSize, Qt::KeepAspectRatio));

    if (scanFrame.width() == scanSize.width())
        scale = (qreal) src.width() / scanSize.width();
    else
        scale = (qreal) src.height() / scanSize.height();

    this->m_cascadeClassifier.setEqualize(true);
    QVector<QRect> vecFaces = this->m_cascadeClassifier.detect(scanFrame);

    if (vecFaces.isEmpty())
        qbSend(packet)

    QPainter painter;
    painter.begin(&oFrame);

    foreach (QRect face, vecFaces) {
        QRect rect(scale * face.x(),
                   scale * face.y(),
                   scale * face.width(),
                   scale * face.height());

        if (this->m_markerType == MarkerTypeRectangle) {
            painter.setPen(this->m_markerPen);
            painter.drawRect(rect);
        } else if (this->m_markerType == MarkerTypeEllipse) {
            painter.setPen(this->m_markerPen);
            painter.drawEllipse(rect);
        } else if (this->m_markerType == MarkerTypeImage)
            painter.drawImage(rect, this->m_markerImg);
        else if (this->m_markerType == MarkerTypePixelate) {
            qreal sw = 1.0 / this->m_pixelGridSize.width();
            qreal sh = 1.0 / this->m_pixelGridSize.height();
            QImage imagePixelate = src.copy(rect);

            imagePixelate = imagePixelate.scaled(sw * imagePixelate.width(),
                                                 sh * imagePixelate.height(),
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::FastTransformation)
                                         .scaled(imagePixelate.width(),
                                                 imagePixelate.height(),
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::FastTransformation);

            painter.drawImage(rect, imagePixelate);
        } else if (this->m_markerType == MarkerTypeBlur) {
            QImage img = src.copy(rect);
            QGraphicsScene scene;
            QGraphicsPixmapItem *pixmapItem = scene.addPixmap(QPixmap::fromImage(img));
            QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
            pixmapItem->setGraphicsEffect(effect);
            effect->setBlurRadius(this->m_blurRadius);

            QImage blurImage(img.size(), img.format());

            QPainter blurPainter;
            blurPainter.begin(&blurImage);
            scene.render(&blurPainter);
            blurPainter.end();

            painter.drawImage(rect, blurImage);
        }
    }

    painter.end();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
