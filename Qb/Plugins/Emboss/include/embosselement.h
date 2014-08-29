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

#ifndef EmbossELEMENT_H
#define EmbossELEMENT_H

#include <QImage>
#include <qb.h>

class EmbossElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float azimuth READ azimuth WRITE setAzimuth RESET resetAzimuth)
    Q_PROPERTY(float elevation READ elevation WRITE setElevation RESET resetElevation)
    Q_PROPERTY(float width45 READ width45 WRITE setWidth45 RESET resetWidth45)
    Q_PROPERTY(float pixelScale READ pixelScale WRITE setPixelScale RESET resetPixelScale)

    public:
        explicit EmbossElement();
        Q_INVOKABLE float azimuth() const;
        Q_INVOKABLE float elevation() const;
        Q_INVOKABLE float width45() const;
        Q_INVOKABLE float pixelScale() const;

    private:
        float m_azimuth;
        float m_elevation;
        float m_width45;
        float m_pixelScale;

        QbElementPtr m_convert;
        QbCaps m_caps;

    public slots:
        void setAzimuth(float azimuth);
        void setElevation(float elevation);
        void setWidth45(float width45);
        void setPixelScale(float pixelScale);
        void resetAzimuth();
        void resetElevation();
        void resetWidth45();
        void resetPixelScale();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // EmbossELEMENT_H
