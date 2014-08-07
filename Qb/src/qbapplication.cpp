/* Webcamoid, webcam capture application.
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

#include "qbapplication.h"

QbApplication::QbApplication(QObject *parent): QObject(parent)
{
    this->resetPluginsPaths();
}

QbApplication::~QbApplication()
{
    QStringList plugins = this->m_plugins.keys();

    foreach (QString pluginId, plugins)
        this->unload(pluginId);
}

QStringList QbApplication::pluginsPaths() const
{
    return this->m_pluginsPaths;
}

QbElementPtr QbApplication::newInstance(const QString &pluginId)
{
    if (!this->load(pluginId))
        return QbElementPtr();

    QbElementPtr element(qobject_cast<QbElement *>(this->m_plugins[pluginId]->create("", "")));

    element->m_application = this;
    element->m_pluginId = pluginId;

    if (this->m_instances.contains(pluginId))
        this->m_instances[pluginId]++;
    else
        this->m_instances[pluginId] = 1;

    return element;
}

void QbApplication::deleteInstance(const QString &pluginId)
{
    if (!this->isLoaded(pluginId))
        return;

    this->m_instances[pluginId]--;

    if (this->m_instances[pluginId] < 1) {
        this->unload(pluginId);

        this->m_instances.remove(pluginId);
    }
}

bool QbApplication::isLoaded(const QString &pluginId)
{
    return this->m_plugins.contains(pluginId);
}

bool QbApplication::load(const QString &pluginId)
{
    if (this->isLoaded(pluginId))
        return true;

    QString fileName;

    for (int i = this->m_pluginsPaths.length() - 1; i >= 0; i--) {
        QString path = this->m_pluginsPaths[i];
        QString filePath = QString("%1%2lib%3.so").arg(path)
                                                  .arg(QDir::separator())
                                                  .arg(pluginId);

        if (QFileInfo(filePath).exists()) {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty() || !QLibrary::isLibrary(fileName))
        return false;

    this->m_pluginPath[pluginId] = fileName;
    this->m_pluginLoader.setFileName(fileName);

    if (!this->m_pluginLoader.load()) {
        qDebug() << this->m_pluginLoader.errorString();

        return false;
    }

    QbPluginPtr plugin(qobject_cast<QbPlugin *>(this->m_pluginLoader.instance()));

    if (!plugin)
        return false;

    this->m_plugins[pluginId] = plugin;

    return true;
}

bool QbApplication::unload(const QString &pluginId)
{
    if(!this->isLoaded(pluginId))
         return true;

    this->m_plugins.remove(pluginId);
    this->m_pluginLoader.setFileName(this->m_pluginPath[pluginId]);
    this->m_pluginLoader.unload();

    return true;
}

void QbApplication::setPluginsPaths(const QStringList &pluginsPaths)
{
    this->m_pluginsPaths = pluginsPaths;
}

void QbApplication::resetPluginsPaths()
{
    this->setPluginsPaths(QStringList());
}
