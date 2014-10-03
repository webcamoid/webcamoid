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

#ifndef EmbossELEMENT_H
#define EmbossELEMENT_H

#include <cmath>
#include <qb.h>
#include <qbutils.h>

class EmbossElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float factor READ factor WRITE setFactor RESET resetFactor)
    Q_PROPERTY(float bias READ bias WRITE setBias RESET resetBias)

    public:
        explicit EmbossElement();
        Q_INVOKABLE float factor() const;
        Q_INVOKABLE float bias() const;

    private:
        float m_factor;
        float m_bias;

        QbElementPtr m_convert;

    public slots:
        void setFactor(float factor);
        void setBias(float bias);
        void resetFactor();
        void resetBias();
        QbPacket iStream(const QbPacket &packet);
};

#endif // EmbossELEMENT_H
