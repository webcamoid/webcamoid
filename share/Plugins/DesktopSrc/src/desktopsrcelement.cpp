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

#include "desktopsrcelement.h"

DesktopSrcElement::DesktopSrcElement(): QbElement()
{
    this->resetState();

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(captureFrame()));
}

DesktopSrcElement::~DesktopSrcElement()
{
    this->setState(ElementStateNull);
}

QbElement::ElementState DesktopSrcElement::state()
{
    return this->m_state;
}

QList<QbElement *> DesktopSrcElement::srcs()
{
    return this->m_srcs;
}

QList<QbElement *> DesktopSrcElement::sinks()
{
    return this->m_sinks;
}

void DesktopSrcElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)
}

void DesktopSrcElement::setState(ElementState state)
{
    switch (state)
    {
        case ElementStatePlaying:
            this->m_timer.start();
        break;

        default:
            this->m_timer.stop();
        break;
    }

    this->m_state = state;
}

void DesktopSrcElement::setSrcs(QList<QbElement *> srcs)
{
    this->m_srcs = srcs;
}

void DesktopSrcElement::setSinks(QList<QbElement *> sinks)
{
    this->m_sinks = sinks;
}

void DesktopSrcElement::resetState()
{
    this->setState(ElementStateNull);
}

void DesktopSrcElement::resetSrcs()
{
    this->setSrcs(QList<QbElement *>());
}

void DesktopSrcElement::resetSinks()
{
    this->setSinks(QList<QbElement *>());
}

void DesktopSrcElement::captureFrame()
{
    this->m_oFrame = QPixmap::grabWindow(QApplication::desktop()->winId()).toImage();

    QbPacket packet(QString("video/x-raw,format=RGB,width=%1,height=%2").arg(this->m_oFrame.width())
                                                                        .arg(this->m_oFrame.height()),
                    this->m_oFrame.constBits(),
                    this->m_oFrame.byteCount());

    emit this->oStream(packet);
}
