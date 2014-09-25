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

#ifndef COLORREPLACEELEMENT_H
#define COLORREPLACEELEMENT_H

#include <cmath>
#include <QImage>
#include <qrgb.h>

#include <qb.h>

class ColorReplaceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb from READ from WRITE setFrom RESET resetFrom)
    Q_PROPERTY(QRgb to READ to WRITE setTo RESET resetTo)
    Q_PROPERTY(float radius READ radius WRITE setRadius RESET resetRadius)

    public:
        explicit ColorReplaceElement();

        Q_INVOKABLE QRgb from() const;
        Q_INVOKABLE QRgb to() const;
        Q_INVOKABLE float radius() const;

    private:
        QRgb m_from;
        QRgb m_to;
        float m_radius;

        QbElementPtr m_convert;

    public slots:
        void setFrom(QRgb from);
        void setTo(QRgb to);
        void setRadius(float radius);
        void resetFrom();
        void resetTo();
        void resetRadius();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // COLORREPLACEELEMENT_H
