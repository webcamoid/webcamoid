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

#include "vcapsconvertelement.h"

VCapsConvertElement::VCapsConvertElement(): QbElement()
{
    this->m_filter = Qb::create("Filter");

    QObject::connect(this,
                     SIGNAL(stateChanged(ElementState)),
                     this->m_filter.data(),
                     SLOT(setState(ElementState)));

    QObject::connect(this->m_filter.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SIGNAL(oStream(const QbPacket &)));

    this->resetCaps();
    this->resetKeepAspectRatio();
}

QString VCapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

bool VCapsConvertElement::keepAspectRatio() const
{
    return this->m_keepAspectRatio;
}

void VCapsConvertElement::setCaps(QString format)
{
    this->m_caps = QbCaps(format);
}

void VCapsConvertElement::setKeepAspectRatio(bool keepAspectRatio)
{
    this->m_keepAspectRatio = keepAspectRatio;
}

void VCapsConvertElement::resetCaps()
{
    this->setCaps("");
}

void VCapsConvertElement::resetKeepAspectRatio()
{
    this->setKeepAspectRatio(false);
}

void VCapsConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    QbCaps caps(packet.caps());
    caps.update(this->m_caps);

    if (caps != this->m_curInputCaps)
    {
        QSize frameSize(packet.caps().property("width").toInt(),
                        packet.caps().property("height").toInt());

        QSize newFrameSize(caps.property("width").toInt(),
                           caps.property("height").toInt());

        int padX;
        int padY;

        if (this->keepAspectRatio())
        {
            frameSize.scale(newFrameSize, Qt::KeepAspectRatio);
            padX = (newFrameSize.width() - frameSize.width()) >> 1;
            padY = (newFrameSize.height() - frameSize.height()) >> 1;
        }
        else
        {
            frameSize = newFrameSize;
            padX = 0;
            padY = 0;
        }

        this->m_filter->setProperty("format", caps.property("format"));

        this->m_filter->setProperty("description",
                                    QString("scale=%1:%2,"
                                            "pad=%3:%4:%5:%6:black,"
                                            "fps=fps=%7").arg(frameSize.width())
                                                         .arg(frameSize.height())
                                                         .arg(newFrameSize.width())
                                                         .arg(newFrameSize.height())
                                                         .arg(padX)
                                                         .arg(padY)
                                                         .arg(caps.property("fps").toString()));

        this->m_curInputCaps = caps;
    }

    this->m_filter->iStream(packet);
}
