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

#include "binelement.h"

BinElement::BinElement(): QbElement()
{
    this->resetDescription();
    this->resetRoutingMode();
}

BinElement::~BinElement()
{
}

QString BinElement::description() const
{/*
    QString str("IN. ! Element prop = color(0, 127, 255) signal > element.slot slot < element.signal ! A. ! OUT.");

    YY_BUFFER_STATE bufferState = yy_scan_string(str.toUtf8().constData());
    yyparse();
    yy_delete_buffer(bufferState);
*/
    return this->m_description;
}

BinElement::RoutingMode BinElement::routingMode() const
{
    return this->m_routingMode;
}

QbElementPtr BinElement::element(QString elementName)
{
}

bool BinElement::add(QbElementPtr element)
{
}

bool BinElement::remove(QString elementName)
{
}

void BinElement::setDescription(QString description)
{
    this->m_description = description;
}

void BinElement::setRoutingMode(RoutingMode pipelineRoutingMode)
{
    this->m_routingMode = pipelineRoutingMode;
}

void BinElement::resetDescription()
{
    this->setDescription("");
}

void BinElement::resetRoutingMode()
{
    this->setRoutingMode(RoutingModeNoCheck);
}

void BinElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet);
}
