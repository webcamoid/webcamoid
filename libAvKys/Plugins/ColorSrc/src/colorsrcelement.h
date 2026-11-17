/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef COLORSRCELEMENT_H
#define COLORSRCELEMENT_H

#include <QRgb>
#include <iak/akmultimediasourceelement.h>

class ColorSrcElementPrivate;
class AkFrac;

class ColorSrcElement: public AkMultimediaSourceElement
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
    Q_PROPERTY(QRgb color
               READ color
               WRITE setColor
               RESET resetColor
               NOTIFY colorChanged)
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)
    Q_PROPERTY(QString summary
               READ summary
               NOTIFY summaryChanged)

    public:
        ColorSrcElement();
        ~ColorSrcElement();

        Q_INVOKABLE QStringList medias() override;
        Q_INVOKABLE QString media() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) override;
        Q_INVOKABLE QString description(const QString &media) override;
        Q_INVOKABLE AkCaps caps(int stream) override;
        Q_INVOKABLE QRgb color() const;
        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE QString summary() const;
        Q_INVOKABLE static QString devicePrefix();

    private:
        ColorSrcElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void colorChanged(QRgb color);
        void fpsChanged(const AkFrac &fps);
        void summaryChanged(const QString &summary);
        void error(const QString &message);

    public slots:
        void setColor(QRgb color);
        void setFps(const AkFrac &fps);
        void resetColor();
        void resetFps();
        void removeSettings();
        void setMedia(const QString &media) override;
        void resetMedia() override;
        bool setState(AkElement::ElementState state) override;
};

#endif // COLORSRCELEMENT_H
