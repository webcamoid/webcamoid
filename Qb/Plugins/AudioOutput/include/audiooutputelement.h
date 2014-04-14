/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef AUDIOOUTPUTELEMENT_H
#define AUDIOOUTPUTELEMENT_H

#include <QtMultimedia>
#include <qb.h>

#include "thread.h"

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

typedef QSharedPointer<QAudioOutput> AudioOutputPtr;

class AudioOutputElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int bufferSize READ bufferSize NOTIFY bufferSizeChanged)

    Q_PROPERTY(QString inputCaps READ inputCaps
                                 WRITE setInputCaps
                                 RESET resetInputCaps)

    Q_PROPERTY(int maxBufferSize READ maxBufferSize
                                 WRITE setMaxBufferSize
                                 RESET resetMaxBufferSize)

    Q_PROPERTY(double clock READ clock NOTIFY elapsedTime)

    public:
        explicit AudioOutputElement();
        ~AudioOutputElement();
        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE QString inputCaps() const;
        Q_INVOKABLE int maxBufferSize() const;
        Q_INVOKABLE double clock() const;

    private:
        QString m_inputCaps;
        int m_maxBufferSize;
        QbElementPtr m_convert;
        QAudioDeviceInfo m_audioDeviceInfo;
        AudioOutputPtr m_audioOutput;
        QIODevice *m_outputDevice;
        QByteArray m_audioBuffer;
        qint64 m_streamId;

        bool m_run;
        QMutex m_mutex;
        QWaitCondition m_bufferEmpty;
        QWaitCondition m_bufferNotEmpty;
        QWaitCondition m_bufferNotFull;
        Thread *m_outputThread;
        double m_timeDrift;

        QbCaps findBestOptions(const QbCaps &caps,
                               const QAudioDeviceInfo &deviceInfo,
                               QAudioFormat *bestOption=NULL) const;

        double hwClock() const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void elapsedTime(double pts);
        void bufferSizeChanged(int size);
        void requestFrame();

    public slots:
        void setInputCaps(const QString &inputCaps);
        void setMaxBufferSize(int maxBufferSize);
        void resetInputCaps();
        void resetMaxBufferSize();
        void iStream(const QbPacket &packet);

    private slots:
        bool init();
        void uninit();
        void processFrame(const QbPacket &packet);
        void pullFrame();
};

#endif // AUDIOOUTPUTELEMENT_H
