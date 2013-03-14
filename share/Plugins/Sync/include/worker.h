/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef WORKER_H
#define WORKER_H

#include "packetinfo.h"

class Worker: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QbElement::ElementState state READ state WRITE setState RESET resetState)

    public:
        explicit Worker(QObject *parent=NULL);
        ~Worker();

        Q_INVOKABLE QbElement::ElementState state() const;

    private:
        QbElement::ElementState m_state;
        QQueue<PacketInfo *> m_queue;
        QByteArray m_data;

    signals:
        void oStream(const QbPacket &packet);

    public slots:
        void doWork();
        void appendPacketInfo(const PacketInfo &packetInfo);
        void setState(QbElement::ElementState state);
        void resetState();
};

#endif // WORKER_H
