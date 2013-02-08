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

#include "qbelement.h"

QbElement::QbElement(QObject *parent): QObject(parent)
{
    this->resetState();
    this->resetSrcs();
    this->resetSinks();
}

QbElement::~QbElement()
{
    this->resetState();
    this->resetSrcs();
    this->resetSinks();
}

QbElement::ElementState QbElement::state()
{
    return this->m_state;
}

QList<QbCaps> QbElement::oCaps()
{
    return this->m_oCaps;
}

QList<QbElement *> QbElement::srcs()
{
    return this->m_srcs;
}

QList<QbElement *> QbElement::sinks()
{
    return this->m_sinks;
}
bool QbElement::link(QObject *dstElement)
{
    if (!dstElement)
        return false;

    if (!QObject::connect(this,
                          SIGNAL(oStream(const QbPacket &)),
                          dstElement,
                          SLOT(iStream(const QbPacket &))))
        return false;

    return true;
}

bool QbElement::link(QbElement *dstElement)
{
    if (!this->link(static_cast<QObject *>(dstElement)))
        return false;

    this->setSinks(this->sinks() << dstElement);
    dstElement->setSrcs(dstElement->srcs() << this);

    return true;
}

bool QbElement::unlink(QObject *dstElement)
{
    if (!dstElement)
        return false;

    if (!QObject::disconnect(this,
                             SIGNAL(oStream(const QbPacket &)),
                             dstElement,
                             SLOT(iStream(const QbPacket &))))
        return false;

    return true;
}

bool QbElement::unlink(QbElement *dstElement)
{
    if (!this->unlink(static_cast<QObject *>(dstElement)))
        return false;

    QList<QbElement *> sinks = this->m_sinks;
    sinks.removeOne(dstElement);
    this->setSinks(sinks);

    QList<QbElement *> srcs = dstElement->m_srcs;
    srcs.removeOne(this);
    dstElement->setSrcs(srcs);

    return true;
}

void QbElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)
}

void QbElement::setState(ElementState state)
{
    this->m_state = state;
}

void QbElement::setSrcs(QList<QbElement *> srcs)
{
    this->m_srcs = srcs;
}

void QbElement::setSinks(QList<QbElement *> sinks)
{
    this->m_sinks = sinks;
}

void QbElement::resetState()
{
    this->setState(ElementStateNull);
}

void QbElement::resetSrcs()
{
    this->setSrcs(QList<QbElement *>());
}

void QbElement::resetSinks()
{
    this->setSinks(QList<QbElement *>());
}
