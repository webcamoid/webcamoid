/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
    Q_PROPERTY(int levels
               READ levels
               WRITE setLevels
               RESET resetLevels
               NOTIFY levelsChange)

    public:
        explicit CartoonElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int levels() const;

    private:
        int m_threshold;
        int m_levels;

        inline int threshold(int color, int levels)
        {
            if (levels < 1)
                levels = 1;

            double k = 256. / levels;
            int r = k * int(color / k + 0.5);

            return qBound(0, r, 255);
        }

    signals:
        void thresholdChange(int threshold);
        void levelsChange(int levels);

    public slots:
        void setThreshold(int threshold);
        void setLevels(int levels);
        void resetThreshold();
        void resetLevels();
        QbPacket iStream(const QbPacket &packet);
};

#endif // CARTOONELEMENT_H
