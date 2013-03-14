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

#include "packetinfo.h"

PacketInfo::PacketInfo(QObject *parent): QObject(parent)
{
    this->resetPacket();
    this->resetData();
}

PacketInfo::PacketInfo(const QbPacket &packet, const QByteArray &data):
    QObject(NULL),
    m_packet(packet),
    m_data(data)
{
}

PacketInfo::PacketInfo(const PacketInfo &other):
    QObject(other.parent()),
    m_packet(other.m_packet),
    m_data(other.m_data)
{
}

PacketInfo::~PacketInfo()
{
}

PacketInfo &PacketInfo::operator =(const PacketInfo &other)
{
    if (this != &other)
    {
        this->m_packet = other.m_packet;
        this->m_data = other.m_data;
    }

    return *this;
}

QbPacket PacketInfo::packet() const
{
    return this->m_packet;
}

QByteArray PacketInfo::data() const
{
    return this->m_data;
}

void PacketInfo::setPacket(const QbPacket &packet)
{
    this->m_packet = packet;
}

void PacketInfo::setData(const QByteArray &data)
{
    this->m_data = data;
}

void PacketInfo::resetPacket()
{
    this->setPacket(QbPacket());
}

void PacketInfo::resetData()
{
    this->setData(QByteArray());
}
