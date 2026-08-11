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

#include <QObject>

#include "akcommons.h"

class AkAudioMixerPrivate;
class AkAudioCaps;
class AkAudioPacket;

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

    public:
        explicit AkAudioMixer(QObject *parent=nullptr);
        AkAudioMixer(const AkAudioMixer &other) = delete;
        AkAudioMixer &operator =(const AkAudioMixer &other) = delete;
        ~AkAudioMixer();

        Q_INVOKABLE size_t inputs() const;
        Q_INVOKABLE AkAudioCaps outputCaps() const;
        Q_INVOKABLE int latency() const;
        Q_INVOKABLE AkAudioPacket read(size_t samples) const;
        Q_INVOKABLE size_t write(const AkAudioPacket &packet);

    private:
        AkAudioMixerPrivate *d;

    Q_SIGNALS:
        void inputsChanged(size_t inputs);
        void outputCapsChanged(const AkAudioCaps &outputCaps);
        void latencyChanged(int latency);

    public Q_SLOTS:
        void setInputs(size_t inputs);
        void setOutputCaps(const AkAudioCaps &outputCaps);
        void setLatency(int latency);
        void resetInputs();
        void resetOutputCaps();
        void resetLatency();
        bool allocate();
        void deallocate();
        static void registerTypes();
};

Q_DECLARE_METATYPE(AkAudioMixer)

#endif // AKAUDIOMIXER_H
