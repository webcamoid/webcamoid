/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "facedetectelement.h"

FaceDetectElement::FaceDetectElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr24");

    qRegisterMetaType<QRgb>("QRgb");

    this->m_markerTypeToStr[MarkerTypeRectangle] = "rectangle";
    this->m_markerTypeToStr[MarkerTypeEllipse] = "ellipse";
    this->m_markerTypeToStr[MarkerTypeImage] = "image";
    this->m_markerTypeToStr[MarkerTypePixelate] = "pixelate";
    this->m_markerTypeToStr[MarkerTypeBlur] = "blur";

    this->m_markerStyleToStr[Qt::SolidLine] = "solid";
    this->m_markerStyleToStr[Qt::DashLine] = "dash";
    this->m_markerStyleToStr[Qt::DotLine] = "dot";
    this->m_markerStyleToStr[Qt::DashDotLine] = "dashDot";
    this->m_markerStyleToStr[Qt::DashDotDotLine] = "dashDotDot";

    this->resetHaarFile();
    this->resetMarkerType();
    this->resetMarkerColor();
    this->resetMarkerWidth();
    this->resetMarkerStyle();
    this->resetMarkerImage();
    this->resetPixelGridSize();
    this->resetBlurRadius();
    this->resetScanSize();
}

QString FaceDetectElement::haarFile() const
{
    return this->m_haarFile;
}

QString FaceDetectElement::markerType() const
{
    return this->m_markerTypeToStr[this->m_markerType];
}

QRgb FaceDetectElement::markerColor() const
{
    return this->m_markerPen.color().rgb();
}

int FaceDetectElement::markerWidth() const
{
    return this->m_markerPen.width();
}

QString FaceDetectElement::markerStyle() const
{
    return this->m_markerStyleToStr[this->m_markerPen.style()];
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
    this->m_haarFile = haarFile;
    this->m_cascadeClassifier.load(haarFile.toStdString());
}

void FaceDetectElement::setMarkerType(const QString &markerType)
{
    if (this->m_markerTypeToStr.values().contains(markerType))
        this->m_markerType = this->m_markerTypeToStr.key(markerType);
    else
        this->m_markerType = MarkerTypeRectangle;
}

void FaceDetectElement::setMarkerColor(QRgb markerColor)
{
    this->m_markerPen.setColor(QColor(qBlue(markerColor),
                                      qGreen(markerColor),
                                      qRed(markerColor)));
}

void FaceDetectElement::setMarkerWidth(int markerWidth)
{
    this->m_markerPen.setWidth(markerWidth);
}

void FaceDetectElement::setMarkerStyle(const QString &markerStyle)
{
    if (this->m_markerStyleToStr.values().contains(markerStyle))
        this->m_markerPen.setStyle(this->m_markerStyleToStr.key(markerStyle));
    else
        this->m_markerPen.setStyle(Qt::SolidLine);
}

void FaceDetectElement::setMarkerImage(const QString &markerImage)
{
    this->m_markerImage = markerImage;

    if (!markerImage.isEmpty())
        this->m_markerImg = QImage(markerImage).rgbSwapped();
}

void FaceDetectElement::setPixelGridSize(const QSize &pixelGridSize)
{
    this->m_pixelGridSize = pixelGridSize;
}

void FaceDetectElement::setBlurRadius(int blurRadius)
{
    this->m_blurRadius = blurRadius;
}

void FaceDetectElement::setScanSize(const QSize &scanSize)
{
    this->m_scanSize = scanSize;
}

void FaceDetectElement::resetHaarFile()
{
    this->setHaarFile(QString("%1/%2")
                      .arg(OPENCVHAARPATH)
                      .arg("haarcascade_frontalface_alt.xml"));
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
    this->setMarkerImage(":/FaceDetect/share/cow.png");
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
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = src;

    float scale = 1;

    QImage scanFrame(src.scaled(this->m_scanSize, Qt::KeepAspectRatio));

    if (scanFrame.width() == this->m_scanSize.width())
        scale = (float) src.width() / this->m_scanSize.width();
    else
        scale = (float) src.height() / this->m_scanSize.height();

    cv::Mat matFrame(scanFrame.height(),
                     scanFrame.width(),
                     CV_8UC3,
                     (uchar *) scanFrame.bits(),
                     scanFrame.bytesPerLine());

    std::vector<cv::Rect> vecFaces;

    cv::cvtColor(matFrame, matFrame, CV_BGR2GRAY);
    cv::equalizeHist(matFrame, matFrame);
    this->m_cascadeClassifier.detectMultiScale(matFrame, vecFaces);

    if (vecFaces.size() < 1)
        qbSend(packet)

    QPainter painter;
    painter.begin(&oFrame);

    for (std::vector<cv::Rect>::const_iterator face = vecFaces.begin();
         face != vecFaces.end();
         face++) {
        QRect rect(face->x * scale, face->y * scale,
                   face->width * scale, face->height * scale);

        if (this->m_markerType == MarkerTypeRectangle) {
            painter.setPen(this->m_markerPen);
            painter.drawRect(rect);
        }
        else if (this->m_markerType == MarkerTypeEllipse) {
            painter.setPen(this->m_markerPen);
            painter.drawEllipse(rect);
        }
        else if (this->m_markerType == MarkerTypeImage)
            painter.drawImage(rect, this->m_markerImg);
        else if (this->m_markerType == MarkerTypePixelate) {
            float sw = 1.0 / this->m_pixelGridSize.width();
            float sh = 1.0 / this->m_pixelGridSize.height();
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
        }
        else if (this->m_markerType == MarkerTypeBlur) {
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

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
