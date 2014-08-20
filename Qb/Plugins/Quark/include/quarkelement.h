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

#ifndef QUARKELEMENT_H
#define QUARKELEMENT_H

#include <QImage>
#include <qb.h>

class QuarkElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int planes READ planes WRITE setPlanes RESET resetPlanes)

    public:
        explicit QuarkElement();

        Q_INVOKABLE int planes() const;

    private:
        int m_planes;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_buffer;
        int m_plane;
        QVector<QRgb *> m_planeTable;

    public slots:
        void setPlanes(int planes);
        void resetPlanes();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // QUARKELEMENT_H
