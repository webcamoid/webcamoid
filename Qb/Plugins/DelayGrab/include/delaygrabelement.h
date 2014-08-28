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

#ifndef DELAYGRABELEMENT_H
#define DELAYGRABELEMENT_H

#include <QImage>
#include <qb.h>

class DelayGrabElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(DelayGrabMode)
    Q_PROPERTY(QString mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(int blockSize READ blockSize WRITE setBlockSize RESET resetBlockSize)
    Q_PROPERTY(int nFrames READ nFrames WRITE setNFrames RESET resetNFrames)

    public:
        enum DelayGrabMode
        {
            // Random delay with square distribution
            DelayGrabModeRandomSquare,
            // Vertical stripes of increasing delay outward from center
            DelayGrabModeVerticalIncrease,
            // Horizontal stripes of increasing delay outward from center
            DelayGrabModeHorizontalIncrease,
            // Rings of increasing delay outward from center
            DelayGrabModeRingsIncrease
        };

        explicit DelayGrabElement();
        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int blockSize() const;
        Q_INVOKABLE int nFrames() const;

    private:
        DelayGrabMode m_mode;
        int m_blockSize;
        int m_nFrames;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QMap<DelayGrabMode, QString> m_modeToStr;
        QVector<QImage> m_frames;
        int m_delayMapWidth;
        int m_delayMapHeight;
        QVector<int> m_delayMap;

        QVector<int> createDelaymap(DelayGrabMode mode);

    public slots:
        void setMode(const QString &mode);
        void setBlockSize(int blockSize);
        void setNFrames(int nFrames);
        void resetMode();
        void resetBlockSize();
        void resetNFrames();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // DELAYGRABELEMENT_H
