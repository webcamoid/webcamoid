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

#ifndef VIGNETTEELEMENT_H
#define VIGNETTEELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class VignetteElementPrivate;

class VignetteElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb color
               READ color
               WRITE setColor
               RESET resetColor
               NOTIFY colorChanged)
    Q_PROPERTY(qreal aspect
               READ aspect
               WRITE setAspect
               RESET resetAspect
               NOTIFY aspectChanged)
    Q_PROPERTY(qreal scale
               READ scale
               WRITE setScale
               RESET resetScale
               NOTIFY scaleChanged)
    Q_PROPERTY(qreal softness
               READ softness
               WRITE setSoftness
               RESET resetSoftness
               NOTIFY softnessChanged)

    public:
        VignetteElement();
        ~VignetteElement();

        Q_INVOKABLE QRgb color() const;
        Q_INVOKABLE qreal aspect() const;
        Q_INVOKABLE qreal scale() const;
        Q_INVOKABLE qreal softness() const;

    private:
        VignetteElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void colorChanged(QRgb color);
        void aspectChanged(qreal aspect);
        void scaleChanged(qreal scale);
        void softnessChanged(qreal softness);

    public slots:
        void setColor(QRgb color);
        void setAspect(qreal aspect);
        void setScale(qreal scale);
        void setSoftness(qreal softness);
        void resetColor();
        void resetAspect();
        void resetScale();
        void resetSoftness();
};

#endif // VIGNETTEELEMENT_H
