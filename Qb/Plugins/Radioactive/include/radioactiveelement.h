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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef RADIOACTIVEELEMENT_H
#define RADIOACTIVEELEMENT_H

#include <cmath>
#include <QColor>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

#include <qb.h>
#include <qbutils.h>

class RadioactiveElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(RadiationMode)
    Q_PROPERTY(QString mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(float blur READ blur WRITE setBlur RESET resetBlur)
    Q_PROPERTY(float zoom READ zoom WRITE setZoom RESET resetZoom)
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold RESET resetThreshold)
    Q_PROPERTY(int lumaThreshold READ lumaThreshold WRITE setLumaThreshold RESET resetLumaThreshold)
    Q_PROPERTY(int alphaDiff READ alphaDiff WRITE setAlphaDiff RESET resetAlphaDiff)
    Q_PROPERTY(QRgb radColor READ radColor WRITE setRadColor RESET resetRadColor)

    public:
        enum RadiationMode
        {
            RadiationModeSoftNormal,
            RadiationModeHardNormal,
            RadiationModeSoftColor,
            RadiationModeHardColor
        };

        explicit RadioactiveElement();
        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE float blur() const;
        Q_INVOKABLE float zoom() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;
        Q_INVOKABLE int alphaDiff() const;
        Q_INVOKABLE QRgb radColor() const;

    private:
        RadiationMode m_mode;
        float m_blur;
        float m_zoom;
        int m_threshold;
        int m_lumaThreshold;
        int m_alphaDiff;
        QRgb m_radColor;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_prevFrame;
        QImage m_blurZoomBuffer;
        QMap<RadiationMode, QString> m_radiationModeToStr;

        QImage imageDiff(const QImage &img1,
                         const QImage &img2,
                         int threshold,
                         int lumaThreshold,
                         QRgb radColor,
                         RadiationMode mode);

        QImage imageAlphaDiff(const QImage &src, int alphaDiff);

    public slots:
        void setMode(const QString &mode);
        void setBlur(float blur);
        void setZoom(float zoom);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void setAlphaDiff(int alphaDiff);
        void setRadColor(QRgb radColor);
        void resetMode();
        void resetBlur();
        void resetZoom();
        void resetThreshold();
        void resetLumaThreshold();
        void resetAlphaDiff();
        void resetRadColor();
        QbPacket iStream(const QbPacket &packet);
};

#endif // RADIOACTIVEELEMENT_H
