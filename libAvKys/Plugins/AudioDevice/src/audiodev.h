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

#ifndef AUDIODEV_H
#define AUDIODEV_H

#include <akaudiocaps.h>

class AudioDevPrivate;
class AkAudioPacket;

class AudioDev: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)

    public:
        AudioDev(QObject *parent=nullptr);
        virtual ~AudioDev();

        Q_INVOKABLE int latency() const;
        Q_INVOKABLE const QVector<int> &commonSampleRates() const;
        Q_INVOKABLE virtual QString error() const;
        Q_INVOKABLE virtual QString defaultInput();
        Q_INVOKABLE virtual QString defaultOutput();
        Q_INVOKABLE virtual QStringList inputs();
        Q_INVOKABLE virtual QStringList outputs();
        Q_INVOKABLE virtual QString description(const QString &device);
        Q_INVOKABLE virtual AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE virtual QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device);
        Q_INVOKABLE virtual QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device);
        Q_INVOKABLE virtual QList<int> supportedSampleRates(const QString &device);
        Q_INVOKABLE virtual bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE virtual QByteArray read();
        Q_INVOKABLE virtual bool write(const AkAudioPacket &packet);
        Q_INVOKABLE virtual bool uninit();

    private:
        AudioDevPrivate *d;

    Q_SIGNALS:
        void latencyChanged(int latency);
        void errorChanged(const QString &error);
        void defaultInputChanged(const QString &defaultInput);
        void defaultOutputChanged(const QString &defaultOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);

    public Q_SLOTS:
        void setLatency(int latency);
        void resetLatency();
};

#endif // AUDIODEV_H
