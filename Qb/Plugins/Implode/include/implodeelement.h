/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#ifndef IMPLODEELEMENT_H
#define IMPLODEELEMENT_H

#include <QtGui>
#include <qb.h>

class ImplodeElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float amount READ amount WRITE setAmount RESET resetAmount)

    public:
        explicit ImplodeElement();
        Q_INVOKABLE float amount() const;

    private:
        float m_amount;
        QbElementPtr m_convert;

        QImage implode(const QImage &img, float amount) const;

        inline uint interpolate(const QImage &img, float xOffset, float yOffset) const
        {
            int width = img.width();
            int height = img.height();
            int x = qBound(0, (int) xOffset, width - 2);
            int y = qBound(0, (int) yOffset, height - 2);

            QRgb *ptr = (QRgb *) img.bits();

            uint p = *(ptr + (y * width) + x);
            uint q = *(ptr + (y * width) + x + 1);
            uint r = *(ptr + ((y + 1) * width) + x);
            uint s = *(ptr + ((y + 1) * width) + x + 1);

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
        void setAmount(float amount);
        void resetAmount();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // IMPLODEELEMENT_H
