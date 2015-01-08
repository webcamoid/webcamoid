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

#ifndef PHOTOCOPYELEMENT_H
#define PHOTOCOPYELEMENT_H

#include <cmath>
#include <qrgb.h>
#include <qb.h>
#include <qbutils.h>

class PhotocopyElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness RESET resetBrightness)
    Q_PROPERTY(qreal sharpness READ sharpness WRITE setSharpness RESET resetSharpness)
    Q_PROPERTY(int sigmoidalBase READ sigmoidalBase WRITE setSigmoidalBase RESET resetSigmoidalBase)
    Q_PROPERTY(int sigmoidalRange READ sigmoidalRange WRITE setSigmoidalRange RESET resetSigmoidalRange)

    public:
        explicit PhotocopyElement();

        Q_INVOKABLE qreal brightness() const;
        Q_INVOKABLE qreal sharpness() const;
        Q_INVOKABLE int sigmoidalBase() const;
        Q_INVOKABLE int sigmoidalRange() const;

    private:
        qreal m_brightness;
        qreal m_sharpness;
        int m_sigmoidalBase;
        int m_sigmoidalRange;

        QbElementPtr m_convert;

        inline int rgbToLuma(int red, int green, int blue)
        {
            int min;
            int max;

            if (red > green) {
                max = qMax(red, blue);
                min = qMin(green, blue);
            }
            else {
                max = qMax(green, blue);
                min = qMin(red, blue);
            }

            return qRound((max + min) / 2.0);
        }

    public slots:
        void setBrightness(qreal brightness);
        void setSharpness(qreal sharpness);
        void setSigmoidalBase(int sigmoidalBase);
        void setSigmoidalRange(int sigmoidalRange);
        void resetBrightness();
        void resetSharpness();
        void resetSigmoidalBase();
        void resetSigmoidalRange();
        QbPacket iStream(const QbPacket &packet);
};

#endif // PHOTOCOPYELEMENT_H
