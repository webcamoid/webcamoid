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

#include "audiobuffer.h"

AudioBuffer::AudioBuffer(QObject *parent):
    QIODevice(parent)
{
}

bool AudioBuffer::atEnd() const
{
    return false;
}

qint64 AudioBuffer::bytesAvailable() const
{
    return 0;
}

qint64 AudioBuffer::bytesToWrite() const
{
    return 0;
}

bool AudioBuffer::canReadLine() const
{
    return false;
}

bool AudioBuffer::isSequential() const
{
    return true;
}

qint64 AudioBuffer::pos() const
{
    return 0;
}

bool AudioBuffer::seek(qint64 pos)
{
    Q_UNUSED(pos)

    return false;
}

qint64 AudioBuffer::size() const
{
    return this->bytesAvailable();
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)

    return 0;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize)
{
    emit this->dataReady(QByteArray(data, maxSize));

    return maxSize;
}
