/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef AUDIODEVCOREAUDIO_H
#define AUDIODEVCOREAUDIO_H

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <QWaitCondition>

#include <akaudiocaps.h>

#include "audiodev.h"

class AudioDevCoreAudio: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevCoreAudio(QObject *parent=NULL);
        ~AudioDevCoreAudio();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const QByteArray &frame);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        QStringList m_inputs;
        QStringList m_outputs;
        AudioUnit m_audioUnit;
        UInt32 m_bufferSize;
        AudioBufferList *m_bufferList;
        QByteArray m_buffer;
        QMutex m_mutex;
        QWaitCondition m_canWrite;
        QWaitCondition m_samplesAvailable;
        int m_maxBufferSize;
        AkAudioCaps m_curCaps;
        bool m_isInput;

        static QString statusToStr(OSStatus status);
        static QString CFStringToString(const CFStringRef &cfstr);
        static QString defaultDevice(bool input, bool *ok=NULL);
        static QStringList listDevices(bool input);
        void clearBuffer();
        QVector<AudioStreamRangedDescription> supportedFormats(AudioStreamID stream,
                                                               AudioObjectPropertySelector selector);
        AkAudioCaps::SampleFormat descriptionToSampleFormat(const AudioStreamBasicDescription &streamDescription);
        static OSStatus devicesChangedCallback(AudioObjectID objectId,
                                               UInt32 nProps,
                                               const AudioObjectPropertyAddress *properties,
                                               void *audioDev);
        static OSStatus defaultInputDeviceChangedCallback(AudioObjectID objectId,
                                                          UInt32 nProps,
                                                          const AudioObjectPropertyAddress *properties,
                                                          void *audioDev);
        static OSStatus defaultOutputDeviceChangedCallback(AudioObjectID objectId,
                                                           UInt32 nProps,
                                                           const AudioObjectPropertyAddress *properties,
                                                           void *audioDev);
        static OSStatus audioCallback(void *audioDev,
                                      AudioUnitRenderActionFlags *actionFlags,
                                      const AudioTimeStamp *timeStamp,
                                      UInt32 busNumber,
                                      UInt32 nFrames,
                                      AudioBufferList *data);
};

#endif // AUDIODEVCOREAUDIO_H
