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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef TEMPERATUREELEMENT_H
#define TEMPERATUREELEMENT_H

#include <cmath>
#include <QColor>
#include <qb.h>
#include <qbutils.h>

class TemperatureElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal temperature READ temperature WRITE setTemperature RESET resetTemperature)

    public:
        explicit TemperatureElement();

        Q_INVOKABLE qreal temperature() const;

    private:
        qreal m_temperature;

        QbElementPtr m_convert;
        qreal m_kr;
        qreal m_kg;
        qreal m_kb;

        inline void colorFromTemperature(qreal temperature, qreal *r, qreal *g, qreal *b)
        {
            // This algorithm was taken from here:
            // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

            // Temperature must fall between 1000 and 40000 degrees
            temperature = qBound(1000.0, (double) temperature, 40000.0);

            // All calculations require temperature / 100, so only do the conversion once
            temperature /= 100.0;

            if (temperature <= 66.0)
                *r = 1;
            else
                *r = 1.2929362 * pow(temperature - 60.0, -0.1332047592);

            if (temperature <= 66.0)
                *g = 0.39008158 * log(temperature) - 0.63184144;
            else
                *g = 1.1298909 * pow(temperature - 60, -0.0755148492);

            if (temperature >= 66)
                *b = 1;
            else if (temperature <= 19)
                *b = 0;
            else
                *b = 0.54320679 * log(temperature - 10) - 1.1962541;
        }

    public slots:
        void setTemperature(qreal temperature);
        void resetTemperature();
        QbPacket iStream(const QbPacket &packet);
};

#endif // TEMPERATUREELEMENT_H
