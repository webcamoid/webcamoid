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

class AudioOutputElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int bufferSize READ bufferSize NOTIFY bufferSizeChanged)

    public:
        explicit AudioOutputElement();
        Q_INVOKABLE int bufferSize() const;

    private:
        QbElementPtr m_audioConvert;
        QAudioDeviceInfo m_audioDeviceInfo;
        QAudioOutput *m_audioOutput;
        QIODevice *m_outputDevice;
        QByteArray m_audioBuffer;
        QTimer m_pullTimer;
        QMutex m_mutex;
        QWaitCondition m_waitCondition;

        QbCaps findBestOptions(const QbCaps &caps,
                               const QAudioDeviceInfo &deviceInfo,
                               QAudioFormat *bestOption=NULL) const;

    signals:
        void bufferSizeChanged();

    public slots:
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
        void pullFrame();
};

#endif // AUDIOOUTPUTELEMENT_H
