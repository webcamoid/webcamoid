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

#ifndef RTPTSELEMENT_H
#define RTPTSELEMENT_H

#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>
#include <qb.h>

class RtPtsElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QbFrac fps READ fps WRITE setFps RESET resetFps)

    public:
        explicit RtPtsElement();
        ~RtPtsElement();

        Q_INVOKABLE QbFrac fps() const;

    private:
        QbFrac m_timeBase;
        QbFrac m_fps;
        QbPacket m_curPacket;
        QMutex m_mutex;
        QTimer m_timer;
        QThread *m_thread;
        QElapsedTimer m_elapsedTimer;
        qint64 m_prevPts;

    public slots:
        void setFps(const QbFrac &fps);
        void resetFps();
        void setState(QbElement::ElementState state);
        QbPacket iStream(const QbPacket &packet);

    private slots:
        void readPacket();
};

#endif // RTPTSELEMENT_H
