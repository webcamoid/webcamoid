/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef RIPPLEELEMENT_H
#define RIPPLEELEMENT_H

#include <cmath>
#include <QQmlComponent>
#include <QQmlContext>
#include <QColor>
#include <qb.h>
#include <qbutils.h>

class RippleElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(RippleMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(int decay
               READ decay
               WRITE setDecay
               RESET resetDecay
               NOTIFY decayChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)
    Q_PROPERTY(int lumaThreshold
               READ lumaThreshold
               WRITE setLumaThreshold
               RESET resetLumaThreshold
               NOTIFY lumaThresholdChanged)

    public:
        enum RippleMode
        {
            RippleModeMotionDetect,
            RippleModeRain
        };

        explicit RippleElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int amplitude() const;
        Q_INVOKABLE int decay() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;

    private:
        RippleMode m_mode;
        int m_amplitude;
        int m_decay;
        int m_threshold;
        int m_lumaThreshold;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_prevFrame;
        QVector<QImage> m_rippleBuffer;
        int m_curRippleBuffer;
        QMap<RippleMode, QString> m_rippleModeToStr;

        int m_period;
        int m_rainStat;
        unsigned int m_dropProb;
        int m_dropProbIncrement;
        int m_dropsPerFrameMax;
        int m_dropsPerFrame;
        int m_dropPower;

        QImage imageDiff(const QImage &img1,
                         const QImage &img2,
                         int threshold,
                         int lumaThreshold, int strength);

        void addDrops(const QImage &buffer, const QImage &drops);
        void ripple(const QImage &buffer1, const QImage &buffer2, int decay);
        QImage applyWater(const QImage &src, const QImage &buffer);
        QImage rainDrop(int width, int height, int strength);

        inline QImage drop(int width, int height, int power)
        {
            QImage drops(width, height, QImage::Format_ARGB32);
            int *dropsBits = (int *) drops.bits();

            drops.fill(qRgba(0, 0, 0, 0));

            int widthM1 = width - 1;
            int widthP1 = width + 1;

            int x = qrand() % (width - 4) + 2;
            int y = qrand() % (height - 4) + 2;

            int offset = x + y * width;

            dropsBits[offset - widthP1] = power >> 2;
            dropsBits[offset - width] = power >> 1;
            dropsBits[offset - widthM1] = power >> 2;
            dropsBits[offset - 1] = power >> 1;
            dropsBits[offset] = power;
            dropsBits[offset + 1] = power >> 1;
            dropsBits[offset + widthM1] = power >> 2;
            dropsBits[offset + width] = power >> 1;
            dropsBits[offset + widthP1] = power >> 2;

            return drops;
        }

    signals:
        void modeChanged();
        void amplitudeChanged();
        void decayChanged();
        void thresholdChanged();
        void lumaThresholdChanged();

    public slots:
        void setMode(const QString &mode);
        void setAmplitude(int amplitude);
        void setDecay(int decay);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void resetMode();
        void resetAmplitude();
        void resetDecay();
        void resetThreshold();
        void resetLumaThreshold();

        QbPacket iStream(const QbPacket &packet);
};

#endif // RIPPLEELEMENT_H
