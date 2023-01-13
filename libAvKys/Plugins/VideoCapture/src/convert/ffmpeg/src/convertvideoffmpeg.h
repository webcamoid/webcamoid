/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef CONVERTVIDEOFFMPEG_H
#define CONVERTVIDEOFFMPEG_H

#include "convertvideo.h"

class ConvertVideoFFmpegPrivate;
struct AVFrame;

class ConvertVideoFFmpeg: public ConvertVideo
{
    Q_OBJECT
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize
               NOTIFY maxPacketQueueSizeChanged)
    Q_PROPERTY(bool showLog
               READ showLog
               WRITE setShowLog
               RESET resetShowLog
               NOTIFY showLogChanged)

    public:
        ConvertVideoFFmpeg(QObject *parent=nullptr);
        ~ConvertVideoFFmpeg();

        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;
        Q_INVOKABLE void packetEnqueue(const AkPacket &packet) override;
        Q_INVOKABLE void dataEnqueue(AVFrame *frame);
        Q_INVOKABLE bool init(const AkCaps &caps) override;
        Q_INVOKABLE void uninit() override;

    private:
        ConvertVideoFFmpegPrivate *d;

    signals:
        void maxPacketQueueSizeChanged(qint64 maxPacketQueueSize);
        void showLogChanged(bool showLog);

    public slots:
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void resetMaxPacketQueueSize();
        void resetShowLog();

        friend class ConvertVideoFFmpegPrivate;
};

#endif // CONVERTVIDEOFFMPEG_H
