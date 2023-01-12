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

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include "akcaps.h"
#include <akmultimediasourceelement.h>

class MultiSrcElementPrivate;

class MultiSrcElement: public AkMultimediaSourceElement
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

    public:
        enum SeekPosition {
            SeekSet,
            SeekCur,
            SeekEnd,
        };
        Q_ENUM(SeekPosition)

        MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QStringList medias() override;
        Q_INVOKABLE QString media() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE bool loop() const override;
        Q_INVOKABLE bool sync() const;
        Q_INVOKABLE QList<int> listTracks(AkCaps::CapsType type=AkCaps::CapsUnknown);
        Q_INVOKABLE QString streamLanguage(int stream);
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) override;
        Q_INVOKABLE QString description(const QString &media) override;
        Q_INVOKABLE AkCaps caps(int stream) override;
        Q_INVOKABLE qint64 durationMSecs();
        Q_INVOKABLE qint64 currentTimeMSecs();
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;
        Q_INVOKABLE AkElement::ElementState state() const override;

    private:
        MultiSrcElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void mediaLoaded(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void syncChanged(bool sync);
        void error(const QString &message);
        void durationMSecsChanged(qint64 durationMSecs);
        void currentTimeMSecsChanged(qint64 currentTimeMSecs);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);

    public slots:
        void seek(qint64 seekTo, MultiSrcElement::SeekPosition position=SeekSet);
        void setMedia(const QString &media) override;
        void setStreams(const QList<int> &streams) override;
        void setLoop(bool loop) override;
        void setSync(bool sync);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void resetMedia() override;
        void resetStreams() override;
        void resetLoop() override;
        void resetSync();
        void resetMaxPacketQueueSize();
        void resetShowLog();
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(MultiSrcElement::SeekPosition)

#endif // MULTISRCELEMENT_H
