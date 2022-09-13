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

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <akelement.h>
#include <akcaps.h>

class MediaSource: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop
               NOTIFY loopChanged)
    Q_PROPERTY(bool sync
               READ sync
               WRITE setSync
               RESET resetSync
               NOTIFY syncChanged)
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
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        enum SeekPosition {
            SeekSet,
            SeekCur,
            SeekEnd,
        };

        MediaSource(QObject *parent=nullptr);
        virtual ~MediaSource() = default;

        Q_INVOKABLE virtual QStringList medias() const;
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams() const;
        Q_INVOKABLE virtual QList<int> listTracks(AkCaps::CapsType type);
        Q_INVOKABLE virtual QString streamLanguage(int stream);
        Q_INVOKABLE virtual bool loop() const;
        Q_INVOKABLE virtual bool sync() const;
        Q_INVOKABLE virtual int defaultStream(AkCaps::CapsType type);
        Q_INVOKABLE virtual QString description(const QString &media) const;
        Q_INVOKABLE virtual AkCaps caps(int stream);
        Q_INVOKABLE virtual qint64 durationMSecs();
        Q_INVOKABLE virtual qint64 currentTimeMSecs();
        Q_INVOKABLE virtual qint64 maxPacketQueueSize() const;
        Q_INVOKABLE virtual bool showLog() const;
        Q_INVOKABLE virtual AkElement::ElementState state() const;

    signals:
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);
        void error(const QString &message);
        void durationMSecsChanged(qint64 durationMSecs);
        void currentTimeMSecsChanged(qint64 currentTimeMSecs);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);
        void loopChanged(bool loop);
        void syncChanged(bool sync);
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void mediaLoaded(const QString &media);
        void streamsChanged(const QList<int> &streams);

    public slots:
        virtual void seek(qint64 seekTo, MediaSource::SeekPosition position);
        virtual void setMedia(const QString &media);
        virtual void setStreams(const QList<int> &streams);
        virtual void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        virtual void setShowLog(bool showLog);
        virtual void setLoop(bool loop);
        virtual void setSync(bool sync);
        virtual bool setState(AkElement::ElementState state);
        virtual void resetMedia();
        virtual void resetStreams();
        virtual void resetMaxPacketQueueSize();
        virtual void resetShowLog();
        virtual void resetLoop();
        virtual void resetSync();
        virtual void resetState();
};

#endif // MEDIASOURCE_H
