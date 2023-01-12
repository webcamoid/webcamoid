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

#ifndef ANALOGTVELEMENT_H
#define ANALOGTVELEMENT_H

#include <akelement.h>

class AnalogTVElementPrivate;

class AnalogTVElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal vsync
               READ vsync
               WRITE setVSync
               RESET resetVSync
               NOTIFY vsyncChanged)
    Q_PROPERTY(int xOffset
               READ xOffset
               WRITE setXOffset
               RESET resetXOffset
               NOTIFY xOffsetChanged)
    Q_PROPERTY(qreal hsyncFactor
               READ hsyncFactor
               WRITE setHSyncFactor
               RESET resetHSyncFactor
               NOTIFY hsyncFactorChanged)
    Q_PROPERTY(int hsyncSmoothness
               READ hsyncSmoothness
               WRITE setHSyncSmoothness
               RESET resetHSyncSmoothness
               NOTIFY hsyncSmoothnessChanged)
    Q_PROPERTY(qreal hueFactor
               READ hueFactor
               WRITE setHueFactor
               RESET resetHueFactor
               NOTIFY hueFactorChanged)
    Q_PROPERTY(qreal noise
               READ noise
               WRITE setNoise
               RESET resetNoise
               NOTIFY noiseChanged)

    public:
        AnalogTVElement();
        ~AnalogTVElement();

        Q_INVOKABLE qreal vsync() const;
        Q_INVOKABLE int xOffset() const;
        Q_INVOKABLE qreal hsyncFactor() const;
        Q_INVOKABLE int hsyncSmoothness() const;
        Q_INVOKABLE qreal hueFactor() const;
        Q_INVOKABLE qreal noise() const;

    private:
        AnalogTVElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void vsyncChanged(qreal vsync);
        void xOffsetChanged(int xOffset);
        void hsyncFactorChanged(qreal hsyncFactor);
        void hsyncSmoothnessChanged(int hsyncSmoothness);
        void hueFactorChanged(qreal hueFactor);
        void noiseChanged(qreal noise);

    public slots:
        void setVSync(qreal vsync);
        void setXOffset(int xOffset);
        void setHSyncFactor(qreal hsyncFactor);
        void setHSyncSmoothness(int hsyncSmoothness);
        void setHueFactor(qreal hueFactor);
        void setNoise(qreal noise);
        void resetVSync();
        void resetXOffset();
        void resetHSyncFactor();
        void resetHSyncSmoothness();
        void resetHueFactor();
        void resetNoise();
};

#endif // ANALOGTVELEMENT_H
