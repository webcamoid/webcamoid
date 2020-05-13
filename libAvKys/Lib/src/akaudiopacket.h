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

#include "akaudiocaps.h"

class AkAudioPacketPrivate;
class AkPacket;
class AkFrac;

class AKCOMMONS_EXPORT AkAudioPacket: public QObject
{
    Q_OBJECT
    Q_ENUMS(ResampleMethod)
    Q_PROPERTY(AkAudioCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(QByteArray buffer
               READ buffer
               WRITE setBuffer
               RESET resetBuffer
               NOTIFY bufferChanged)
    Q_PROPERTY(qint64 id
               READ id
               WRITE setId
               RESET resetId
               NOTIFY idChanged)
    Q_PROPERTY(qint64 pts
               READ pts
               WRITE setPts
               RESET resetPts
               NOTIFY ptsChanged)
    Q_PROPERTY(AkFrac timeBase
               READ timeBase
               WRITE setTimeBase
               RESET resetTimeBase
               NOTIFY timeBaseChanged)
    Q_PROPERTY(int index
               READ index
               WRITE setIndex
               RESET resetIndex
               NOTIFY indexChanged)

    public:
        enum ResampleMethod
        {
            ResampleMethod_Fast,
            ResampleMethod_Linear,
            ResampleMethod_Quadratic
        };

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
        operator AkPacket() const;

        Q_INVOKABLE AkAudioCaps caps() const;
        Q_INVOKABLE AkAudioCaps &caps();
        Q_INVOKABLE QByteArray buffer() const;
        Q_INVOKABLE QByteArray &buffer();
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE qint64 &id();
        Q_INVOKABLE qint64 pts() const;
        Q_INVOKABLE qint64 &pts();
        Q_INVOKABLE AkFrac timeBase() const;
        Q_INVOKABLE AkFrac &timeBase();
        Q_INVOKABLE int index() const;
        Q_INVOKABLE int &index();
        Q_INVOKABLE void copyMetadata(const AkAudioPacket &other);

        Q_INVOKABLE const quint8 *constPlaneData(int plane) const;
        Q_INVOKABLE quint8 *planeData(int plane);
        Q_INVOKABLE const quint8 *constSample(int channel, int i) const;
        Q_INVOKABLE quint8 *sample(int channel, int i);
        Q_INVOKABLE void setSample(int channel, int i, const quint8 *sample);
        Q_INVOKABLE AkAudioPacket convert(const AkAudioCaps &caps) const;
        Q_INVOKABLE static bool canConvertFormat(AkAudioCaps::SampleFormat input,
                                           AkAudioCaps::SampleFormat output);
        Q_INVOKABLE bool canConvertFormat(AkAudioCaps::SampleFormat output) const;
        Q_INVOKABLE AkAudioPacket convertFormat(AkAudioCaps::SampleFormat format) const;
        Q_INVOKABLE AkAudioPacket convertLayout(AkAudioCaps::ChannelLayout layout) const;
        Q_INVOKABLE AkAudioPacket convertSampleRate(int rate,
                                                    qreal &sampleCorrection,
                                                    ResampleMethod method=ResampleMethod_Fast) const;
        Q_INVOKABLE AkAudioPacket scale(int samples,
                                        ResampleMethod method=ResampleMethod_Fast) const;
        Q_INVOKABLE AkAudioPacket convertPlanar(bool planar) const;
        Q_INVOKABLE AkAudioPacket realign(int align) const;
        Q_INVOKABLE AkAudioPacket pop(int samples);

    private:
        AkAudioPacketPrivate *d;

    Q_SIGNALS:
        void capsChanged(const AkAudioCaps &caps);
        void bufferChanged(const QByteArray &buffer);
        void idChanged(qint64 id);
        void ptsChanged(qint64 pts);
        void timeBaseChanged(const AkFrac &timeBase);
        void indexChanged(int index);

    public Q_SLOTS:
        void setCaps(const AkAudioCaps &caps);
        void setBuffer(const QByteArray &buffer);
        void setId(qint64 id);
        void setPts(qint64 pts);
        void setTimeBase(const AkFrac &timeBase);
        void setIndex(int index);
        void resetCaps();
        void resetBuffer();
        void resetId();
        void resetPts();
        void resetTimeBase();
        void resetIndex();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkAudioPacket &packet);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioPacket::ResampleMethod method);

Q_DECLARE_METATYPE(AkAudioPacket)
Q_DECLARE_METATYPE(AkAudioPacket::ResampleMethod)

#endif // AKAUDIOPACKET_H
