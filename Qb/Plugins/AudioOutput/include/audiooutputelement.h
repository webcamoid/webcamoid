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

#ifndef AUDIOOUTPUTELEMENT_H
#define AUDIOOUTPUTELEMENT_H

#include <QAudioOutput>
#include <QAudioDeviceInfo>

#include <qbutils.h>
#include "audiobuffer.h"

typedef QSharedPointer<QAudioOutput> AudioOutputPtr;

class AudioOutputElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int bufferSize READ bufferSize
                              WRITE setBufferSize
                              RESET resetBufferSize)

    Q_PROPERTY(double clock READ clock NOTIFY elapsedTime)

    public:
        explicit AudioOutputElement();
        ~AudioOutputElement();
        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE double clock() const;

    private:
        int m_bufferSize;
        QbElementPtr m_convert;
        QAudioDeviceInfo m_audioDeviceInfo;
        AudioOutputPtr m_audioOutput;
        QIODevice *m_outputDevice;
        AudioBuffer m_audioBuffer;
        qint64 m_streamId;

        QMutex m_mutex;
        QWaitCondition m_bufferEmpty;
        double m_timeDrift;

        QbCaps findBestOptions(const QAudioFormat &audioFormat) const;

        double hwClock() const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void elapsedTime(double pts);

    public slots:
        void setBufferSize(int bufferSize);
        void resetBufferSize();
        QbPacket iStream(const QbPacket &packet);

    private slots:
        bool init();
        void uninit();
        void releaseInput();
        void updateClock();
};

#endif // AUDIOOUTPUTELEMENT_H
