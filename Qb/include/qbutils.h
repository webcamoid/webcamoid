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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBUTILS_H
#define QBUTILS_H

#include <QImage>
#include <QAudioFormat>

#include "qbpacket.h"

namespace QbUtils
{
    QbPacket imageToPacket(const QImage &image, const QbPacket &defaultPacket);
    QImage packetToImage(const QbPacket &packet);
    QString defaultSampleFormat(QAudioFormat::SampleType sampleType,
                                int sampleSize,
                                bool planar);
    QString defaultChannelLayout(int nChannels);
}

#endif // QBUTILS_H
