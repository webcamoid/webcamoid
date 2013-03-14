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
    Q_PROPERTY(ElementState state READ state WRITE setState RESET resetState)
    Q_PROPERTY(QList<QbElement *> srcs READ srcs WRITE setSrcs RESET resetSrcs)
    Q_PROPERTY(QList<QbElement *> sinks READ sinks WRITE setSinks RESET resetSinks)

    public:
        enum ElementState
        {
            ElementStateNull,
            ElementStateReady,
            ElementStatePaused,
            ElementStatePlaying
        };

        explicit QbElement(QObject *parent=NULL);
        virtual ~QbElement();

        Q_INVOKABLE virtual ElementState state();
        Q_INVOKABLE virtual QList<QbElement *> srcs();
        Q_INVOKABLE virtual QList<QbElement *> sinks();
        Q_INVOKABLE virtual bool link(QObject *dstElement);
        Q_INVOKABLE virtual bool link(QbElementPtr dstElement);
        Q_INVOKABLE virtual bool unlink(QObject *dstElement);
        Q_INVOKABLE virtual bool unlink(QbElementPtr dstElement);

    protected:
        ElementState m_state;
        QList<QbElement *> m_srcs;
        QList<QbElement *> m_sinks;

        virtual bool init();
        virtual void uninit();

    private:
        QString m_pluginId;
        QObject *m_application;
        QMap<QbElement *, QbCaps> m_iCaps;

    signals:
        void oStream(const QbPacket &packet);

    public slots:
        virtual void iStream(const QbPacket &packet);
        virtual void setState(ElementState state);
        virtual void setSrcs(QList<QbElement *> srcs);
        virtual void setSinks(QList<QbElement *> sinks);
        virtual void resetState();
        virtual void resetSrcs();
        virtual void resetSinks();

    friend class QbApplication;
};

Q_DECLARE_METATYPE(QbElement::ElementState)

#endif // QBELEMENT_H
