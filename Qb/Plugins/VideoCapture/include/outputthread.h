/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef OUTPUTTHREAD_H
#define OUTPUTTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <qbpacket.h>

class OutputThread: public QThread
{
    Q_OBJECT

    public:
        explicit OutputThread(QObject *parent=NULL);
        ~OutputThread();

    private:
        QbPacket m_packet;
        QMutex m_mutex;
        QWaitCondition m_packetReady;

    protected:
        void run() Q_DECL_OVERRIDE;

    signals:
        void oStream(const QbPacket &packet);

    public slots:
        void setPacket(const QbPacket &packet);
};

#endif // OUTPUTTHREAD_H
