/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef SCREENDEV_H
#define SCREENDEV_H

#include <akfrac.h>
#include <akcaps.h>
#include <akvideocaps.h>

class DesktopCaptureElement;
class AkPacket;

class ScreenDev: public QObject
{
    Q_OBJECT

    public:
        ScreenDev(QObject *parent=nullptr);
        virtual ~ScreenDev() = default;

        Q_INVOKABLE virtual AkFrac fps() const = 0;
        Q_INVOKABLE virtual QStringList medias() = 0;
        Q_INVOKABLE virtual QString media() const = 0;
        Q_INVOKABLE virtual QList<int> streams() const = 0;
        Q_INVOKABLE virtual int defaultStream(AkCaps::CapsType type) = 0;
        Q_INVOKABLE virtual QString description(const QString &media) = 0;
        Q_INVOKABLE virtual AkVideoCaps caps(int stream) = 0;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void fpsChanged(const AkFrac &fps);
        void sizeChanged(const QString &media, const QSize &size);
        void oStream(const AkPacket &packet);

    public slots:
        virtual void setFps(const AkFrac &fps) = 0;
        virtual void resetFps() = 0;
        virtual void setMedia(const QString &media) = 0;
        virtual void resetMedia() = 0;
        virtual void setStreams(const QList<int> &streams) = 0;
        virtual void resetStreams() = 0;
        virtual bool init() = 0;
        virtual bool uninit() = 0;

    friend class DesktopCaptureElement;
};

#endif // SCREENDEV_H
