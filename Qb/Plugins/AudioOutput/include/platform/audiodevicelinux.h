/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <qbaudiocaps.h>
#include <pulse/simple.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <pulse/error.h>

class AudioDevice: public QObject
{
    Q_OBJECT
    Q_ENUMS(DeviceMode)
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)

    public:
        enum DeviceMode
        {
            DeviceModeCapture,
            DeviceModePlayback
        };

        explicit AudioDevice(QObject *parent=NULL);
        ~AudioDevice();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE bool preferredFormat(DeviceMode mode,
                                         QbAudioCaps::SampleFormat *sampleFormat,
                                         int *channels,
                                         int *sampleRate);
        Q_INVOKABLE bool init(DeviceMode mode,
                              QbAudioCaps::SampleFormat sampleFormat,
                              int channels,
                              int sampleRate);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const QByteArray &frame);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        pa_simple *m_paSimple;
        pa_threaded_mainloop *m_mainLoop;
        QString m_defaultSink;
        QString m_defaultSource;
        pa_sample_format_t m_defaultFormat;
        int m_defaultChannels;
        int m_defaultRate;
        int m_curBps;
        int m_curChannels;

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

    signals:
        void errorChanged(const QString & error);
};

#endif // AUDIODEVICE_H
