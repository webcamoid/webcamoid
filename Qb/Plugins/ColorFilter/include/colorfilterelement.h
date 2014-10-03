/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef COLORFILTERELEMENT_H
#define COLORFILTERELEMENT_H

#include <cmath>
#include <qrgb.h>

#include <qb.h>
#include <qbutils.h>

class ColorFilterElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb colorf READ color WRITE setColor RESET resetColor)
    Q_PROPERTY(float radius READ radius WRITE setRadius RESET resetRadius)
    Q_PROPERTY(bool gradient READ gradient WRITE setGradient RESET resetGradient)

    public:
        explicit ColorFilterElement();

        Q_INVOKABLE QRgb color() const;
        Q_INVOKABLE float radius() const;
        Q_INVOKABLE bool gradient() const;

    private:
        QRgb m_color;
        float m_radius;
        bool m_gradient;

        QbElementPtr m_convert;

    public slots:
        void setColor(QRgb color);
        void setRadius(float radius);
        void setGradient(bool gradient);
        void resetColor();
        void resetRadius();
        void resetGradient();
        QbPacket iStream(const QbPacket &packet);
};

#endif // COLORFILTERELEMENT_H
