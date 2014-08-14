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

#ifndef STREAKELEMENT_H
#define STREAKELEMENT_H

#include <QImage>
#include <qb.h>

class StreakElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(int planes READ planes WRITE setPlanes RESET resetPlanes)
    Q_PROPERTY(int stride READ stride WRITE setStride RESET resetStride)
    Q_PROPERTY(quint32 strideMask READ strideMask WRITE setStrideMask RESET resetStrideMask)
    Q_PROPERTY(int strideShift READ strideShift WRITE setStrideShift RESET resetStrideShift)

    public:
        explicit StreakElement();

        Q_INVOKABLE int planes() const;
        Q_INVOKABLE int stride() const;
        Q_INVOKABLE quint32 strideMask() const;
        Q_INVOKABLE int strideShift() const;

    private:
        int m_planes;
        int m_stride;
        quint32 m_strideMask;
        int m_strideShift;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_buffer;
        QVector<quint32 *> m_planetable;
        int m_plane;

    public slots:
        void setPlanes(int planes);
        void setStride(int stride);
        void setStrideMask(quint32 strideMask);
        void setStrideShift(int strideShift);
        void resetPlanes();
        void resetStride();
        void resetStrideMask();
        void resetStrideShift();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // STREAKELEMENT_H
