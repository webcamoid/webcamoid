/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <akmultimediasourceelement.h>

#ifdef USE_GSTREAMER
#include "gstreamer/mediasource.h"
#else
#include "ffmpeg/mediasource.h"
#endif

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
        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE QList<int> listTracks(const QString &type="");
        Q_INVOKABLE QString streamLanguage(int stream);

        using AkMultimediaSourceElement::defaultStream;
        using AkMultimediaSourceElement::caps;

        Q_INVOKABLE int defaultStream(const QString &mimeType);
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream);
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;

    protected:
        void stateChange(AkElement::ElementState from,
                         AkElement::ElementState to);

    private:
        MediaSource m_mediaSource;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void error(const QString &message);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);

    public slots:
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setLoop(bool loop);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void resetMedia();
        void resetStreams();
        void resetLoop();
        void resetMaxPacketQueueSize();
        void resetShowLog();
};

#endif // MULTISRCELEMENT_H
