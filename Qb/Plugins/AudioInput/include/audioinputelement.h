/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#ifndef AUDIOINPUTELEMENT_H
#define AUDIOINPUTELEMENT_H

#include <QAudioInput>
#include <QAudioDeviceInfo>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

#include "audiobuffer.h"

typedef QSharedPointer<QAudioInput> AudioInputPtr;

class AudioInputElement: public QbElement
{
    Q_OBJECT
        Q_PROPERTY(int bufferSize READ bufferSize
                                  WRITE setBufferSize
                                  RESET resetBufferSize)

        Q_PROPERTY(QString streamCaps READ streamCaps)

    public:
        explicit AudioInputElement();
        ~AudioInputElement();
        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE QString streamCaps() const;

    private:
        int m_bufferSize;
        QbCaps m_caps;
        QAudioDeviceInfo m_audioDeviceInfo;
        AudioInputPtr m_audioInput;
        QIODevice *m_inputDevice;
        AudioBuffer m_audioBuffer;
        qint64 m_streamId;
        qint64 m_pts;
        QbFrac m_timeBase;

        QbCaps findBestOptions(const QAudioFormat &audioFormat) const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    public slots:
        void setBufferSize(int bufferSize);
        void resetBufferSize();

    private slots:
        bool init();
        void uninit();
        void processFrame(const QByteArray &data);
};

#endif // AUDIOINPUTELEMENT_H
