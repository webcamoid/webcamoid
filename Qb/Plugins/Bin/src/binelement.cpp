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

#include "binelement.h"

BinElement::BinElement(): QbElement()
{
    this->resetDescription();
    this->resetBlocking();
}

BinElement::~BinElement()
{
}

QString BinElement::description() const
{
    return this->m_description;
}

bool BinElement::blocking() const
{
    return this->m_blocking;
}

QbElementPtr BinElement::element(QString elementName)
{
    return this->m_elements[elementName];
}

void BinElement::add(QbElementPtr element)
{
    this->m_pipelineDescription.addElement(element);
}

void BinElement::remove(QString elementName)
{
    this->m_pipelineDescription.removeElement(elementName);
}

void BinElement::setDescription(QString description)
{
    if (this->m_description == description)
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);

    if (this->m_description.isEmpty())
    {
        YY_BUFFER_STATE bufferState = yy_scan_string(description.toStdString().c_str());
        yyparse(&this->m_pipelineDescription);
        yy_delete_buffer(bufferState);

        QString error = this->m_pipelineDescription.error();

        if (error.isEmpty())
        {
            this->m_description = description;

            this->m_elements = this->m_pipelineDescription.elements();
            this->m_inputs = this->m_pipelineDescription.inputs();
            this->m_outputs = this->m_pipelineDescription.outputs();

            foreach (QbElementPtr element, this->m_outputs)
                QObject::connect(element.data(),
                                 SIGNAL(oStream(const QbPacket &)),
                                 this,
                                 SIGNAL(oStream(const QbPacket &)));
        }
        else
        {
            this->m_pipelineDescription.cleanAll();

            qDebug() << error;
        }
    }
    else if (description.isEmpty())
    {
        this->m_pipelineDescription.cleanAll();
        this->m_description = description;
    }
    else
    {
        foreach (QbElementPtr element, this->m_outputs)
            QObject::disconnect(element.data(),
                                SIGNAL(oStream(const QbPacket &)),
                                this,
                                SIGNAL(oStream(const QbPacket &)));

        this->m_pipelineDescription.cleanAll();

        YY_BUFFER_STATE bufferState = yy_scan_string(description.toStdString().c_str());
        yyparse(&this->m_pipelineDescription);
        yy_delete_buffer(bufferState);

        QString error = this->m_pipelineDescription.error();

        if (error.isEmpty())
        {
            this->m_description = description;

            this->m_elements = this->m_pipelineDescription.elements();
            this->m_inputs = this->m_pipelineDescription.inputs();
            this->m_outputs = this->m_pipelineDescription.outputs();

            foreach (QbElementPtr element, this->m_outputs)
                QObject::connect(element.data(),
                                 SIGNAL(oStream(const QbPacket &)),
                                 this,
                                 SIGNAL(oStream(const QbPacket &)));
        }
        else
        {
            this->m_pipelineDescription.cleanAll();
            this->m_description = "";

            qDebug() << error;
        }
    }

    this->setState(preState);
}

void BinElement::setBlocking(bool blocking)
{
    this->m_blocking = blocking;
}

void BinElement::resetDescription()
{
    this->setDescription("");
}

void BinElement::resetBlocking()
{
    this->setBlocking(false);
}

void BinElement::iStream(const QbPacket &packet)
{
    if (this->state() != ElementStatePlaying)
        return;

    if (this->description().isEmpty())
    {
        if (!this->blocking())
            emit this->oStream(packet);
    }
    else
        foreach (QbElementPtr element, this->m_inputs)
            element->iStream(packet);
}

void BinElement::setState(ElementState state)
{
    QbElement::setState(state);

    foreach (QbElementPtr element, this->m_elements)
        element->setState(this->state());
}
