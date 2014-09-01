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

#ifndef BLURELEMENT_H
#define BLURELEMENT_H

#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QPainter>

#include <qb.h>

class BlurElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int radius READ radius WRITE setRadius RESET resetRadius)

    public:
        explicit BlurElement();
        Q_INVOKABLE int radius() const;

    private:
        int m_radius;
        QbElementPtr m_convert;

    public slots:
        void setRadius(int radius);
        void resetRadius();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // BLURELEMENT_H
