/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "binelement.h"

BinElement::BinElement():
    AkElement()
{
    this->m_pipelineDescription.setParent(this);
    this->m_blocking = false;
}

QString BinElement::description() const
{
    return this->m_description;
}

bool BinElement::blocking() const
{
    return this->m_blocking;
}

AkElementPtr BinElement::element(const QString &elementName)
{
    return this->m_elements[elementName];
}

void BinElement::add(AkElementPtr element)
{
    this->m_pipelineDescription.addElement(element);
}

void BinElement::remove(const QString &elementName)
{
    this->m_pipelineDescription.removeElement(elementName);
}

void BinElement::setDescription(const QString &description)
{
    if (this->m_description == description)
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);

    if (this->m_description.isEmpty()) {
        this->m_pipelineDescription.parse(description);
        QString error = this->m_pipelineDescription.error();

        if (error.isEmpty()) {
            this->m_description = description;

            this->m_elements = this->m_pipelineDescription.elements();
            this->m_inputs = this->m_pipelineDescription.inputs();
            this->m_outputs = this->m_pipelineDescription.outputs();
            this->connectOutputs();
        } else {
            this->m_pipelineDescription.cleanAll();

            qDebug() << error;
        }
    } else if (description.isEmpty()) {
        this->m_pipelineDescription.cleanAll();
        this->m_description = description;
    } else {
        for (const AkElementPtr &element: this->m_outputs)
            QObject::disconnect(element.data(),
                                &AkElement::oStream,
                                this,
                                &BinElement::oStream);

        this->m_pipelineDescription.cleanAll();

        this->m_pipelineDescription.parse(description);
        QString error = this->m_pipelineDescription.error();

        if (error.isEmpty()) {
            this->m_description = description;

            this->m_elements = this->m_pipelineDescription.elements();
            this->m_inputs = this->m_pipelineDescription.inputs();
            this->m_outputs = this->m_pipelineDescription.outputs();
            this->connectOutputs();
        } else {
            this->m_pipelineDescription.cleanAll();
            this->m_description = "";

            qDebug() << error;
        }
    }

    this->setState(preState);
    emit this->descriptionChanged(description);
}

void BinElement::setBlocking(bool blocking)
{
    if (this->m_blocking == blocking)
        return;

    this->m_blocking = blocking;
    emit this->blockingChanged(blocking);
}

void BinElement::resetDescription()
{
    this->setDescription("");
}

void BinElement::resetBlocking()
{
    this->setBlocking(false);
}

AkPacket BinElement::iStream(const AkPacket &packet)
{
    if (!this->m_description.isEmpty())
        for (const AkElementPtr &element: this->m_inputs)
            element->iStream(packet);
    else if (!this->m_blocking)
        akSend(packet)

    return AkPacket();
}

bool BinElement::setState(AkElement::ElementState state)
{
    AkElement::setState(state);
    bool ok = true;

    for (const AkElementPtr &element: this->m_elements) {
        bool ret = false;
        QMetaObject::invokeMethod(element.data(),
                                  "setState",
                                  Q_RETURN_ARG(bool, ret),
                                  Q_ARG(AkElement::ElementState,
                                        this->state()));
        ok &= ret;
    }

    return ok;
}

void BinElement::connectOutputs()
{
    QList<Qt::ConnectionType> connectionTypes = this->m_pipelineDescription.outputConnectionTypes();
    int i = 0;

    for (const AkElementPtr &element: this->m_outputs) {
        QObject::connect(element.data(),
                         &AkElement::oStream,
                         this,
                         &BinElement::oStream,
                         connectionTypes[i]);

        i++;
    }
}

void BinElement::disconnectOutputs()
{
    for (const AkElementPtr &element: this->m_outputs)
        QObject::disconnect(element.data(),
                            &AkElement::oStream,
                            this,
                            &BinElement::oStream);
}
