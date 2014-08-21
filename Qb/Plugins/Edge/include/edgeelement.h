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

#ifndef EDGEELEMENT_H
#define EDGEELEMENT_H

#include <QImage>
#include <qb.h>

class EdgeElement: public QbElement
{
    Q_OBJECT

    public:
        explicit EdgeElement();

    private:
        QbElementPtr m_convert;

        QImage convolve(const QImage &src) const;

        inline void sobel(const QRgb *src, const int *kernel,
                          int x, int y, int width, int height,
                          int *r, int *g, int *b) const
        {
            *r = 0;
            *g = 0;
            *b = 0;

            QRect rect(0, 0, width, height);

            for (int j = -1; j < 2; j++) {
                int yp = y + j;

                for (int i = -1; i < 2; i++) {
                    int k = *kernel++;
                    int xp = x + i;

                    if (k && rect.contains(xp, yp)) {
                        QRgb pixel = src[xp + yp * width];

                        *r += k * qRed(pixel);
                        *g += k * qGreen(pixel);
                        *b += k * qBlue(pixel);
                    }
                }
            }
        }

    public slots:
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // EDGEELEMENT_H
