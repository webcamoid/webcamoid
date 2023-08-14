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

#ifndef IMAGESRCELEMENT_H
#define IMAGESRCELEMENT_H

#include <akmultimediasourceelement.h>

class ImageSrcElementPrivate;
class AkFrac;

class ImageSrcElement: public AkMultimediaSourceElement
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
    Q_PROPERTY(bool isAnimated
               READ isAnimated
               NOTIFY isAnimatedChanged)
    Q_PROPERTY(bool forceFps
               READ forceFps
               WRITE setForceFps
               RESET resetForceFps
               NOTIFY forceFpsChanged)
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)
    Q_PROPERTY(QStringList supportedFormats
               READ supportedFormats
               CONSTANT)

    public:
        ImageSrcElement();
        ~ImageSrcElement();

        Q_INVOKABLE QStringList medias() override;
        Q_INVOKABLE QString media() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) override;
        Q_INVOKABLE QString description(const QString &media) override;
        Q_INVOKABLE AkCaps caps(int stream) override;
        Q_INVOKABLE bool isAnimated() const;
        Q_INVOKABLE bool forceFps() const;
        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE QStringList supportedFormats() const;

    private:
        ImageSrcElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void isAnimatedChanged(bool isAnimated);
        void forceFpsChanged(bool forceFps);
        void fpsChanged(const AkFrac &fps);
        void sizeChanged(const QSize &size);
        void error(const QString &message);

    public slots:
        void setForceFps(bool forceFps);
        void setFps(const AkFrac &fps);
        void resetForceFps();
        void resetFps();
        void setMedia(const QString &media) override;
        void resetMedia() override;
        bool setState(AkElement::ElementState state) override;
};

#endif // IMAGESRCELEMENT_H
