/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef HYPNOTICELEMENT_H
#define HYPNOTICELEMENT_H

#include <cmath>
#include <QColor>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

typedef QMap<QString, QImage> OpticalMap;

class HypnoticElement: public QbElement
{
    Q_OBJECT
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
        explicit HypnoticElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int speedInc() const;
        Q_INVOKABLE int threshold() const;

    private:
        QString m_mode;
        int m_speedInc;
        int m_threshold;

        QbElementPtr m_convert;
        QVector<QRgb> m_palette;
        OpticalMap m_opticalMap;
        quint8 m_speed;
        quint8 m_phase;

        QVector<QRgb> createPalette();
        OpticalMap createOpticalMap(int width, int height);
        QImage imageYOver(const QImage &src, int threshold);

    signals:
        void modeChanged();
        void speedIncChanged();
        void thresholdChanged();

    public slots:
        void setMode(const QString &mode);
        void setSpeedInc(int speedInc);
        void setThreshold(int threshold);
        void resetMode();
        void resetSpeedInc();
        void resetThreshold();
        QbPacket iStream(const QbPacket &packet);
};

#endif // HYPNOTICELEMENT_H
