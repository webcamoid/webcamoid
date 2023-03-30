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

#ifndef AKELEMENT_H
#define AKELEMENT_H

#include <QObject>

#include "akcommons.h"

class AkElement;
class AkElementPrivate;
class AkPacket;
class AkAudioPacket;
class AkVideoPacket;
class QDataStream;
class QQmlEngine;
class QQmlContext;

using AkElementPtr = QSharedPointer<AkElement>;

class AKCOMMONS_EXPORT AkElement: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        enum ElementState
        {
            ElementStateNull,
            ElementStatePaused,
            ElementStatePlaying
        };
        Q_ENUM(ElementState)

        AkElement(QObject *parent=nullptr);
        virtual ~AkElement();

        Q_INVOKABLE virtual AkElement::ElementState state() const;
        Q_INVOKABLE virtual QObject *controlInterface(QQmlEngine *engine,
                                                      const QString &controlId) const;

        Q_INVOKABLE virtual bool link(const QObject *dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool link(const AkElementPtr &dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool unlink(const QObject *dstElement) const;
        Q_INVOKABLE virtual bool unlink(const AkElementPtr &dstElement) const;

        Q_INVOKABLE static bool link(const AkElementPtr &srcElement,
                                     const QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool link(const AkElementPtr &srcElement,
                                     const AkElementPtr &dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool link(const QObject *srcElement,
                                     const QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool unlink(const AkElementPtr &srcElement,
                                       const QObject *dstElement);
        Q_INVOKABLE static bool unlink(const AkElementPtr &srcElement,
                                       const AkElementPtr &dstElement);
        Q_INVOKABLE static bool unlink(const QObject *srcElement,
                                       const QObject *dstElement);

    private:
        AkElementPrivate *d;

    protected:
        virtual QString controlInterfaceProvide(const QString &controlId) const;
        virtual void controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const;
        virtual AkPacket iAudioStream(const AkAudioPacket &packet);
        virtual AkPacket iVideoStream(const AkVideoPacket &packet);

    Q_SIGNALS:
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);

    public Q_SLOTS:
        virtual AkPacket iStream(const AkPacket &packet);
        virtual bool setState(AkElement::ElementState state);
        virtual void resetState();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkElement::ElementState &state);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, AkElement::ElementState state);

Q_DECLARE_METATYPE(AkElement::ElementState)

#endif // AKELEMENT_H
