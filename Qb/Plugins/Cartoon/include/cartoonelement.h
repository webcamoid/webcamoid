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

#ifndef CARTOONELEMENT_H
#define CARTOONELEMENT_H

#include <QImage>
#include <QColor>

#include <qb.h>

class CartoonElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float triplevel READ triplevel WRITE setTriplevel RESET resetTriplevel)
    Q_PROPERTY(float diffspace READ diffspace WRITE setDiffspace RESET resetDiffspace)

    public:
        explicit CartoonElement();
        Q_INVOKABLE float triplevel() const;
        Q_INVOKABLE float diffspace() const;

    private:
        int m_triplevel;
        int m_diffspace;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QVector<int> m_yprecal;

        inline QRgb flattenColor(QRgb color)
        {
            int r = (qRed(color) >> 5) << 5;
            int g = (qGreen(color) >> 5) << 5;
            int b = (qBlue(color) >> 5) << 5;

            return qRgba(r, g, b, qAlpha(color));
        }

        inline QRgb pixelate(const QRgb *src, int x, int y)
        {
            return src[x + this->m_yprecal[y]];
        }

        inline QRgb gmError(QRgb color1, QRgb color2)
        {
            int rDiff = qRed(color1) - qRed(color2);
            int gDiff = qGreen(color1) - qGreen(color2);
            int bDiff = qBlue(color1) - qBlue(color2);

            return rDiff * rDiff + gDiff * gDiff + bDiff * bDiff;
        }

        long getMaxContrast(const QRgb *src, int x, int y);

    public slots:
        void setTriplevel(float triplevel);
        void setDiffspace(float diffspace);
        void resetTriplevel();
        void resetDiffspace();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // CARTOONELEMENT_H
