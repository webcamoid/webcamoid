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
    Q_PROPERTY(int lightning
               READ lightning
               WRITE setLightning
               RESET resetLightning
               NOTIFY lightningChanged)
    Q_PROPERTY(qreal slope
               READ slope
               WRITE setSlope
               RESET resetSlope
               NOTIFY slopeChanged)
    Q_PROPERTY(qreal interception
               READ interception
               WRITE setInterception
               RESET resetInterception
               NOTIFY interceptionChanged)

    public:
        HalftoneElement();
        ~HalftoneElement();

        Q_INVOKABLE QString pattern() const;
        Q_INVOKABLE QSize patternSize() const;
        Q_INVOKABLE int lightning() const;
        Q_INVOKABLE qreal slope() const;
        Q_INVOKABLE qreal interception() const;

    private:
        HalftoneElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void patternChanged(const QString &pattern);
        void patternSizeChanged(const QSize &patternSize);
        void lightningChanged(int lightning);
        void slopeChanged(qreal slope);
        void interceptionChanged(qreal interception);

    public slots:
        void setPattern(const QString &pattern);
        void setPatternSize(const QSize &patternSize);
        void setLightning(int lightning);
        void setSlope(qreal slope);
        void setInterception(qreal interception);
        void resetPattern();
        void resetPatternSize();
        void resetLightning();
        void resetSlope();
        void resetInterception();
};

#endif // HALFTONEELEMENT_H
