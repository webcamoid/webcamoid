/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef PIPEWIRESCREENDEV_H
#define PIPEWIRESCREENDEV_H

#include "screendev.h"

class PipewireScreenDevPrivate;
class QScreen;

class PipewireScreenDev: public ScreenDev
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
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        PipewireScreenDev();
        ~PipewireScreenDev();

        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE QStringList medias();
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type);
        Q_INVOKABLE QString description(const QString &media);
        Q_INVOKABLE AkVideoCaps caps(int stream);

    private:
        PipewireScreenDevPrivate *d;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void fpsChanged(const AkFrac &fps);
        void sizeChanged(const QString &media, const QSize &size);
        void error(const QString &message);

    public slots:
        void setFps(const AkFrac &fps);
        void resetFps();
        void setMedia(const QString &media);
        void resetMedia();
        void setStreams(const QList<int> &streams);
        void resetStreams();
        bool init();
        bool uninit();

    private slots:
        void screenAdded(QScreen *screen);
        void screenRemoved(QScreen *screen);
        void srceenResized(int screen);
        void responseReceived(quint32 response, const QVariantMap &results);
};

#endif // PIPEWIRESCREENDEV_H
