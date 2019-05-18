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

#ifndef HALFTONEELEMENT_H
#define HALFTONEELEMENT_H

#include <akelement.h>

class HalftoneElementPrivate;

class HalftoneElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString pattern
               READ pattern
               WRITE setPattern
               RESET resetPattern
               NOTIFY patternChanged)
    Q_PROPERTY(QSize patternSize
               READ patternSize
               WRITE setPatternSize
               RESET resetPatternSize
               NOTIFY patternSizeChanged)
    Q_PROPERTY(qreal lightness
               READ lightness
               WRITE setLightness
               RESET resetLightness
               NOTIFY lightnessChanged)
    Q_PROPERTY(qreal slope
               READ slope
               WRITE setSlope
               RESET resetSlope
               NOTIFY slopeChanged)
    Q_PROPERTY(qreal intercept
               READ intercept
               WRITE setIntercept
               RESET resetIntercept
               NOTIFY interceptChanged)

    public:
        HalftoneElement();
        ~HalftoneElement();

        Q_INVOKABLE QString pattern() const;
        Q_INVOKABLE QSize patternSize() const;
        Q_INVOKABLE qreal lightness() const;
        Q_INVOKABLE qreal slope() const;
        Q_INVOKABLE qreal intercept() const;

    private:
        HalftoneElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;
        AkPacket iVideoStream(const AkVideoPacket &packet);

    signals:
        void patternChanged(const QString &pattern);
        void patternSizeChanged(const QSize &patternSize);
        void lightnessChanged(qreal lightness);
        void slopeChanged(qreal slope);
        void interceptChanged(qreal intercept);

    public slots:
        void setPattern(const QString &pattern);
        void setPatternSize(const QSize &patternSize);
        void setLightness(qreal lightness);
        void setSlope(qreal slope);
        void setIntercept(qreal intercept);
        void resetPattern();
        void resetPatternSize();
        void resetLightness();
        void resetSlope();
        void resetIntercept();
};

#endif // HALFTONEELEMENT_H
