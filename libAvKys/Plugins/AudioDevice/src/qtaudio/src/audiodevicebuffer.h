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

#ifndef AUDIODEVICEBUFFER_H
#define AUDIODEVICEBUFFER_H

#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>

class AudioDeviceBuffer: public QIODevice
{
    Q_OBJECT
    Q_PROPERTY(qint64 blockSize
               READ blockSize
               WRITE setBlockSize
               RESET resetBlockSize
               NOTIFY blockSizeChanged)
    Q_PROPERTY(qint64 maxBufferSize
               READ maxBufferSize
               WRITE setMaxBufferSize
               RESET resetMaxBufferSize
               NOTIFY maxBufferSizeChanged)

    public:
        AudioDeviceBuffer(QObject *parent=nullptr);
        ~AudioDeviceBuffer();

        Q_INVOKABLE qint64 blockSize() const;
        Q_INVOKABLE qint64 maxBufferSize() const;

        bool atEnd() const;
        qint64 bytesAvailable() const;
        qint64 bytesToWrite() const;
        bool canReadLine() const;
        void close();
        bool isSequential() const;
        bool open(OpenMode mode);
        qint64 pos() const;
        bool reset();
        bool seek(qint64 pos);
        qint64 size() const;
        bool waitForBytesWritten(int msecs);
        bool waitForReadyRead(int msecs);

    private:
        QByteArray m_buffer;
        qint64 m_blockSize;
        qint64 m_maxBufferSize;
        QMutex m_mutex;
        QWaitCondition m_bufferNotFull;
        bool m_isOpen;

    protected:
        qint64 readData(char *data, qint64 maxSize);
        qint64 writeData(const char *data, qint64 maxSize);

    signals:
        void blockSizeChanged(qint64 blockSize);
        void maxBufferSizeChanged(qint64 maxBufferSize);

    public slots:
        void setBlockSize(qint64 blockSize);
        void setMaxBufferSize(qint64 maxBufferSize);
        void resetBlockSize();
        void resetMaxBufferSize();
};

#endif // AUDIODEVICEBUFFER_H
