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

#ifndef CARTOONELEMENT_H
#define CARTOONELEMENT_H

#include <QMutex>
#include <ak.h>
#include <akutils.h>

class CartoonElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int ncolors
               READ ncolors
               WRITE setNColors
               RESET resetNColors
               NOTIFY ncolorsChanged)
    Q_PROPERTY(int colorDiff
               READ colorDiff
               WRITE setColorDiff
               RESET resetColorDiff
               NOTIFY colorDiffChanged)
    Q_PROPERTY(bool showEdges
               READ showEdges
               WRITE setShowEdges
               RESET resetShowEdges
               NOTIFY showEdgesChanged)
    Q_PROPERTY(int thresholdLow
               READ thresholdLow
               WRITE setThresholdLow
               RESET resetThresholdLow
               NOTIFY thresholdLowChanged)
    Q_PROPERTY(int thresholdHi
               READ thresholdHi
               WRITE setThresholdHi
               RESET resetThresholdHi
               NOTIFY thresholdHiChanged)
    Q_PROPERTY(QRgb lineColor
               READ lineColor
               WRITE setLineColor
               RESET resetLineColor
               NOTIFY lineColorChanged)
    Q_PROPERTY(QSize scanSize
               READ scanSize
               WRITE setScanSize
               RESET resetScanSize
               NOTIFY scanSizeChanged)

    public:
        explicit CartoonElement();
        ~CartoonElement();

        Q_INVOKABLE int ncolors() const;
        Q_INVOKABLE int colorDiff() const;
        Q_INVOKABLE bool showEdges() const;
        Q_INVOKABLE int thresholdLow() const;
        Q_INVOKABLE int thresholdHi() const;
        Q_INVOKABLE QRgb lineColor() const;
        Q_INVOKABLE QSize scanSize() const;

    private:
        int m_ncolors;
        int m_colorDiff;
        bool m_showEdges;
        int m_thresholdLow;
        int m_thresholdHi;
        QRgb m_lineColor;
        QSize m_scanSize;
        QVector<QRgb> m_palette;
        qint64 m_id;
        qint64 m_lastTime;
        QMutex m_mutex;

        QVector<QRgb> palette(const QImage &img, int ncolors, int colorDiff);
        QRgb nearestColor(int *index, int *diff, const QVector<QRgb> &palette, QRgb color) const;
        QImage edges(const QImage &src, int thLow, int thHi, QRgb color) const;

        inline int rgb24Torgb16(QRgb color)
        {
            return ((qRed(color) >> 3) << 11)
                    | ((qGreen(color) >> 2) << 5)
                    | (qBlue(color) >> 3);
        }

        inline void rgb16Torgb24(int *r, int *g, int *b, int color)
        {
            *r = (color >> 11) & 0x1f;
            *g = (color >> 5) & 0x3f;
            *b = color & 0x1f;
            *r = 0xff * *r / 0x1f;
            *g = 0xff * *g / 0x3f;
            *b = 0xff * *b / 0x1f;
        }

        inline QRgb rgb16Torgb24(int color)
        {
            int r;
            int g;
            int b;
            rgb16Torgb24(&r, &g, &b, color);

            return qRgb(r, g, b);
        }

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void ncolorsChanged(int ncolors);
        void colorDiffChanged(int colorDiff);
        void showEdgesChanged(bool showEdges);
        void thresholdLowChanged(int thresholdLow);
        void thresholdHiChanged(int thresholdHi);
        void lineColorChanged(QRgb lineColor);
        void scanSizeChanged(const QSize &scanSize);

    public slots:
        void setNColors(int ncolors);
        void setColorDiff(int colorDiff);
        void setShowEdges(bool showEdges);
        void setThresholdLow(int thresholdLow);
        void setThresholdHi(int thresholdHi);
        void setLineColor(QRgb lineColor);
        void setScanSize(const QSize &scanSize);
        void resetNColors();
        void resetColorDiff();
        void resetShowEdges();
        void resetThresholdLow();
        void resetThresholdHi();
        void resetLineColor();
        void resetScanSize();
        AkPacket iStream(const AkPacket &packet);
};

#endif // CARTOONELEMENT_H
