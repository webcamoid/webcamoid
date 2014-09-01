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

#ifndef FRAMEOVERLAPELEMENT_H
#define FRAMEOVERLAPELEMENT_H

#include <QImage>
#include <QColor>

#include <qb.h>

class FrameOverlapElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int nFrames READ nFrames WRITE setNFrames RESET resetNFrames)
    Q_PROPERTY(int stride READ stride WRITE setStride RESET resetStride)

    public:
        explicit FrameOverlapElement();

        Q_INVOKABLE int nFrames() const;
        Q_INVOKABLE int stride() const;

    private:
        int m_nFrames;
        int m_stride;

        QbElementPtr m_convert;
        QVector<QImage> m_frames;
        QbCaps m_caps;

    public slots:
        void setNFrames(int nFrames);
        void setStride(int stride);
        void resetNFrames();
        void resetStride();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // FRAMEOVERLAPELEMENT_H
