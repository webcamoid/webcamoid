/* Webcamoid, camera capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

class AudioDeviceBufferPrvate;

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

        bool atEnd() const override;
        qint64 bytesAvailable() const override;
        qint64 bytesToWrite() const override;
        bool canReadLine() const override;
        void close() override;
        bool isSequential() const override;
        bool open(OpenMode mode) override;
        qint64 pos() const override;
        bool reset() override;
        bool seek(qint64 pos) override;
        qint64 size() const override;
        bool waitForBytesWritten(int msecs) override;
        bool waitForReadyRead(int msecs) override;

    private:
        AudioDeviceBufferPrvate *d;

    protected:
        qint64 readData(char *data, qint64 maxSize) override;
        qint64 writeData(const char *data, qint64 maxSize) override;

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
