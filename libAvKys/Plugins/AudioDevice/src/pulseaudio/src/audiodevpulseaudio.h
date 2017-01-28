/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef AUDIODEVPULSEAUDIO_H
#define AUDIODEVPULSEAUDIO_H

#include <QMutex>
#include <akaudiocaps.h>
#include <pulse/simple.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
#include <pulse/thread-mainloop.h>
#include <pulse/error.h>

#include "audiodev.h"

class AudioDevPulseAudio: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevPulseAudio(QObject *parent=NULL);
        ~AudioDevPulseAudio();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device);
        Q_INVOKABLE QList<int> supportedChannels(const QString &device);
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device);
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &frame);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        pa_simple *m_paSimple;
        pa_threaded_mainloop *m_mainLoop;
        pa_context *m_context;
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<uint32_t, QString> m_sinks;
        QMap<uint32_t, QString> m_sources;
        QMap<QString, AkAudioCaps> m_pinCapsMap;
        QMap<QString, QString> m_pinDescriptionMap;
        QMutex m_mutex;
        int m_curBps;
        int m_curChannels;

        static void deviceUpdateCallback(pa_context *context,
                                         pa_subscription_event_type_t eventType,
                                         uint32_t index,
                                         void *userData);
        static void contextStateCallbackInit(pa_context *context,
                                             void *userdata);
        static void serverInfoCallback(pa_context *context,
                                       const pa_server_info *info,
                                       void *userdata);
        static void sourceInfoCallback(pa_context *context,
                                       const pa_source_info *info,
                                       int isLast,
                                       void *userdata);
        static void sinkInfoCallback(pa_context *context,
                                     const pa_sink_info *info,
                                     int isLast,
                                     void *userdata);
};

#endif // AUDIODEVPULSEAUDIO_H
