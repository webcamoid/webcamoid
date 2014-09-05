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

#ifndef DIZZYELEMENT_H
#define DIZZYELEMENT_H

#include <QImage>
#include <QColor>

#include <qb.h>

class DizzyElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float phaseIncrement READ phaseIncrement
                                    WRITE setPhaseIncrement
                                    RESET resetPhaseIncrement)

    Q_PROPERTY(float zoomRate READ zoomRate WRITE setZoomRate RESET resetZoomRate)

    public:
        explicit DizzyElement();
        Q_INVOKABLE float phaseIncrement() const;
        Q_INVOKABLE float zoomRate() const;

    private:
        float m_phaseIncrement;
        float m_zoomRate;

        QbElementPtr m_convert;
        QImage m_prevFrame;
        QbCaps m_caps;
        float m_phase;

        void setParams(int *dx, int *dy,
                       int *sx, int *sy,
                       int width, int height,
                       float phase, float zoomRate);

    public slots:
        void setPhaseIncrement(float phaseIncrement);
        void setZoomRate(float zoomRate);
        void resetPhaseIncrement();
        void resetZoomRate();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // DIZZYELEMENT_H
