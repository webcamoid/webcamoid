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

#ifndef SHAGADELICELEMENT_H
#define SHAGADELICELEMENT_H

#include <QImage>
#include <qb.h>

class ShagadelicElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int mask READ mask WRITE setMask RESET resetMask)

    public:
        explicit ShagadelicElement();
        Q_INVOKABLE int mask() const;

    private:
        int m_mask;
        int m_rx;
        int m_ry;
        int m_bx;
        int m_by;
        int m_rvx;
        int m_rvy;
        int m_bvx;
        int m_bvy;
        uchar m_phase;

        QByteArray m_ripple;
        QByteArray m_spiral;

        QSize m_curSize;
        QbElementPtr m_convert;

        QByteArray makeRipple(const QSize &size) const;
        QByteArray makeSpiral(const QSize &size) const;
        void init(const QSize &size);

    public slots:
        void setMask(int mask);
        void resetMask();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // SHAGADELICELEMENT_H
