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

#ifndef MEDIASOURCEGSTREAMER_H
#define MEDIASOURCEGSTREAMER_H

#include "mediasource.h"

class MediaSourceGStreamerPrivate;

class MediaSourceGStreamer: public MediaSource
{
    Q_OBJECT
    Q_PROPERTY(qint64 durationMSecs
               READ durationMSecs
               NOTIFY durationMSecsChanged)
    Q_PROPERTY(qint64 currentTimeMSecs
               READ currentTimeMSecs
               NOTIFY currentTimeMSecsChanged)
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
        MediaSourceGStreamer(QObject *parent=nullptr);
        ~MediaSourceGStreamer();

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE QString streamLanguage(int stream);
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE int defaultStream(const QString &mimeType);
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream);
        Q_INVOKABLE qint64 durationMSecs();
        Q_INVOKABLE qint64 currentTimeMSecs();
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;

    private:
        MediaSourceGStreamerPrivate *d;

    signals:
        void oStream(const AkPacket &packet);
        void error(const QString &message);
        void durationMSecsChanged(qint64 durationMSecs);
        void currentTimeMSecsChanged(qint64 currentTimeMSecs);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);
        void loopChanged(bool loop);
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);

    public slots:
        void seek(qint64 mSecs, MultiSrcElement::SeekPosition position);
        void nextVideoFrame();
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void setLoop(bool loop);
        void resetMedia();
        void resetStreams();
        void resetMaxPacketQueueSize();
        void resetShowLog();
        void resetLoop();
        bool setState(AkElement::ElementState state);

    private slots:
        void updateStreams();

        friend class MediaSourceGStreamerPrivate;
};

#endif // MEDIASOURCEGSTREAMER_H
