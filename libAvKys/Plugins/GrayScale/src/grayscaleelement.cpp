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

#include <QImage>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "grayscaleelement.h"

class GrayScaleElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_graya8pack, 0, 0, {}}};
};

GrayScaleElement::GrayScaleElement(): AkElement()
{
    this->d = new GrayScaleElementPrivate;
}

GrayScaleElement::~GrayScaleElement()
{
    delete this->d;
}

AkPacket GrayScaleElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!dst)
        return {};

    emit this->oStream(dst);

    return dst;
}

#include "moc_grayscaleelement.cpp"
