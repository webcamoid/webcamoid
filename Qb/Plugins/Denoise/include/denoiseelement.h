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

#ifndef DENOISEELEMENT_H
#define DENOISEELEMENT_H

#include <cmath>
#include <qb.h>
#include <qbutils.h>

class DenoiseElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(DenoiseMode)
    Q_PROPERTY(QString mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(QSize scanSize READ scanSize WRITE setScanSize RESET resetScanSize)
    Q_PROPERTY(float mu READ mu WRITE setMu RESET resetMu)
    Q_PROPERTY(float sigma READ sigma WRITE setSigma RESET resetSigma)

    public:
        enum DenoiseMode
        {
            DenoiseModeGauss,
            DenoiseModeSelect
        };

        explicit DenoiseElement();
        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE QSize scanSize() const;
        Q_INVOKABLE float mu() const;
        Q_INVOKABLE float sigma() const;

    private:
        DenoiseMode m_mode;
        QSize m_scanSize;
        int m_mu;
        int m_sigma;

        QbElementPtr m_convert;
        QMap<DenoiseMode, QString> m_denoiseModeToStr;

        inline QRgb selectAverageColor(QRgb *src,
                                       int width,
                                       const QRect &kernel)
        {
            int kernelSize = kernel.width() * kernel.height();
            QVector<int> planeR(kernelSize);
            QVector<int> planeG(kernelSize);
            QVector<int> planeB(kernelSize);

            // Calculate average.
            for (int y = kernel.top(), i = 0; y < kernel.bottom() + 1; y++) {
                int xOffset = y * width;

                for (int x = kernel.left(); x < kernel.right() + 1; x++, i++) {
                    QRgb pixel = src[x + xOffset];

                    planeR[i] = qRed(pixel);
                    planeG[i] = qGreen(pixel);
                    planeB[i] = qBlue(pixel);
                }
            }

            qSort(planeR);
            qSort(planeG);
            qSort(planeB);

            int mid = kernelSize >> 1;

            QPoint center = kernel.center();
            int a = qAlpha(src[center.x() + center.y() * width]);

            return qRgba(planeR[mid], planeG[mid], planeB[mid], a);
        }

        inline QRgb averageColor(QRgb *src,
                                 int width,
                                 const QRect &kernel,
                                 float mu,
                                 float sigma)
        {
            int kernelSize = kernel.width() * kernel.height();
            int planeR[kernelSize];
            int planeG[kernelSize];
            int planeB[kernelSize];

            // Calculate average.
            float mr = 0;
            float mg = 0;
            float mb = 0;

            for (int y = kernel.top(), i = 0; y < kernel.bottom() + 1; y++) {
                int xOffset = y * width;

                for (int x = kernel.left(); x < kernel.right() + 1; x++, i++) {
                    QRgb pixel = src[x + xOffset];

                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);

                    mr += r;
                    mg += g;
                    mb += b;

                    planeR[i] = r;
                    planeG[i] = g;
                    planeB[i] = b;
                }
            }

            mr /= kernelSize;
            mg /= kernelSize;
            mb /= kernelSize;

            int planeRd2[kernelSize];
            int planeGd2[kernelSize];
            int planeBd2[kernelSize];

            // Calculate standard deviation.
            float sr = 0;
            float sg = 0;
            float sb = 0;

            for (int i = 0; i < kernelSize; i++) {
                int dr = planeR[i] - mr;
                int dg = planeG[i] - mg;
                int db = planeB[i] - mb;

                dr *= dr;
                dg *= dg;
                db *= db;

                sr += dr;
                sg += dg;
                sb += db;

                planeRd2[i] = dr;
                planeGd2[i] = dg;
                planeBd2[i] = db;
            }

            // Apply factors.
            int ks = kernelSize - 1;

            mr *= mu;
            mg *= mu;
            mb *= mu;

            sr = sigma * sqrt(sr / ks);
            sg = sigma * sqrt(sg / ks);
            sb = sigma * sqrt(sb / ks);

            // Calculate weighted average.
            float vr = 0;
            float vg = 0;
            float vb = 0;

            float twr = 0;
            float twg = 0;
            float twb = 0;

            float hr = 2 * sr * sr;
            float hg = 2 * sg * sg;
            float hb = 2 * sb * sb;

            for (int i = 0; i < kernelSize; i++) {
                if (hr) {
                    float wr = exp(-planeRd2[i] / hr);
                    vr += wr * planeR[i];
                    twr += wr;
                }

                if (hg) {
                    float wg = exp(-planeGd2[i] / hg);
                    vg += wg * planeG[i];
                    twg += wg;
                }

                if (hb) {
                    float wb = exp(-planeBd2[i] / hb);
                    vb += wb * planeB[i];
                    twb += wb;
                }
            }

            int r = twr? vr / twr: mr;
            int g = twg? vg / twg: mg;
            int b = twb? vb / twb: mb;

            QPoint center = kernel.center();
            int a = qAlpha(src[center.x() + center.y() * width]);

            return qRgba(r, g, b, a);
        }

    public slots:
        void setMode(const QString &mode);
        void setScanSize(const QSize &scanSize);
        void setMu(float mu);
        void setSigma(float sigma);
        void resetMode();
        void resetScanSize();
        void resetMu();
        void resetSigma();
        QbPacket iStream(const QbPacket &packet);
};

#endif // DENOISEELEMENT_H
