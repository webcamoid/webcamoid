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

#ifndef PACKETINFO_H
#define PACKETINFO_H

#include <qb.h>

class PacketInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QbPacket packet READ packet WRITE setPacket RESET resetPacket)
    Q_PROPERTY(QByteArray data READ data WRITE setData RESET resetData)

    public:
        explicit PacketInfo(QObject *parent=NULL);
        PacketInfo(const QbPacket &packet);
        PacketInfo(const PacketInfo &other);
        ~PacketInfo();

        PacketInfo &operator =(const PacketInfo &other);

        Q_INVOKABLE QbPacket packet() const;
        Q_INVOKABLE QByteArray data() const;

    private:
        QbPacket m_packet;
        QByteArray m_data;

    public slots:
        void setPacket(const QbPacket &packet);
        void setData(const QByteArray &data);
        void resetPacket();
        void resetData();
};

Q_DECLARE_METATYPE(PacketInfo)

#endif // PACKETINFO_H
