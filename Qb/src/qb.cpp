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

#include "qb.h"
#include "qbapplication.h"

static QbApplication application;

void Qb::init()
{
    qRegisterMetaType<QbCaps>("QbCaps");
    qRegisterMetaType<QbElement::ElementState>("QbElement::ElementState");
    qRegisterMetaType<QbElement::ElementState>("ElementState");
    qRegisterMetaType<QbFrac>("QbFrac");
    qRegisterMetaType<QbPacket>("QbPacket");

    application.setPluginsPaths(QStringList() << QString("%1/%2").arg(INSTALLLIBDIR).arg(COMMONS_TARGET));
}

QStringList Qb::pluginsPaths()
{
    return application.pluginsPaths();
}

QbElementPtr Qb::create(QString pluginId, QString elementName)
{
    QbElementPtr element = application.newInstance(pluginId);

    if (!elementName.isEmpty())
        element->setObjectName(elementName);

    return element;
}

void Qb::setPluginsPaths(QStringList pluginsPaths)
{
    application.setPluginsPaths(pluginsPaths);
}

void Qb::resetPluginsPaths()
{
    application.resetPluginsPaths();
}
