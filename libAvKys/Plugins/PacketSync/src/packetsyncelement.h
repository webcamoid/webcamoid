/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef PACKETSYNCELEMENT_H
#define PACKETSYNCELEMENT_H

#include <iak/akelement.h>

class PacketSyncElementPrivate;

class PacketSyncElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(bool audioEnabled
               READ audioEnabled
               WRITE setAudioEnabled
               RESET resetAudioEnabled
               NOTIFY audioEnabledChanged)
    Q_PROPERTY(bool discardLast
               READ discardLast
               WRITE setDiscardLast
               RESET resetDiscardLast
               NOTIFY discardLastChanged)

    public:
        PacketSyncElement();
        ~PacketSyncElement();

        Q_INVOKABLE bool audioEnabled() const;
        Q_INVOKABLE bool discardLast() const;

    private:
        PacketSyncElementPrivate *d;

    signals:
        void audioEnabledChanged(bool audioEnabled);
        void discardLastChanged(bool discardLast);

    public slots:
        void setAudioEnabled(bool audioEnabled);
        void setDiscardLast(bool discardLast);
        void resetAudioEnabled();
        void resetDiscardLast();
        AkPacket iStream(const AkPacket &packet) override;
        bool setState(AkElement::ElementState state) override;
};

#endif // PACKETSYNCELEMENT_H
