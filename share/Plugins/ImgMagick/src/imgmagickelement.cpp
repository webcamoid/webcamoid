/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#include "imgmagickelement.h"

ImgMagickElement::ImgMagickElement(): QbElement()
{
    this->m_capsConvert = Qb::create("VCapsConvert");
    this->m_capsConvert->setProperty("caps", "video/x-raw,format=rgba");

    QObject::connect(this->m_capsConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetMethod();
    this->resetParams();
}

ImgMagickElement::~ImgMagickElement()
{
}

QString ImgMagickElement::method() const
{
    return this->m_method;
}

QVariantList ImgMagickElement::params() const
{
    return this->m_params;
}

void ImgMagickElement::setMethod(QString method)
{
    this->m_method = method;
}

void ImgMagickElement::setParams(QVariantList params)
{
    this->m_params = params;
}

void ImgMagickElement::resetMethod()
{
    this->setMethod("");
}

void ImgMagickElement::resetParams()
{
    this->setParams(QVariantList());
}

void ImgMagickElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    if (packet.caps().property("format").toString() == "rgb24")
        this->processFrame(packet);
    else
        this->m_capsConvert->iStream(packet);
}

void ImgMagickElement::setState(ElementState state)
{
    QbElement::setState(state);
    this->m_capsConvert->setState(this->state());
}

void ImgMagickElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();
    QString format = packet.caps().property("format").toString();
    QString map = (format== "rgb24")? "RGB": "RGBA";

    Magick::Image magickImage(width,
                              height,
                              map.toUtf8().constData(),
                              Magick::CharPixel,
                              packet.data());

    magickImage.write(0,
                      0,
                      magickImage.columns(),
                      magickImage.rows(),
                      map,
                      Magick::CharPixel,
                      pixels);
}
