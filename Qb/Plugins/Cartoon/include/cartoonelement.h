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

#ifndef CARTOONELEMENT_H
#define CARTOONELEMENT_H

#include <cmath>
#include <QColor>
#include <QQmlComponent>
#include <QQmlContext>

#include <qb.h>
#include <qbutils.h>

class CartoonElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChange)
    Q_PROPERTY(int diffSpace
               READ diffSpace
               WRITE setDiffSpace
               RESET resetDiffSpace
               NOTIFY diffSpaceChange)

    public:
        explicit CartoonElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int diffSpace() const;

    private:
        int m_threshold;
        int m_diffSpace;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QVector<int> m_yprecal;
        int m_width;
        int m_height;

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

        int getMaxContrast(const QRgb *src, int x, int y);

    signals:
        void thresholdChange();
        void diffSpaceChange();

    public slots:
        void setThreshold(int threshold);
        void setDiffSpace(int diffSpace);
        void resetThreshold();
        void resetDiffSpace();
        QbPacket iStream(const QbPacket &packet);
};

#endif // CARTOONELEMENT_H
