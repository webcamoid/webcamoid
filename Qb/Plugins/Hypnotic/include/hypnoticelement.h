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

#ifndef HYPNOTICELEMENT_H
#define HYPNOTICELEMENT_H

#include <QImage>
#include <QColor>

#include <qb.h>

typedef QMap<QString, QImage> OpticalMap;

class HypnoticElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(int speedInc READ speedInc WRITE setSpeedInc RESET resetSpeedInc)
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold RESET resetThreshold)

    public:
        explicit HypnoticElement();
        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int speedInc() const;
        Q_INVOKABLE int threshold() const;

    private:
        QString m_mode;
        int m_speedInc;
        int m_threshold;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QVector<QRgb> m_palette;
        OpticalMap m_opticalMap;
        quint8 m_speed;
        quint8 m_phase;

        QVector<QRgb> createPalette();
        OpticalMap createOpticalMap(int width, int height);
        QImage imageYOver(const QImage &src, int threshold);

    public slots:
        void setMode(const QString &mode);
        void setSpeedInc(int speedInc);
        void setThreshold(int threshold);
        void resetMode();
        void resetSpeedInc();
        void resetThreshold();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // HYPNOTICELEMENT_H
