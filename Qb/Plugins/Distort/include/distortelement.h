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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef DISTORTELEMENT_H
#define DISTORTELEMENT_H

#include <cmath>
#include <qrgb.h>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class DistortElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(qreal frequency
               READ frequency
               WRITE setFrequency
               RESET resetFrequency
               NOTIFY frequencyChanged)
    Q_PROPERTY(int gridSizeLog
               READ gridSizeLog
               WRITE setGridSizeLog
               RESET resetGridSizeLog
               NOTIFY gridSizeLogChanged)

    public:
        explicit DistortElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE qreal amplitude() const;
        Q_INVOKABLE qreal frequency() const;
        Q_INVOKABLE int gridSizeLog() const;

    private:
        qreal m_amplitude;
        qreal m_frequency;
        int m_gridSizeLog;

        QbElementPtr m_convert;

        // this will compute a displacement value such that
        // 0<=x_retval<xsize and 0<=y_retval<ysize.
        inline QPoint plasmaFunction(const QPoint &point, const QSize &size,
                                     qreal amp, qreal freq, qreal t)
        {
            qreal time = fmod(t, 2 * M_PI);
            qreal h = size.height() - 1;
            qreal w = size.width() - 1;
            qreal dx = (-4.0 / (w * w) * point.x() + 4.0 / w) * point.x();
            qreal dy = (-4.0 / (h * h) * point.y() + 4.0 / h) * point.y();

            int x = point.x() + amp * (size.width() / 4.0) * dx
                    * sin(freq * point.y() / size.height() + time);

            int y = point.y() + amp * (size.height() / 4.0) * dy
                    * sin(freq * point.x() / size.width() + time);

            return QPoint(qBound(0, x, size.width() - 1),
                          qBound(0, y, size.height() - 1));
        }

        QVector<QPoint> createGrid(int width, int height,
                                   int gridSize, qreal time);

    signals:
        void amplitudeChanged();
        void frequencyChanged();
        void gridSizeLogChanged();

    public slots:
        void setAmplitude(qreal amplitude);
        void setFrequency(qreal frequency);
        void setGridSizeLog(int gridSizeLog);
        void resetAmplitude();
        void resetFrequency();
        void resetGridSizeLog();
        QbPacket iStream(const QbPacket &packet);
};

#endif // DISTORTELEMENT_H
