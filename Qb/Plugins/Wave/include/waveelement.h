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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef WAVEELEMENT_H
#define WAVEELEMENT_H

#include <cmath>
#include <QImage>
#include <qb.h>

class WaveElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float amplitude READ amplitude WRITE setAmplitude RESET resetAmplitude)
    Q_PROPERTY(float phases READ phases WRITE setPhases RESET resetPhases)
    Q_PROPERTY(QRgb background READ background WRITE setBackground RESET resetBackground)

    public:
        explicit WaveElement();
        Q_INVOKABLE float amplitude() const;
        Q_INVOKABLE float phases() const;
        Q_INVOKABLE QRgb background() const;

    private:
        float m_amplitude;
        float m_phases;
        QRgb m_background;

        QbElementPtr m_convert;

        QImage wave(const QImage &img, float amplitude, float phases, QRgb background) const;

        inline uint interpolateBackground(const QImage &img, float xOffset, float yOffset, QRgb background) const
        {
            int width = img.width();
            int height = img.height();
            int x = xOffset;
            int y = yOffset;

            uint p = background;
            uint q = background;
            uint r = background;
            uint s = background;

            QRgb *ptr = (QRgb *) img.bits();

            if (y >= 0 && y < height && x >= 0 && x < width) {
                p = *(((QRgb *) ptr) + (y * width) + x);

                if (y + 1 < height)
                    r = *(((QRgb *) ptr) + ((y + 1) * width) + x);

                if (x + 1 < width) {
                    q = *(((QRgb *) ptr) + (y * width) + x + 1);

                    if (y + 1 < height)
                        q = *(((QRgb *) ptr) + ((y + 1) * width) + x + 1);
                }
            }

            xOffset -= floor(xOffset);
            yOffset -= floor(yOffset);
            uint alpha = (uint) (255 * xOffset);
            uint beta = (uint) (255 * yOffset);

            p = this->interpolate255(p, 255 - alpha, q, alpha);
            r = this->interpolate255(r, 255 - alpha, s, alpha);

            return this->interpolate255(p, 255 - beta, r, beta);
        }

        inline QRgb interpolate255(QRgb x, uint a, QRgb y, uint b) const
        {
            uint t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
            t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
            t &= 0xff00ff;

            x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
            x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
            x &= 0xff00ff00;
            x |= t;

            return x;
        }

    public slots:
        void setAmplitude(float amplitude);
        void setPhases(float phases);
        void setBackground(QRgb background);
        void resetAmplitude();
        void resetPhases();
        void resetBackground();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // WAVEELEMENT_H
