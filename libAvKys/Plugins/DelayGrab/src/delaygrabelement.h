/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef DELAYGRABELEMENT_H
#define DELAYGRABELEMENT_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

class DelayGrabElement: public AkElement
{
    Q_OBJECT
    Q_ENUMS(DelayGrabMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int blockSize
               READ blockSize
               WRITE setBlockSize
               RESET resetBlockSize
               NOTIFY blockSizeChanged)
    Q_PROPERTY(int nFrames
               READ nFrames
               WRITE setNFrames
               RESET resetNFrames
               NOTIFY nFramesChanged)

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

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int blockSize() const;
        Q_INVOKABLE int nFrames() const;

    private:
        DelayGrabMode m_mode;
        int m_blockSize;
        int m_nFrames;

        QMutex m_mutex;
        QSize m_frameSize;
        QVector<QImage> m_frames;
        QVector<int> m_delayMap;

    signals:
        void modeChanged(const QString &mode);
        void blockSizeChanged(int blockSize);
        void nFramesChanged(int nFrames);
        void frameSizeChanged(const QSize &frameSize);

    public slots:
        void setMode(const QString &mode);
        void setBlockSize(int blockSize);
        void setNFrames(int nFrames);
        void resetMode();
        void resetBlockSize();
        void resetNFrames();
        AkPacket iStream(const AkPacket &packet);

    private slots:
        void updateDelaymap();
};

#endif // DELAYGRABELEMENT_H
