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

#ifndef AUDIOOUT_H
#define AUDIOOUT_H

#include <qbaudiocaps.h>
#include <objbase.h>
#include <xaudio2.h>

#include "voicecallback.h"

class AudioOut: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)

    public:
        explicit AudioOut(QObject *parent=NULL);
        ~AudioOut();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE bool init(QbAudioCaps::SampleFormat sampleFormat,
                              int channels,
                              int sampleRate);
        Q_INVOKABLE bool write(const QByteArray &frame);
        Q_INVOKABLE bool uninit();

    private:
        Lock m_lock;
        QString m_error;
        IXAudio2 *m_pXAudio2;
        IXAudio2MasteringVoice *m_pMasterVoice;
        IXAudio2SourceVoice *m_pSourceVoice;
        VoiceCallback m_voiceCallbacks;

    signals:
        void errorChanged(const QString &error);
};

#endif // AUDIOOUT_H
