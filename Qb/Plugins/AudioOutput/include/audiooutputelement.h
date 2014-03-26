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

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

typedef QSharedPointer<QThread> ThreadPtr;
typedef QSharedPointer<QAudioOutput> AudioOutputPtr;

class AudioOutputElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int bufferSize READ bufferSize NOTIFY bufferSizeChanged)

    Q_PROPERTY(QThread *outputThread READ outputThread
                                     WRITE setOutputThread
                                     RESET resetOutputThread)

    Q_PROPERTY(QString inputCaps READ inputCaps
                                 WRITE setInputCaps
                                 RESET resetInputCaps)

    public:
        explicit AudioOutputElement();
        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE QThread *outputThread() const;
        Q_INVOKABLE QString inputCaps() const;
        bool event(QEvent *event);

    private:
        QThread *m_outputThread;
        QString m_inputCaps;
        QbElementPtr m_convert;
        QAudioDeviceInfo m_audioDeviceInfo;
        AudioOutputPtr m_audioOutput;
        QIODevice *m_outputDevice;
        QByteArray m_audioBuffer;

        QTimer m_pullTimer;
        QTimer m_soundInitTimer;
        QTimer m_soundUninitTimer;

        QMutex m_mutex;
        QWaitCondition m_bufferNotEmpty;
        QWaitCondition m_bufferNotFull;
        ThreadPtr m_outputThreadPtr;

        static void deleteThread(QThread *thread);

        QbCaps findBestOptions(const QbCaps &caps,
                               const QAudioDeviceInfo &deviceInfo,
                               QAudioFormat *bestOption=NULL) const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void elapsedTime(double pts);
        void bufferSizeChanged(int size);
        void requestFrame();

    public slots:
        void setOutputThread(const QThread *outputThread);
        void setInputCaps(const QString &inputCaps);
        void resetOutputThread();
        void resetInputCaps();
        void iStream(const QbPacket &packet);

    private slots:
        bool init();
        void uninit(bool lock=true);
        void processFrame(const QbPacket &packet);
        void pullFrame();
};

#endif // AUDIOOUTPUTELEMENT_H
