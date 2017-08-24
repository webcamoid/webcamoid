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

#ifndef SCREENDEV_H
#define SCREENDEV_H

#include <ak.h>

class DesktopCaptureElement;

class ScreenDev: public QObject
{
    Q_OBJECT

    public:
        explicit ScreenDev(QObject *parent=NULL);
        virtual ~ScreenDev();

        Q_INVOKABLE virtual AkFrac fps() const;
        Q_INVOKABLE virtual QStringList medias();
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams() const;
        Q_INVOKABLE virtual int defaultStream(const QString &mimeType);
        Q_INVOKABLE virtual QString description(const QString &media);
        Q_INVOKABLE virtual AkCaps caps(int stream);

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void fpsChanged(const AkFrac &fps);
        void sizeChanged(const QString &media, const QSize &size);
        void oStream(const AkPacket &packet);

    public slots:
        virtual void setFps(const AkFrac &fps);
        virtual void resetFps();
        virtual void setMedia(const QString &media);
        virtual void resetMedia();
        virtual void setStreams(const QList<int> &streams);
        virtual void resetStreams();
        virtual bool init();
        virtual bool uninit();

    friend class DesktopCaptureElement;
};

#endif // SCREENDEV_H
