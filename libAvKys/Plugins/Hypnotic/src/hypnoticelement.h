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

#ifndef HYPNOTICELEMENT_H
#define HYPNOTICELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

class HypnoticElement: public AkElement
{
    Q_OBJECT
    Q_ENUMS(OpticMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int speedInc
               READ speedInc
               WRITE setSpeedInc
               RESET resetSpeedInc
               NOTIFY speedIncChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)

    public:
        enum OpticMode
        {
            OpticModeSpiral1,
            OpticModeSpiral2,
            OpticModeParabola,
            OpticModeHorizontalStripe
        };
        typedef QMap<OpticMode, QImage> OpticalMap;

        explicit HypnoticElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int speedInc() const;
        Q_INVOKABLE int threshold() const;

    private:
        OpticMode m_mode;
        int m_speedInc;
        int m_threshold;

        QSize m_frameSize;
        QVector<QRgb> m_palette;
        OpticalMap m_opticalMap;
        quint8 m_speed;
        quint8 m_phase;

        QVector<QRgb> createPalette();
        OpticalMap createOpticalMap(const QSize &size);
        QImage imageThreshold(const QImage &src, int threshold);

    signals:
        void modeChanged(const QString &mode);
        void speedIncChanged(int speedInc);
        void thresholdChanged(int threshold);

    public slots:
        void setMode(const QString &mode);
        void setSpeedInc(int speedInc);
        void setThreshold(int threshold);
        void resetMode();
        void resetSpeedInc();
        void resetThreshold();
        AkPacket iStream(const AkPacket &packet);
};

#endif // HYPNOTICELEMENT_H
