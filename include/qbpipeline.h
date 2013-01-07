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

#ifndef QBPIPELINE_H
#define QBPIPELINE_H

#include "qbplugin.h"

class QbPipeline: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList pluginsPaths
               READ pluginsPaths
               WRITE setPluginsPaths
               RESET resetPluginsPaths)

    public:
        explicit QbPipeline(QObject *parent=0);
        ~QbPipeline();

        Q_INVOKABLE QStringList pluginsPaths();
        Q_INVOKABLE QbElement *element(QString elementName);
        Q_INVOKABLE QbElement *add(QString pluginId, QString elementName="");
        Q_INVOKABLE void remove(QbElement *element);
        Q_INVOKABLE void remove(QString elementName);
        static void link(QbElement *elementSrc, QbElement *elementDst);
        static void link(QbElement *elementSrc, QObject *elementDst);
        static void link(QObject *elementSrc, QbElement *elementDst);
        static void link(QbPipeline *pipeline, QString elementSrc, QString elementDst);
        static void link(QbPipeline *pipeline, QString elementSrc, QObject *elementDst);
        static void link(QbPipeline *pipeline, QObject *elementSrc, QString elementDst);
        static void link(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst);
        static void link(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst);
        void link(QString elementSrc, QString elementDst);
        void link(QString elementSrc, QObject *elementDst);
        void link(QObject *elementSrc, QString elementDst);
        void link(QbElement *elementSrc, QString elementDst);
        void link(QString elementSrc, QbElement *elementDst);
        static void unlink(QbElement *elementSrc, QbElement *elementDst);
        static void unlink(QbElement *elementSrc, QObject *elementDst);
        static void unlink(QObject *elementSrc, QbElement *elementDst);
        static void unlink(QbPipeline *pipeline, QString elementSrc, QString elementDst);
        static void unlink(QbPipeline *pipeline, QString elementSrc, QObject *elementDst);
        static void unlink(QbPipeline *pipeline, QObject *elementSrc, QString elementDst);
        static void unlink(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst);
        static void unlink(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst);
        void unlink(QString elementSrc, QString elementDst);
        void unlink(QString elementSrc, QObject *elementDst);
        void unlink(QObject *elementSrc, QString elementDst);
        void unlink(QbElement *elementSrc, QString elementDst);
        void unlink(QString elementSrc, QbElement *elementDst);

    private:
        QStringList m_pluginsPaths;
        QPluginLoader m_pluginLoader;
        QMap<QString, QbPlugin *> m_plugins;
        QMap<QbElement *, QString> m_elements;
        QMap<QString, QString> m_pluginPath;

        bool isLoaded(QString pluginId);
        bool load(QString pluginId);
        bool unload(QString pluginId);

    public slots:
        void setState(QbElement::ElementState state);
        void resetState();
        void setPluginsPaths(QStringList pluginsPaths);
        void resetPluginsPaths();
};

#endif // QBPIPELINE_H
