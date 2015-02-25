/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef WAVEELEMENT_H
#define WAVEELEMENT_H

#include <cmath>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class WaveElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(qreal phases
               READ phases
               WRITE setPhases
               RESET resetPhases
               NOTIFY phasesChanged)
    Q_PROPERTY(QRgb background
               READ background
               WRITE setBackground
               RESET resetBackground
               NOTIFY backgroundChanged)

    public:
        explicit WaveElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE qreal amplitude() const;
        Q_INVOKABLE qreal phases() const;
        Q_INVOKABLE QRgb background() const;

    private:
        qreal m_amplitude;
        qreal m_phases;
        QRgb m_background;
        QbElementPtr m_convert;

        inline uint interpolateBackground(const QImage &img, qreal xOffset, qreal yOffset, QRgb background) const
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

    signals:
        void amplitudeChanged();
        void phasesChanged();
        void backgroundChanged();

    public slots:
        void setAmplitude(qreal amplitude);
        void setPhases(qreal phases);
        void setBackground(QRgb background);
        void resetAmplitude();
        void resetPhases();
        void resetBackground();
        QbPacket iStream(const QbPacket &packet);
};

#endif // WAVEELEMENT_H
