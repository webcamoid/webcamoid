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

#include "qbpluginloader.h"

class QbPipeline: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList pluginsPaths READ pluginsPaths
                                        WRITE setPluginsPaths
                                        RESET resetPluginsPaths)

    Q_PROPERTY(QList<QbElement *> elements READ elements)

    public:
        explicit QbPipeline(QObject *parent=NULL);
        ~QbPipeline();

        Q_INVOKABLE QStringList pluginsPaths();
        Q_INVOKABLE QbElement *element(QString elementName);
        Q_INVOKABLE QList<QbElement *> elements();
        Q_INVOKABLE QbElement *add(QString pluginId, QString elementName="");
        Q_INVOKABLE void remove(QbElement *element);
        Q_INVOKABLE void remove(QString elementName);
        Q_INVOKABLE static void link(QbElement *elementSrc, QbElement *elementDst);
        Q_INVOKABLE static void link(QbElement *elementSrc, QObject *elementDst);
        Q_INVOKABLE static void link(QObject *elementSrc, QbElement *elementDst);
        Q_INVOKABLE static void link(QbPipeline *pipeline, QString elementSrc, QString elementDst);
        Q_INVOKABLE static void link(QbPipeline *pipeline, QString elementSrc, QObject *elementDst);
        Q_INVOKABLE static void link(QbPipeline *pipeline, QObject *elementSrc, QString elementDst);
        Q_INVOKABLE static void link(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst);
        Q_INVOKABLE static void link(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst);
        Q_INVOKABLE void link(QString elementSrc, QString elementDst);
        Q_INVOKABLE void link(QString elementSrc, QObject *elementDst);
        Q_INVOKABLE void link(QObject *elementSrc, QString elementDst);
        Q_INVOKABLE void link(QbElement *elementSrc, QString elementDst);
        Q_INVOKABLE void link(QString elementSrc, QbElement *elementDst);
        Q_INVOKABLE static void unlink(QbElement *elementSrc, QbElement *elementDst);
        Q_INVOKABLE static void unlink(QbElement *elementSrc, QObject *elementDst);
        Q_INVOKABLE static void unlink(QObject *elementSrc, QbElement *elementDst);
        Q_INVOKABLE static void unlink(QbPipeline *pipeline, QString elementSrc, QString elementDst);
        Q_INVOKABLE static void unlink(QbPipeline *pipeline, QString elementSrc, QObject *elementDst);
        Q_INVOKABLE static void unlink(QbPipeline *pipeline, QObject *elementSrc, QString elementDst);
        Q_INVOKABLE static void unlink(QbPipeline *pipeline, QbElement *elementSrc, QString elementDst);
        Q_INVOKABLE static void unlink(QbPipeline *pipeline, QString elementSrc, QbElement *elementDst);
        Q_INVOKABLE void unlink(QString elementSrc, QString elementDst);
        Q_INVOKABLE void unlink(QString elementSrc, QObject *elementDst);
        Q_INVOKABLE void unlink(QObject *elementSrc, QString elementDst);
        Q_INVOKABLE void unlink(QbElement *elementSrc, QString elementDst);
        Q_INVOKABLE void unlink(QString elementSrc, QbElement *elementDst);

    private:
        QbPluginLoader *m_pluginLoader;
        QMap<QbElement *, QString> m_elements;

    public slots:
        void setState(QbElement::ElementState state);
        void resetState();
        void setPluginsPaths(QStringList pluginsPaths);
        void resetPluginsPaths();
};

#endif // QBPIPELINE_H
