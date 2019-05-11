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

#ifndef AKAUDIOPACKET_H
#define AKAUDIOPACKET_H

#include "akpacket.h"
#include "akaudiocaps.h"

class AkAudioPacketPrivate;
class AkAudioCaps;

class AKCOMMONS_EXPORT AkAudioPacket: public AkPacket
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        AkAudioPacket(QObject *parent=nullptr);
        AkAudioPacket(const AkAudioCaps &caps);
        AkAudioPacket(const AkPacket &other);
        AkAudioPacket(const AkAudioPacket &other);
        ~AkAudioPacket();
        AkAudioPacket &operator =(const AkPacket &other);
        AkAudioPacket &operator =(const AkAudioPacket &other);
        AkAudioPacket operator +(const AkAudioPacket &other);
        AkAudioPacket& operator +=(const AkAudioPacket &other);
        operator bool() const;

        Q_INVOKABLE AkAudioCaps caps() const;
        Q_INVOKABLE AkAudioCaps &caps();
        Q_INVOKABLE const quint8 *constPlaneData(int plane) const;
        Q_INVOKABLE quint8 *planeData(int plane);
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkPacket toPacket() const;
        Q_INVOKABLE static bool canConvert(AkAudioCaps::SampleFormat input,
                                           AkAudioCaps::SampleFormat output);
        Q_INVOKABLE bool canConvert(AkAudioCaps::SampleFormat output) const;
        Q_INVOKABLE AkAudioPacket convert(AkAudioCaps::SampleFormat format) const;
        Q_INVOKABLE AkAudioPacket convert(AkAudioCaps::SampleFormat format,
                                          int align) const;
        Q_INVOKABLE static bool canConvertLayout(AkAudioCaps::ChannelLayout input,
                                                 AkAudioCaps::ChannelLayout output);
        Q_INVOKABLE bool canConvertLayout(AkAudioCaps::ChannelLayout output) const;
        Q_INVOKABLE AkAudioPacket convertLayout(AkAudioCaps::ChannelLayout layout) const;
        Q_INVOKABLE AkAudioPacket convertLayout(AkAudioCaps::ChannelLayout layout, int align) const;
        Q_INVOKABLE AkAudioPacket convertPlanar(bool planar) const;
        Q_INVOKABLE AkAudioPacket convertPlanar(bool planar, int align) const;
        Q_INVOKABLE AkAudioPacket realign(int align) const;
        Q_INVOKABLE void copyMetadata(const AkPacket &other);
        Q_INVOKABLE void copyMetadata(const AkAudioPacket &other);

    private:
        AkAudioPacketPrivate *d;

    Q_SIGNALS:
        void capsChanged(const AkAudioCaps &caps);

    public Q_SLOTS:
        void setCaps(const AkAudioCaps &caps);
        void resetCaps();

        friend QDebug operator <<(QDebug debug, const AkAudioPacket &packet);
};

QDebug operator <<(QDebug debug, const AkAudioPacket &packet);

Q_DECLARE_METATYPE(AkAudioPacket)

#endif // AKAUDIOPACKET_H
