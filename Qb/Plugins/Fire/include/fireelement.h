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

#ifndef FIREELEMENT_H
#define FIREELEMENT_H

#include <cmath>
#include <QColor>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class FireElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(FireMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int cool
               READ cool
               WRITE setCool
               RESET resetCool
               NOTIFY coolChanged)
    Q_PROPERTY(qreal disolve
               READ disolve
               WRITE setDisolve
               RESET resetDisolve
               NOTIFY disolveChanged)
    Q_PROPERTY(qreal blur
               READ blur
               WRITE setBlur
               RESET resetBlur
               NOTIFY blurChanged)
    Q_PROPERTY(qreal zoom
               READ zoom
               WRITE setZoom
               RESET resetZoom
               NOTIFY zoomChanged)
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
    Q_PROPERTY(int alphaDiff
               READ alphaDiff
               WRITE setAlphaDiff
               RESET resetAlphaDiff
               NOTIFY alphaDiffChanged)
    Q_PROPERTY(int alphaVariation
               READ alphaVariation
               WRITE setAlphaVariation
               RESET resetAlphaVariation
               NOTIFY alphaVariationChanged)
    Q_PROPERTY(int nColors
               READ nColors
               WRITE setNColors
               RESET resetNColors
               NOTIFY nColorsChanged)

    public:
        enum FireMode
        {
            FireModeSoft,
            FireModeHard
        };

        explicit FireElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int cool() const;
        Q_INVOKABLE qreal disolve() const;
        Q_INVOKABLE qreal blur() const;
        Q_INVOKABLE qreal zoom() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;
        Q_INVOKABLE int alphaDiff() const;
        Q_INVOKABLE int alphaVariation() const;
        Q_INVOKABLE int nColors() const;

    private:
        FireMode m_mode;
        int m_cool;
        qreal m_disolve;
        qreal m_blur;
        qreal m_zoom;
        int m_threshold;
        int m_lumaThreshold;
        int m_alphaDiff;
        int m_alphaVariation;
        int m_nColors;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_prevFrame;
        QImage m_fireBuffer;
        QVector<QRgb> m_palette;
        QMap<FireMode, QString> m_fireModeToStr;

        QImage imageDiff(const QImage &img1,
                         const QImage &img2,
                         int colors,
                         int threshold,
                         int lumaThreshold, int alphaVariation,
                         FireMode mode);

        QImage zoomImage(const QImage &src, qreal factor);
        void coolImage(const QImage &src, int colorDiff);
        void imageAlphaDiff(const QImage &src, int alphaDiff);
        void disolveImage(const QImage &src, qreal amount);
        QImage blurImage(const QImage &src, qreal factor);
        QImage burn(const QImage &src, const QVector<QRgb> &palette);
        QVector<QRgb> createPalette();

    signals:
        void modeChanged();
        void coolChanged();
        void disolveChanged();
        void blurChanged();
        void zoomChanged();
        void thresholdChanged();
        void lumaThresholdChanged();
        void alphaDiffChanged();
        void alphaVariationChanged();
        void nColorsChanged();

    public slots:
        void setMode(const QString &mode);
        void setCool(int cool);
        void setDisolve(qreal disolve);
        void setBlur(qreal blur);
        void setZoom(qreal zoom);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void setAlphaDiff(int alphaDiff);
        void setAlphaVariation(int alphaVariation);
        void setNColors(int nColors);
        void resetMode();
        void resetCool();
        void resetDisolve();
        void resetBlur();
        void resetZoom();
        void resetThreshold();
        void resetLumaThreshold();
        void resetAlphaDiff();
        void resetAlphaVariation();
        void resetNColors();
        QbPacket iStream(const QbPacket &packet);
};

#endif // FIREELEMENT_H
