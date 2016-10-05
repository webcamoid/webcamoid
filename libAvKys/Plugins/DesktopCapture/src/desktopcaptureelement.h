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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef DESKTOPCAPTUREELEMENT_H
#define DESKTOPCAPTUREELEMENT_H

#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QDesktopWidget>

#include <ak.h>
#include <akmultimediasourceelement.h>

class DesktopCaptureElement: public AkMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        explicit DesktopCaptureElement();
        ~DesktopCaptureElement();

        Q_INVOKABLE AkFrac fps() const;

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;

    private:
        AkFrac m_fps;
        QString m_curScreen;
        int m_curScreenNumber;
        qint64 m_id;
        bool m_threadedRead;
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        AkPacket m_curPacket;

        static void sendPacket(DesktopCaptureElement *element,
                               const AkPacket &packet);

    signals:
        void fpsChanged(const AkFrac &fps);
        void sizeChanged(const QString &media, const QSize &size);

    public slots:
        void setFps(const AkFrac &fps);
        void resetFps();
        void setMedia(const QString &media);
        void resetMedia();
        bool setState(AkElement::ElementState state);

    private slots:
        void readFrame();
        void screenCountChanged(QScreen *screen);
        void srceenResized(int screen);
};

#endif // DESKTOPCAPTUREELEMENT_H
