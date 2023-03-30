/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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

#ifndef MEDIASOURCEVLC_H
#define MEDIASOURCEVLC_H

#include "mediasource.h"

class MediaSourceVLCPrivate;

class MediaSourceVLC: public MediaSource
{
    Q_OBJECT

    public:
        MediaSourceVLC(QObject *parent=nullptr);
        ~MediaSourceVLC();

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(AkCaps::CapsType type);
        Q_INVOKABLE QString streamLanguage(int stream);
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE bool sync() const;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type);
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream);
        Q_INVOKABLE qint64 durationMSecs();
        Q_INVOKABLE qint64 currentTimeMSecs();
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;
        Q_INVOKABLE AkElement::ElementState state() const;

    private:
        MediaSourceVLCPrivate *d;

    public slots:
        void seek(qint64 mSecs, SeekPosition position);
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void setLoop(bool loop);
        void setSync(bool sync);
        void resetMedia();
        void resetStreams();
        void resetMaxPacketQueueSize();
        void resetShowLog();
        void resetLoop();
        void resetSync();
        bool setState(AkElement::ElementState state);

    friend MediaSourceVLCPrivate;
};

#endif // MEDIASOURCEVLC_H
