/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#ifndef QBELEMENT_H
#define QBELEMENT_H

#include "qbpacket.h"

class QbApplication;
class QbElement;

typedef QSharedPointer<QbElement> QbElementPtr;

/// Plugin template.
class QbElement: public QObject
{
    Q_OBJECT
    Q_ENUMS(ElementState)
    Q_PROPERTY(QbElement::ElementState state READ state WRITE setState RESET resetState NOTIFY stateChanged)
    Q_PROPERTY(QList<QbElement *> srcs READ srcs WRITE setSrcs RESET resetSrcs)
    Q_PROPERTY(QList<QbElement *> sinks READ sinks WRITE setSinks RESET resetSinks)

    public:
        enum ElementState
        {
            ElementStateNull,
            ElementStatePaused,
            ElementStatePlaying
        };

        explicit QbElement(QObject *parent=NULL);
        virtual ~QbElement();

        Q_INVOKABLE virtual QbElement::ElementState state();
        Q_INVOKABLE virtual QList<QbElement *> srcs();
        Q_INVOKABLE virtual QList<QbElement *> sinks();

        Q_INVOKABLE virtual bool link(QObject *dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection);

        Q_INVOKABLE virtual bool link(QbElementPtr dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection);

        Q_INVOKABLE virtual bool unlink(QObject *dstElement);
        Q_INVOKABLE virtual bool unlink(QbElementPtr dstElement);

        Q_INVOKABLE static bool link(QbElementPtr srcElement, QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);

        Q_INVOKABLE static bool link(QbElementPtr srcElement, QbElementPtr dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);

        Q_INVOKABLE static bool unlink(QbElementPtr srcElement, QObject *dstElement);
        Q_INVOKABLE static bool unlink(QbElementPtr srcElement, QbElementPtr dstElement);

    protected:
        QbElement::ElementState m_state;
        QList<QbElement *> m_srcs;
        QList<QbElement *> m_sinks;

        virtual void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        QString m_pluginId;
        QObject *m_application;

        QList<QMetaMethod> methodsByName(QObject *object, QString methodName);
        bool methodCompat(QMetaMethod method1, QMetaMethod method2);

    signals:
        void stateChanged(QbElement::ElementState state);
        void oStream(const QbPacket &packet);

    public slots:
        virtual void iStream(const QbPacket &packet);
        virtual void setState(QbElement::ElementState state);
        virtual void setSrcs(QList<QbElement *> srcs);
        virtual void setSinks(QList<QbElement *> sinks);
        virtual void resetState();
        virtual void resetSrcs();
        virtual void resetSinks();

    friend class QbApplication;
};

Q_DECLARE_METATYPE(QbElement::ElementState)

#endif // QBELEMENT_H
