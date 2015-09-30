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

#ifndef DIZZYELEMENT_H
#define DIZZYELEMENT_H

#include <cmath>
#include <QColor>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class DizzyElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal phaseIncrement
               READ phaseIncrement
               WRITE setPhaseIncrement
               RESET resetPhaseIncrement
               NOTIFY phaseIncrementChanged)
    Q_PROPERTY(qreal zoomRate
               READ zoomRate
               WRITE setZoomRate
               RESET resetZoomRate
               NOTIFY zoomRateChanged)

    public:
        explicit DizzyElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE qreal phaseIncrement() const;
        Q_INVOKABLE qreal zoomRate() const;

    private:
        qreal m_phaseIncrement;
        qreal m_zoomRate;

        QbElementPtr m_convert;
        QImage m_prevFrame;
        QbCaps m_caps;
        qreal m_phase;

        void setParams(int *dx, int *dy,
                       int *sx, int *sy,
                       int width, int height,
                       qreal phase, qreal zoomRate);

    signals:
        void phaseIncrementChanged();
        void zoomRateChanged();

    public slots:
        void setPhaseIncrement(qreal phaseIncrement);
        void setZoomRate(qreal zoomRate);
        void resetPhaseIncrement();
        void resetZoomRate();
        QbPacket iStream(const QbPacket &packet);
};

#endif // DIZZYELEMENT_H
