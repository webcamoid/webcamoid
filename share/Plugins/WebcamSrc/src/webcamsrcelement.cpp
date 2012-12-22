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

#include <gst/app/gstappsink.h>

#include "webcamsrcelement.h"

WebcamSrcElement::WebcamSrcElement(): Element()
{
}

WebcamSrcElement::~WebcamSrcElement()
{
}

QString WebcamSrcElement::device()
{
}

QSize WebcamSrcElement::size()
{
}

Element::ElementState WebcamSrcElement::state()
{
}

void WebcamSrcElement::newBuffer(GstElement *appsink, gpointer self)
{
}

void WebcamSrcElement::setDevice(QString device)
{
}

void WebcamSrcElement::setSize(QSize size)
{
}

void WebcamSrcElement::resetDevice()
{
}

void WebcamSrcElement::resetSize()
{
}

void WebcamSrcElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(data)
    Q_UNUSED(datalen)
    Q_UNUSED(dataType)
}

void WebcamSrcElement::setState(ElementState state)
{
    Q_UNUSED(state)
}

void WebcamSrcElement::resetState()
{
}
