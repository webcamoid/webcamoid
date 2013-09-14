/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "qimageconvertelement.h"

QImageConvertElement::QImageConvertElement(): QbElement()
{
    this->m_imageToFormat["Mono"] = "monob";
    this->m_imageToFormat["Indexed8"] = "rgb8";
    this->m_imageToFormat["RGB32"] = "bgr0";
    this->m_imageToFormat["ARGB32"] = "bgra";
    this->m_imageToFormat["RGB16"] = "rgb565le";
    this->m_imageToFormat["RGB555"] = "rgb555le";
    this->m_imageToFormat["RGB888"] = "rgb24";
    this->m_imageToFormat["RGB444"] = "rgb444le";

    this->m_imageToQt["Mono"] = QImage::Format_Mono;
    this->m_imageToQt["Indexed8"] = QImage::Format_Indexed8;
    this->m_imageToQt["RGB32"] = QImage::Format_RGB32;
    this->m_imageToQt["ARGB32"] = QImage::Format_ARGB32;
    this->m_imageToQt["RGB16"] = QImage::Format_RGB16;
    this->m_imageToQt["RGB555"] = QImage::Format_RGB555;
    this->m_imageToQt["RGB888"] = QImage::Format_RGB888;
    this->m_imageToQt["RGB444"] = QImage::Format_RGB444;

    this->m_capsConvert = Qb::create("VCapsConvert");

    QObject::connect(this->m_capsConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetFormat();
}

QString QImageConvertElement::format()
{
    return this->m_format;
}

void QImageConvertElement::setFormat(QString format)
{
    this->m_format = format;

    if (this->m_imageToQt.contains(format))
        this->m_qFormat = this->m_imageToQt[format];
    else
    {
        this->m_format = "";
        this->m_qFormat = QImage::Format_Invalid;
    }
}

void QImageConvertElement::resetFormat()
{
    this->setFormat("ARGB32");
}

void QImageConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying ||
        !this->m_capsConvert)
        return;

    if (this->m_capsConvert->property("caps").toString() == "")
        this->m_capsConvert->setProperty("caps",
                                         QString("video/x-raw,format=%1").arg(this->m_imageToFormat[this->m_format]));

    this->m_capsConvert->iStream(packet);
}

void QImageConvertElement::setState(ElementState state)
{
    QbElement::setState(state);
    this->m_capsConvert->setState(this->state());
}

void QImageConvertElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage oFrame((const uchar *) packet.buffer().data(),
                  width,
                  height,
                  this->m_qFormat);

    emit this->oStream(oFrame);
}
