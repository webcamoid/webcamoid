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
