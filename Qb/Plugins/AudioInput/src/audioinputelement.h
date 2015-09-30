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

#ifndef AUDIOINPUTELEMENT_H
#define AUDIOINPUTELEMENT_H

#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>

#include <qbelement.h>

#ifdef Q_OS_LINUX
#include "pulseaudio/audiodevice.h"
#endif

#ifdef Q_OS_WIN32
#include "wasapi/audiodevice.h"
#endif

class AudioInputElement: public QbElement
{
    Q_OBJECT
    // Buffer size in samples.
    Q_PROPERTY(int bufferSize
               READ bufferSize
               WRITE setBufferSize
               RESET resetBufferSize
               NOTIFY bufferSizeChanged)
    Q_PROPERTY(QbCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit AudioInputElement();
        ~AudioInputElement();

        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE QbCaps caps() const;

    private:
        int m_bufferSize;
        QbCaps m_caps;
        AudioDevice m_audioDevice;
        qint64 m_streamId;
        QbFrac m_timeBase;
        bool m_threadedRead;
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        QbPacket m_curPacket;

        static void sendPacket(AudioInputElement *element,
                               const QbPacket &packet);

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void bufferSizeChanged(int bufferSize);
        void capsChanged(const QbCaps &caps);

    public slots:
        void setBufferSize(int bufferSize);
        void setCaps(const QbCaps &caps);
        void resetBufferSize();
        void resetCaps();

    private slots:
        bool init();
        void uninit();
        void readFrame();
};

#endif // AUDIOINPUTELEMENT_H
