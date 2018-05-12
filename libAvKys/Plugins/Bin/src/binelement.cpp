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

#include <QDebug>
#include <QSharedPointer>
#include <akpacket.h>

#include "binelement.h"
#include "pipeline.h"

class BinElementPrivate
{
    public:
        QString m_description;
        QMap<QString, AkElementPtr> m_elements;
        QList<AkElementPtr> m_inputs;
        QList<AkElementPtr> m_outputs;
        Pipeline m_pipelineDescription;
        bool m_blocking;

        BinElementPrivate():
            m_blocking(false)
        {
        }
};

BinElement::BinElement():
    AkElement()
{
    this->d = new BinElementPrivate;
    this->d->m_pipelineDescription.setParent(this);
}

BinElement::~BinElement()
{
    delete this->d;
}

QString BinElement::description() const
{
    return this->d->m_description;
}

bool BinElement::blocking() const
{
    return this->d->m_blocking;
}

AkElementPtr BinElement::element(const QString &elementName)
{
    return this->d->m_elements[elementName];
}

void BinElement::add(AkElementPtr element)
{
    this->d->m_pipelineDescription.addElement(element);
}

void BinElement::remove(const QString &elementName)
{
    this->d->m_pipelineDescription.removeElement(elementName);
}

void BinElement::setDescription(const QString &description)
{
    if (this->d->m_description == description)
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);

    if (this->d->m_description.isEmpty()) {
        this->d->m_pipelineDescription.parse(description);
        QString error = this->d->m_pipelineDescription.error();

        if (error.isEmpty()) {
            this->d->m_description = description;

            this->d->m_elements = this->d->m_pipelineDescription.elements();
            this->d->m_inputs = this->d->m_pipelineDescription.inputs();
            this->d->m_outputs = this->d->m_pipelineDescription.outputs();
            this->connectOutputs();
        } else {
            this->d->m_pipelineDescription.cleanAll();

            qDebug() << error;
        }
    } else if (description.isEmpty()) {
        this->d->m_pipelineDescription.cleanAll();
        this->d->m_description = description;
    } else {
        for (const AkElementPtr &element: this->d->m_outputs)
            QObject::disconnect(element.data(),
                                &AkElement::oStream,
                                this,
                                &BinElement::oStream);

        this->d->m_pipelineDescription.cleanAll();
        this->d->m_pipelineDescription.parse(description);
        QString error = this->d->m_pipelineDescription.error();

        if (error.isEmpty()) {
            this->d->m_description = description;
            this->d->m_elements = this->d->m_pipelineDescription.elements();
            this->d->m_inputs = this->d->m_pipelineDescription.inputs();
            this->d->m_outputs = this->d->m_pipelineDescription.outputs();
            this->connectOutputs();
        } else {
            this->d->m_pipelineDescription.cleanAll();
            this->d->m_description = "";

            qDebug() << error;
        }
    }

    this->setState(preState);
    emit this->descriptionChanged(description);
}

void BinElement::setBlocking(bool blocking)
{
    if (this->d->m_blocking == blocking)
        return;

    this->d->m_blocking = blocking;
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
    if (!this->d->m_description.isEmpty())
        for (const AkElementPtr &element: this->d->m_inputs)
            element->iStream(packet);
    else if (!this->d->m_blocking)
        akSend(packet)

    return AkPacket();
}

bool BinElement::setState(AkElement::ElementState state)
{
    AkElement::setState(state);
    bool ok = true;

    for (const AkElementPtr &element: this->d->m_elements) {
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
    auto connectionTypes =
            this->d->m_pipelineDescription.outputConnectionTypes();
    int i = 0;

    for (const AkElementPtr &element: this->d->m_outputs) {
        QObject::connect(element.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         connectionTypes[i]);

        i++;
    }
}

void BinElement::disconnectOutputs()
{
    for (const AkElementPtr &element: this->d->m_outputs)
        QObject::disconnect(element.data(),
                            &AkElement::oStream,
                            this,
                            &BinElement::oStream);
}

#include "moc_binelement.cpp"
