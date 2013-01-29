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

#include "qbpipeline.h"

QbPipeline::QbPipeline(QObject *parent): QObject(parent)
{
    this->m_pluginLoader = QbPluginLoader::current();
}

QbPipeline::~QbPipeline()
{
    QList<QbElement *> elements = this->m_elements.keys();

    foreach (QbElement *element, elements)
        this->remove(element);
}

QStringList QbPipeline::pluginsPaths()
{
    return this->m_pluginLoader->pluginsPaths();
}

QbElement *QbPipeline::element(QString elementName)
{
    foreach (QbElement *element, this->m_elements.keys())
        if (element->objectName() == elementName)
            return element;

    return NULL;
}

QList<QbElement *> QbPipeline::elements()
{
    return this->m_elements.keys();
}

QbElement *QbPipeline::add(QString pluginId, QString elementName)
{
    QbElement *element = this->m_pluginLoader->newInstance(pluginId);

    if (!element)
        return element;

    element->setObjectName(elementName);

    if (element)
        this->m_elements[element] = pluginId;

    return element;
}

void QbPipeline::remove(QbElement *element)
{
    if (!this->m_elements.contains(element))
        return;

    element->setState(QbElement::ElementStateNull);
    QString pluginId = this->m_elements[element];
    delete element;
    this->m_elements.remove(element);
    this->m_pluginLoader->deleteInstance(pluginId);
}

void QbPipeline::remove(QString elementName)
{
    this->remove(this->element(elementName));
}

void QbPipeline::link(QbElement *elementSrc, QbElement *elementDst)
{
    QObject::connect(elementSrc,
                     SIGNAL(oStream(const QbPacket &)),
                     elementDst,
                     SLOT(iStream(const QbPacket &)));

    QList<QbElement *> srcSinks = elementSrc->sinks();
    QList<QbElement *> dstSrc = elementDst->srcs();

    srcSinks << elementDst;
    dstSrc << elementSrc;

    elementSrc->setSinks(srcSinks);
    elementDst->setSrcs(dstSrc);
}

void QbPipeline::link(QbElement *elementSrc, QObject *elementDst)
{
    QObject::connect(elementSrc,
                     SIGNAL(oStream(const QbPacket &)),
                     elementDst,
                     SLOT(iStream(const QbPacket &)));
}

void QbPipeline::link(QObject *elementSrc, QbElement *elementDst)
{
    QObject::connect(elementSrc,
                     SIGNAL(oStream(const QbPacket &)),
                     elementDst,
                     SLOT(iStream(const QbPacket &)));
}

void QbPipeline::link(QbPipeline *pipeline, QString elementSrc, QString elementDst)
{
    pipeline->link(pipeline->element(elementSrc), pipeline->element(elementDst));
}

void QbPipeline::link(QbPipeline *pipeline, QString elementSrc, QObject *elementDst)
{
    pipeline->link(pipeline->element(elementSrc), elementDst);
}

void QbPipeline::link(QbPipeline *pipeline, QObject *elementSrc, QString elementDst)
{
    pipeline->link(elementSrc, pipeline->element(elementDst));
}

void QbPipeline::link(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst)
{
    pipeline->link(elementSrc, pipeline->element(elementDst));
}

void QbPipeline::link(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst)
{
    pipeline->link(pipeline->element(elementSrc), elementDst);
}

void QbPipeline::link(QString elementSrc, QString elementDst)
{
    this->link(this, elementSrc, elementDst);
}

void QbPipeline::link(QString elementSrc, QObject *elementDst)
{
    this->link(this, elementSrc, elementDst);
}

void QbPipeline::link(QObject *elementSrc, QString elementDst)
{
    this->link(this, elementSrc, elementDst);
}

void QbPipeline::link(QbElement *elementSrc, QString elementDst)
{
    this->link(this, elementSrc, elementDst);
}

void QbPipeline::link(QString elementSrc, QbElement *elementDst)
{
    this->link(this, elementSrc, elementDst);
}

void QbPipeline::unlink(QbElement *elementSrc, QbElement *elementDst)
{
    QObject::disconnect(elementSrc,
                        SIGNAL(oStream(const QbPacket &)),
                        elementDst,
                        SLOT(iStream(const QbPacket &)));

    QList<QbElement *> srcSinks = elementSrc->sinks();
    QList<QbElement *> dstSrc = elementDst->srcs();

    srcSinks.removeOne(elementDst);
    dstSrc.removeOne(elementSrc);

    elementSrc->setSinks(srcSinks);
    elementDst->setSrcs(dstSrc);
}

void QbPipeline::unlink(QbElement *elementSrc, QObject *elementDst)
{
    QObject::disconnect(elementSrc,
                        SIGNAL(oStream(const QbPacket &)),
                        elementDst,
                        SLOT(iStream(const QbPacket &)));
}

void QbPipeline::unlink(QObject *elementSrc, QbElement *elementDst)
{
    QObject::disconnect(elementSrc,
                        SIGNAL(oStream(const QbPacket &)),
                        elementDst,
                        SLOT(iStream(const QbPacket &)));
}

void QbPipeline::unlink(QbPipeline *pipeline, QString elementSrc, QString elementDst)
{
    pipeline->unlink(pipeline->element(elementSrc), pipeline->element(elementDst));
}

void QbPipeline::unlink(QbPipeline *pipeline, QString elementSrc, QObject *elementDst)
{
    pipeline->unlink(pipeline->element(elementSrc), elementDst);
}

void QbPipeline::unlink(QbPipeline *pipeline, QObject *elementSrc, QString elementDst)
{
    pipeline->unlink(elementSrc, pipeline->element(elementDst));
}

void QbPipeline::unlink(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst)
{
    pipeline->unlink(elementSrc, pipeline->element(elementDst));
}

void QbPipeline::unlink(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst)
{
    pipeline->unlink(pipeline->element(elementSrc), elementDst);
}

void QbPipeline::unlink(QString elementSrc, QString elementDst)
{
    this->unlink(this, elementSrc, elementDst);
}

void QbPipeline::unlink(QString elementSrc, QObject *elementDst)
{
    this->unlink(this, elementSrc, elementDst);
}

void QbPipeline::unlink(QObject *elementSrc, QString elementDst)
{
    this->unlink(this, elementSrc, elementDst);
}

void QbPipeline::unlink(QbElement *elementSrc, QString elementDst)
{
    this->unlink(this, elementSrc, elementDst);
}

void QbPipeline::unlink(QString elementSrc, QbElement *elementDst)
{
    this->unlink(this, elementSrc, elementDst);
}

void QbPipeline::setState(QbElement::ElementState state)
{
    foreach (QbElement *element, this->m_elements.keys())
        element->setState(state);
}

void QbPipeline::resetState()
{
    this->setState(QbElement::ElementStateNull);
}

void QbPipeline::setPluginsPaths(QStringList pluginsPaths)
{
    this->m_pluginLoader->setPluginsPaths(pluginsPaths);
}

void QbPipeline::resetPluginsPaths()
{
    this->setPluginsPaths(QStringList());
}
