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

#ifndef LIFEELEMENT_H
#define LIFEELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class LifeElementPrivate;

class LifeElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb lifeColor
               READ lifeColor
               WRITE setLifeColor
               RESET resetLifeColor
               NOTIFY lifeColorChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)
    Q_PROPERTY(int lumaThreshold
               READ lumaThreshold
               WRITE setLumaThreshold
               RESET resetLumaThreshold
               NOTIFY lumaThresholdChanged)

    public:
        LifeElement();
        ~LifeElement();

        Q_INVOKABLE QRgb lifeColor() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;

    private:
        LifeElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void lifeColorChanged(QRgb lifeColor);
        void thresholdChanged(int threshold);
        void lumaThresholdChanged(int lumaThreshold);

    public slots:
        void setLifeColor(QRgb lifeColor);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void resetLifeColor();
        void resetThreshold();
        void resetLumaThreshold();
};

#endif // LIFEELEMENT_H
