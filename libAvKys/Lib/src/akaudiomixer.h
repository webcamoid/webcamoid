/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef AKAUDIOMIXER_H
#define AKAUDIOMIXER_H

#include "akaudiocaps.h"
#include "iak/akelement.h"

class AkAudioMixerPrivate;
class AkAudioPacket;
class AkPacket;

class AKCOMMONS_EXPORT AkAudioMixer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(size_t inputs
               READ inputs
               WRITE setInputs
               RESET resetInputs
               NOTIFY inputsChanged)
    Q_PROPERTY(AkAudioCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(int latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        explicit AkAudioMixer(QObject *parent=nullptr);
        AkAudioMixer(const AkAudioMixer &other) = delete;
        AkAudioMixer &operator =(const AkAudioMixer &other) = delete;
        ~AkAudioMixer();

        Q_INVOKABLE size_t inputs() const;
        Q_INVOKABLE AkAudioCaps outputCaps() const;
        Q_INVOKABLE int latency() const;
        Q_INVOKABLE AkElement::ElementState state() const;

    private:
        AkAudioMixerPrivate *d;

    Q_SIGNALS:
        void inputsChanged(size_t nSources);
        void outputCapsChanged(const AkAudioCaps &outputCaps);
        void latencyChanged(int latency);
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);

    public Q_SLOTS:
        void setInputs(size_t inputs);
        void setOutputCaps(const AkAudioCaps &outputCaps);
        void setLatency(int latency);
        bool setState(AkElement::ElementState state);
        void resetInputs();
        void resetOutputCaps();
        void resetState();
        void resetLatency();

        // Feed a packet. packet.id() must be the slot index in [0, inputs).
        AkPacket iStream(const AkPacket &packet);

        static void registerTypes();
};

Q_DECLARE_METATYPE(AkAudioMixer)

#endif // AKAUDIOMIXER_H
