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

#ifndef RTPTSELEMENT_H
#define RTPTSELEMENT_H

#include <QMutex>
#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <ak.h>

class RtPtsElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        explicit RtPtsElement();
        ~RtPtsElement();

        Q_INVOKABLE AkFrac fps() const;

    private:
        AkFrac m_timeBase;
        AkFrac m_fps;
        QTimer m_timer;
        QElapsedTimer m_elapsedTimer;
        qint64 m_prevPts;
        QMutex m_mutex;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        AkVideoPacket m_inPacket;
        AkVideoPacket m_curPacket;

        static void sendPacket(RtPtsElement *element,
                               const AkVideoPacket &packet);

    protected:
        void stateChange(AkElement::ElementState from,
                         AkElement::ElementState to);

    signals:
        void fpsChanged(const AkFrac &fps);

    public slots:
        void setFps(const AkFrac &fps);
        void resetFps();

        AkPacket iStream(const AkVideoPacket &packet);

    private slots:
        bool init();
        void uninit();
        void readPacket();
};

#endif // RTPTSELEMENT_H
