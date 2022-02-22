/* QtHaar, Viola-Jones implementation in Qt.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
 *
 * QtHaar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QtHaar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtHaar. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://github.com/hipersayanX/QtHaar
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *      By downloading, copying, installing or using the software you agree to this license.
 *      If you do not agree to this license, do not download, install,
 *      copy or use the software.
 *
 *
 *                            Intel License Agreement
 *                    For Open Source Computer Vision Library
 *
 *     Copyright (C) 2000, Intel Corporation, all rights reserved.
 *     Third party copyrights are property of their respective owners.
 *
 *     Redistribution and use in source and binary forms, with or without modification,
 *     are permitted provided that the following conditions are met:
 *
 *       * Redistribution's of source code must retain the above copyright notice,
 *         this list of conditions and the following disclaimer.
 *
 *       * Redistribution's in binary form must reproduce the above copyright notice,
 *         this list of conditions and the following disclaimer in the documentation
 *         and/or other materials provided with the distribution.
 *
 *       * The name of Intel Corporation may not be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *     This software is provided by the copyright holders and contributors "as is" and
 *     any express or implied warranties, including, but not limited to, the implied
 *     warranties of merchantability and fitness for a particular purpose are disclaimed.
 *     In no event shall the Intel Corporation or contributors be liable for any direct,
 *     indirect, incidental, special, exemplary, or consequential damages
 *     (including, but not limited to, procurement of substitute goods or services;
 *     loss of use, data, or profits; or business interruption) however caused
 *     and on any theory of liability, whether in contract, strict liability,
 *     or tort (including negligence or otherwise) arising in any way out of
 *     the use of this software, even if advised of the possibility of such damage.
 */

#include <QtMath>
#include <QtConcurrent>

#include "haarcascade.h"
#include "haardetector.h"

class HaarDetectorPrivate
{
    public:
        HaarCascade m_cascade;
        bool m_equalize {false};
        int m_denoiseRadius {0};
        int m_denoiseMu {0};
        int m_denoiseSigma {0};
        bool m_cannyPruning {false};
        qreal m_lowCannyThreshold {0};
        qreal m_highCannyThreshold {50};
        int m_minNeighbors {3};
        QVector<int> m_weight;
        QMutex m_mutex;

        QVector<int> makeWeightTable(int factor) const;
        void computeGray(const QImage &src, bool equalize,
                         QVector<quint8> &gray) const;
        void computeIntegral(int width, int height,
                             const QVector<quint8> &image,
                             QVector<quint32> &integral) const;
        void computeIntegral(int width, int height,
                             const QVector<quint8> &image,
                             int paddingTL,
                             QVector<quint32> &integral) const;
        void computeIntegral(int width, int height,
                             const QVector<quint8> &image,
                             QVector<quint32> &integral,
                             QVector<quint64> &integral2) const;
        void computeIntegral(int width, int height,
                             const QVector<quint8> &image,
                             QVector<quint32> &integral,
                             QVector<quint64> &integral2,
                             QVector<quint32> &tiltedIntegral) const;
        QVector<quint8> canny(int width, int height,
                              const QVector<quint8> &gray) const;
        void imagePadding(int width, int height,
                          const QVector<quint8> &image,
                          int paddingTL, int paddingBR,
                          QVector<quint8> &padded) const;
        void denoise(int width, int height, const QVector<quint8> &gray,
                     int radius, int mu, int sigma,
                     QVector<quint8> &denoised) const;
        void sobel(int width, int height, const QVector<quint8> &gray,
                   QVector<quint16> &gradient, QVector<quint8> &direction) const;
        QVector<quint16> thinning(int width, int height,
                                  const QVector<quint16> &gradient,
                                  const QVector<quint8> &direction) const;
        QVector<int> calculateHistogram(int width, int height,
                                        const QVector<quint16> &image,
                                        int levels) const;
        QVector<qreal> buildTables(const QVector<int> &histogram) const;
        void forLoop(qreal *maxSum,
                     QVector<int> *thresholds,
                     const QVector<qreal> &H,
                     int u,
                     int vmax,
                     int level,
                     int levels,
                     QVector<int> *index) const;
        QVector<int> otsu(QVector<int> histogram, int classes) const;
        QVector<quint8> threshold(int width, int height,
                                  const QVector<quint16> &image,
                                  const QVector<int> &thresholds,
                                  const QVector<int> &map) const;
        void trace(int width, int height, QVector<quint8> &canny,
                   int x, int y) const;
        QVector<quint8> hysteresisThresholding(int width, int height,
                                               const QVector<quint8> &thresholded) const;
        bool areSimilar(const QRect &r1, const QRect &r2, qreal eps) const;
        void markRectangle(const QVector<QRect> &rectangles,
                           QVector<int> &labels,
                           int i, int label, qreal eps) const;
        QVector<int> classifyRectangles(const QVector<QRect> &rectangles,
                                        qreal eps, int *nClasses=nullptr) const;
        RectVector groupRectangles(const RectVector &rects, int minNeighbors=3,
                                   qreal eps=0.2) const;
};

QVector<int> HaarDetectorPrivate::makeWeightTable(int factor) const
{
    QVector<int> weight(1 << 24);

    for (int s = 0; s < 128; s++) {
        int h = -2 * s * s;

        for (int m = 0; m < 256; m++)
            for (int c = 0; c < 256; c++) {
                if (s == 0) {
                    weight[(m << 16) | (s << 8) | c] = 0;

                    continue;
                }

                int d = c - m;
                d *= d;

                weight[(m << 16) | (s << 8) | c] = int(factor * exp(qreal(d) / h));
            }
    }

    return weight;
}

void HaarDetectorPrivate::computeGray(const QImage &src, bool equalize,
                                      QVector<quint8> &gray) const
{
    gray.resize(src.width() * src.height());
    QImage image;

    if (src.format() == QImage::Format_ARGB32)
        image = src;
    else
        image = src.convertToFormat(QImage::Format_ARGB32);

    auto imageBits = reinterpret_cast<const QRgb *>(image.constBits());
    int minGray = 255;
    int maxGray = 0;

    for (int i = 0; i < gray.size(); i++) {
        int pixel = qGray(imageBits[i]);

        if (equalize) {
            if (pixel < minGray)
                minGray = pixel;

            if (pixel > maxGray)
                maxGray = pixel;
        }

        gray[i] = quint8(pixel);
    }

    if (!equalize || maxGray == minGray)
        return;

    int diffGray = maxGray - minGray;

    for (auto &g: gray)
        g = quint8(255 * (g - minGray) / diffGray);
}

void HaarDetectorPrivate::computeIntegral(int width, int height,
                                          const QVector<quint8> &image,
                                          QVector<quint32> &integral) const
{
    integral.resize(image.size());
    quint32 sum = 0;

    for (int x = 0; x < width; x++) {
        sum += image[x];
        integral[x] = sum;
    }

    const quint32 *integralPrevLine = integral.constData();

    for (int y = 1; y < height; y++) {
        auto yOffset = size_t(y * width);
        auto imageLine = image.constData() + yOffset;
        auto integralLine = integral.data() + yOffset;
        sum = 0;

        for (int x = 0; x < width; x++) {
            sum += imageLine[x];
            integralLine[x] = sum + integralPrevLine[x];
        }

        integralPrevLine = integralLine;
    }
}

void HaarDetectorPrivate::computeIntegral(int width, int height,
                                          const QVector<quint8> &image,
                                          int paddingTL,
                                          QVector<quint32> &integral) const
{
    if (paddingTL < 0)
        paddingTL = 0;

    integral.resize((width + paddingTL) * (height + paddingTL));
    auto integralData = integral.data();

    if (paddingTL)
        integralData += paddingTL * (width + paddingTL + 1);

    quint32 sum = 0;

    for (int x = 0; x < width; x++) {
        sum += image[x];
        integralData[x] = sum;
    }

    const quint32 *integralPrevLine = integralData;

    for (int y = 1; y < height; y++) {
        auto yOffset = size_t(y * width);
        auto imageLine = image.data() + yOffset;
        auto integralLine = integralData + yOffset + paddingTL * y;
        quint32 sum = 0;

        for (int x = 0; x < width; x++) {
            sum += imageLine[x];
            integralLine[x] = sum + integralPrevLine[x];
        }

        integralPrevLine = integralLine;
    }
}

void HaarDetectorPrivate::computeIntegral(int width, int height,
                                          const QVector<quint8> &image,
                                          QVector<quint32> &integral,
                                          QVector<quint64> &integral2) const
{
    integral.resize(image.size());
    integral2.resize(image.size());
    quint32 sum = 0;
    quint64 sum2 = 0;

    for (int x = 0; x < width; x++) {
        quint8 pixel = image[x];
        sum += pixel;
        sum2 += pixel * pixel;
        integral[x] = sum;
        integral2[x] = sum2;
    }

    auto integralPrevLine = integral.constData();
    auto integral2PrevLine = integral2.constData();

    for (int y = 1; y < height; y++) {
        auto yOffset = size_t(y * width);
        auto imageLine = image.constData() + yOffset;
        auto integralLine = integral.data() + yOffset;
        auto integral2Line = integral2.data() + yOffset;

        sum = 0;
        sum2 = 0;

        for (int x = 0; x < width; x++) {
            quint8 pixel = imageLine[x];
            sum += pixel;
            sum2 += pixel * pixel;
            integralLine[x] = sum + integralPrevLine[x];
            integral2Line[x] = sum2 + integral2PrevLine[x];
        }

        integralPrevLine = integralLine;
        integral2PrevLine = integral2Line;
    }
}

void HaarDetectorPrivate::computeIntegral(int width, int height,
                                          const QVector<quint8> &image,
                                          QVector<quint32> &integral,
                                          QVector<quint64> &integral2,
                                          QVector<quint32> &tiltedIntegral) const
{
    int oWidth = width + 1;
    int oHeight = height + 1;
    int outSize = oWidth * oHeight;
    integral.resize(outSize);
    integral2.resize(outSize);
    tiltedIntegral.resize(outSize);

    int oWidth2 = oWidth + 1;
    quint32 *integralLine = integral.data() + oWidth2;
    quint64 *integral2Line = integral2.data() + oWidth2;
    quint32 *tiltedIntegralLine = tiltedIntegral.data() + oWidth2;

    quint8 pixel;
    quint32 sum = 0;
    quint64 sum2 = 0;

    for (int x = 0; x < width; x++) {
        pixel = image[x];
        sum += pixel;
        sum2 += pixel * pixel;

        integralLine[x] = sum;
        integral2Line[x] = sum2;
        tiltedIntegralLine[x] = pixel;
    }

    for (int y = 2; y < oHeight; y++) {
        const quint8 *imageLine_mow = image.constData() + y * width - oWidth;
        const quint8 *imageLine_mow_mw = imageLine_mow - width;

        int yoOffset = y * oWidth;

        integralLine = integral.data() + yoOffset;
        quint32 *integralLine_mow = integralLine - oWidth;

        integral2Line = integral2.data() + yoOffset;
        quint64 *integral2Line_mow = integral2Line - oWidth;

        tiltedIntegralLine = tiltedIntegral.data() + yoOffset;
        quint32 *tiltedIntegralLine_mow2 = tiltedIntegralLine - oWidth2;
        quint32 *tiltedIntegralLine_mw = tiltedIntegralLine - width;
        quint32 *tiltedIntegralLine_m2ow = tiltedIntegralLine - 2 * oWidth;

        sum = 0;
        sum2 = 0;

        for (int x = 0; x < oWidth; x++) {
            if (x > 0) {
                pixel = imageLine_mow[x];
                sum += pixel;
                sum2 += pixel * pixel;
            } else
                pixel = 0;

            integralLine[x] = sum + integralLine_mow[x];
            integral2Line[x] = sum2 + integral2Line_mow[x];

            quint32 tiltedSum = pixel;

            if (x > 0) {
                tiltedSum += imageLine_mow_mw[x];
                tiltedSum += tiltedIntegralLine_mow2[x];
            }

            if (x < width) {
                tiltedSum += tiltedIntegralLine_mw[x];

                if (x > 0)
                    tiltedSum -= tiltedIntegralLine_m2ow[x];
            }

            tiltedIntegralLine[x] = tiltedSum;
        }
    }
}

QVector<quint8> HaarDetectorPrivate::canny(int width, int height,
                                           const QVector<quint8> &gray) const
{
    QVector<quint16> gradient;
    QVector<quint8> direction;
    this->sobel(width, height, gray, gradient, direction);
    auto thinned = this->thinning(width, height, gradient, direction);

    QVector<int> otsu(2);

    if (qIsNaN(this->m_lowCannyThreshold)
        || qIsNaN(this->m_highCannyThreshold)) {
        auto hist = this->calculateHistogram(width,
                                             height,
                                             thinned,
                                             6 * 255 + 1);
        otsu = this->otsu(hist, 3);
    }

    if (!qIsNaN(this->m_lowCannyThreshold))
        otsu[0] = int(this->m_lowCannyThreshold);

    if (!qIsNaN(this->m_highCannyThreshold))
        otsu[1] = int(this->m_highCannyThreshold);

    QVector<int> colors {0, 127, 255};
    auto thresholded = this->threshold(width, height, thinned, otsu, colors);

    return this->hysteresisThresholding(width, height, thresholded);
}

void HaarDetectorPrivate::imagePadding(int width, int height,
                                       const QVector<quint8> &image,
                                       int paddingTL, int paddingBR,
                                       QVector<quint8> &padded) const
{
    int oWidth = width  + paddingTL + paddingBR;
    padded.resize(oWidth * (height + paddingTL + paddingBR));
    int offset = paddingTL * (oWidth + 1);

    for (int y = 0; y < height; y++) {
        const quint8 *imageLine = image.constData() + y * width;
        quint8 *paddedLine = padded.data() + y * oWidth + offset;
        memcpy(paddedLine, imageLine, size_t(width));
    }
}

void HaarDetectorPrivate::denoise(int width, int height,
                                  const QVector<quint8> &gray,
                                  int radius, int mu, int sigma,
                                  QVector<quint8> &denoised) const
{
    denoised.resize(gray.size());

    QVector<quint8> padded;
    this->imagePadding(width, height, gray, radius + 1, radius, padded);

    int kernelSize = 2 * radius + 1;
    int oWidth = width + kernelSize;
    QVector<quint32> integral;
    QVector<quint64> integral2;
    this->computeIntegral(oWidth, height + kernelSize,
                          padded, integral, integral2);

    int kernelSize2 = kernelSize * kernelSize;

    for (int y = 0, pixels = 0; y < height; y++) {
        auto integral_p0 = integral.constData() + y * oWidth;
        auto integral_p1 = integral_p0 + kernelSize;
        auto integral_p2 = integral_p0 + kernelSize * oWidth;
        auto integral_p3 = integral_p2 + kernelSize;

        auto integral2_p0 = integral2.constData() + y * oWidth;
        auto integral2_p1 = integral2_p0 + kernelSize;
        auto integral2_p2 = integral2_p0 + kernelSize * oWidth;
        auto integral2_p3 = integral2_p2 + kernelSize;

        for (int x = 0; x < width; x++, pixels++) {
            quint32 sum = integral_p0[x]
                        + integral_p3[x]
                        - integral_p1[x]
                        - integral_p2[x];

            quint64 sum2 = integral2_p0[x]
                         + integral2_p1[x]
                         - integral2_p2[x]
                         - integral2_p3[x];

            auto mean = quint8(sum / uint(kernelSize2));
            auto stdev = quint8(sqrt(qreal(sum2) / kernelSize2 - mean * mean));

            mean = quint8(qBound(0, mean + mu, 255));
            stdev = quint8(qBound(0, stdev + sigma, 255));

            int offset = x + y * oWidth;

            quint64 sumPound = 0;
            quint64 sumWeights = 0;

            for (int j = 0; j < kernelSize; j++) {
                const quint8 *paddedLine = padded.constData() + j * oWidth + offset;

                for (int i = 0; i < kernelSize; i++) {
                    quint8 pixel = paddedLine[i];
                    int weight = this->m_weight[(mean << 16) | (stdev << 8) | pixel];
                    sumPound += quint64(weight * pixel);
                    sumWeights += quint64(weight);
                }
            }

            denoised[pixels] = sumWeights < 1?
                                   gray[pixels]:
                                   quint8(sumPound / sumWeights);
        }
    }
}

void HaarDetectorPrivate::sobel(int width, int height,
                                const QVector<quint8> &gray,
                                QVector<quint16> &gradient,
                                QVector<quint8> &direction) const
{
    gradient.resize(gray.size());
    direction.resize(gray.size());

    for (int y = 0; y < height; y++) {
        auto yOffset = size_t(y * width);
        auto grayLine = gray.constData() + yOffset;

        auto grayLine_m1 = y < 1? grayLine: grayLine - width;
        auto grayLine_p1 = y >= height - 1? grayLine: grayLine + width;

        quint16 *gradientLine = gradient.data() + yOffset;
        quint8 *directionLine = direction.data() + yOffset;

        for (int x = 0; x < width; x++) {
            int x_m1 = x < 1? x: x - 1;
            int x_p1 = x >= width - 1? x: x + 1;

            int gradX = grayLine_m1[x_p1]
                      + 2 * grayLine[x_p1]
                      + grayLine_p1[x_p1]
                      - grayLine_m1[x_m1]
                      - 2 * grayLine[x_m1]
                      - grayLine_p1[x_m1];

            int gradY = grayLine_m1[x_m1]
                      + 2 * grayLine_m1[x]
                      + grayLine_m1[x_p1]
                      - grayLine_p1[x_m1]
                      - 2 * grayLine_p1[x]
                      - grayLine_p1[x_p1];

            gradientLine[x] = quint16(qAbs(gradX) + qAbs(gradY));

            /* Gradient directions are classified in 4 possible cases
             *
             * dir 0
             *
             * x x x
             * - - -
             * x x x
             *
             * dir 1
             *
             * x x /
             * x / x
             * / x x
             *
             * dir 2
             *
             * \ x x
             * x \ x
             * x x \
             *
             * dir 3
             *
             * x | x
             * x | x
             * x | x
             */
            if (gradX == 0 && gradY == 0)
                directionLine[x] = 0;
            else if (gradX == 0)
                directionLine[x] = 3;
            else {
                qreal a = 180. * atan(qreal(gradY) / gradX) / M_PI;

                if (a >= -22.5 && a < 22.5)
                    directionLine[x] = 0;
                else if (a >= 22.5 && a < 67.5)
                    directionLine[x] = 1;
                else if (a >= -67.5 && a < -22.5)
                    directionLine[x] = 2;
                else
                    directionLine[x] = 3;
            }
        }
    }
}

QVector<quint16> HaarDetectorPrivate::thinning(int width, int height,
                                               const QVector<quint16> &gradient,
                                               const QVector<quint8> &direction) const
{
    QVector<quint16> thinned(gradient.size(), 0);

    for (int y = 0; y < height; y++) {
        int yOffset = y * width;
        auto edgesLine = gradient.constData() + yOffset;
        auto edgesLine_m1 = y < 1? edgesLine: edgesLine - width;
        auto edgesLine_p1 = y >= height - 1? edgesLine: edgesLine + width;
        auto edgesAngleLine = direction.constData() + yOffset;
        auto thinnedLine = thinned.data() + yOffset;

        for (int x = 0; x < width; x++) {
            int x_m1 = x < 1? 0: x - 1;
            int x_p1 = x >= width - 1? x: x + 1;

            quint8 direction = edgesAngleLine[x];

            if (direction == 0) {
                /* x x x
                 * - - -
                 * x x x
                 */
                if (edgesLine[x] >= edgesLine[x_m1]
                    && edgesLine[x] >= edgesLine[x_p1])
                    thinnedLine[x] = edgesLine[x];
            } else if (direction == 1) {
                /* x x /
                 * x / x
                 * / x x
                 */
                if (edgesLine[x] >= edgesLine_m1[x_p1]
                    && edgesLine[x] >= edgesLine_p1[x_m1])
                    thinnedLine[x] = edgesLine[x];
            } else if (direction == 2) {
                /* \ x x
                 * x \ x
                 * x x \
                 */
                if (edgesLine[x] >= edgesLine_m1[x_m1]
                    && edgesLine[x] >= edgesLine_p1[x_p1])
                    thinnedLine[x] = edgesLine[x];
            } else {
                /* x | x
                 * x | x
                 * x | x
                 */
                if (edgesLine[x] >= edgesLine_m1[x]
                    && edgesLine[x] >= edgesLine_p1[x])
                    thinnedLine[x] = edgesLine[x];
            }
        }
    }

    return thinned;
}

QVector<int> HaarDetectorPrivate::calculateHistogram(int width, int height,
                                                     const QVector<quint16> &image,
                                                     int levels) const
{
    QVector<int> histogram(levels, 0);
    int pixels = width * height;

    for (int i = 0; i < pixels; i++)
        histogram[image[i]]++;

    // Since we use sum tables add one more to avoid unexistent colors.
    for (int i = 0; i < histogram.size(); i++)
        histogram[i]++;

    return histogram;
}

QVector<qreal> HaarDetectorPrivate::buildTables(const QVector<int> &histogram) const
{
    // Create cumulative sum tables.
    QVector<quint64> P(histogram.size() + 1);
    QVector<quint64> S(histogram.size() + 1);
    P[0] = 0;
    S[0] = 0;

    quint64 sumP = 0;
    quint64 sumS = 0;

    for (int i = 0; i < histogram.size(); i++) {
        sumP += quint64(histogram[i]);
        sumS += quint64(i * histogram[i]);
        P[i + 1] = sumP;
        S[i + 1] = sumS;
    }

    // Calculate the between-class variance for the interval u-v
    QVector<qreal> H(histogram.size() * histogram.size(), 0.);

    for (int u = 0; u < histogram.size(); u++) {
        auto hLine = H.data() + u * histogram.size();

        for (int v = u + 1; v < histogram.size(); v++)
            hLine[v] = qPow(S[v] - S[u], 2) / (P[v] - P[u]);
    }

    return H;
}

void HaarDetectorPrivate::forLoop(qreal *maxSum,
                                  QVector<int> *thresholds,
                                  const QVector<qreal> &H,
                                  int u,
                                  int vmax,
                                  int level,
                                  int levels,
                                  QVector<int> *index) const
{
    int classes = index->size() - 1;

    for (int i = u; i < vmax; i++) {
        (*index)[level] = i;

        if (level + 1 >= classes) {
            // Reached the end of the for loop.

            // Calculate the quadratic sum of all intervals.
            qreal sum = 0.;

            for (int c = 0; c < classes; c++) {
                int u = index->at(c);
                int v = index->at(c + 1);
                sum += H[v + u * levels];
            }

            if (*maxSum < sum) {
                // Return calculated threshold.
                *thresholds = index->mid(1, thresholds->size());
                *maxSum = sum;
            }
        } else
            // Start a new for loop level, one position after current one.
            this->forLoop(maxSum,
                          thresholds,
                          H,
                          i + 1,
                          vmax + 1,
                          level + 1,
                          levels,
                          index);
    }
}

QVector<int> HaarDetectorPrivate::otsu(QVector<int> histogram,
                                       int classes) const
{
    qreal maxSum = 0.;
    QVector<int> thresholds(classes - 1, 0);
    auto H = this->buildTables(histogram);
    QVector<int> index(classes + 1);
    index[0] = 0;
    index[index.size() - 1] = histogram.size() - 1;

    this->forLoop(&maxSum,
                  &thresholds,
                  H,
                  1,
                  histogram.size() - classes + 1,
                  1,
                  histogram.size(), &index);

    return thresholds;
}

QVector<quint8> HaarDetectorPrivate::threshold(int width, int height,
                                               const QVector<quint16> &image,
                                               const QVector<int> &thresholds,
                                               const QVector<int> &map) const
{
    int size = width * height;
    auto in = image.constData();
    QVector<quint8> out(size);

    for (int i = 0; i < size; i++) {
        int value = -1;

        for (int j = 0; j < thresholds.size(); j++)
            if (in[i] <= thresholds[j]) {
                value = map[j];

                break;
            }

        out[i] = quint8(value < 0? map[thresholds.size()]: value);
    }

    return out;
}

void HaarDetectorPrivate::trace(int width, int height, QVector<quint8> &canny,
                                int x, int y) const
{
    int yOffset = y * width;
    quint8 *cannyLine = canny.data() + yOffset;

    if (cannyLine[x] != 255)
        return;

    bool isPoint = true;

    for (int j = -1; j < 2; j++) {
        int nextY = y + j;

        if (nextY < 0 || nextY >= height)
            continue;

        quint8 *cannyLineNext = cannyLine + j * width;

        for (int i = -1; i < 2; i++) {
            int nextX = x + i;

            if (i == 0 && j == 0)
                continue;

            if (nextX < 0 || nextX >= width)
                continue;

            if (cannyLineNext[nextX] == 127) {
                cannyLineNext[nextX] = 255;
                this->trace(width, height, canny, nextX, nextY);
            }

            if (cannyLineNext[nextX] > 0)
                isPoint = false;
        }
    }

    if (isPoint)
        cannyLine[x] = 0;
}

QVector<quint8> HaarDetectorPrivate::hysteresisThresholding(int width, int height,
                                                            const QVector<quint8> &thresholded) const
{
    QVector<quint8> canny = thresholded;

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            this->trace(width, height, canny, x, y);

    for (auto &c: canny)
        if (c == 127)
            c = 0;

    return canny;
}

bool HaarDetectorPrivate::areSimilar(const QRect &r1, const QRect &r2,
                                     qreal eps) const
{
    qreal delta = 0.5 * eps * (qMin(r1.width(), r2.width())
                               + qMin(r1.height(), r2.height()));

    return qAbs(r1.x() - r2.x()) <= delta
           && qAbs(r1.y() - r2.y()) <= delta
           && qAbs(r1.x() + r1.width() - r2.x() - r2.width()) <= delta
           && qAbs(r1.y() + r1.height() - r2.y() - r2.height()) <= delta;
}

void HaarDetectorPrivate::markRectangle(const QVector<QRect> &rectangles,
                                        QVector<int> &labels,
                                        int i, int label, qreal eps) const
{
    labels[i] = label;

    for (int j = 0; j < rectangles.size(); j++)
        if (labels[j] < 0 && this->areSimilar(rectangles[i], rectangles[j], eps))
            this->markRectangle(rectangles, labels, j, label, eps);
}

QVector<int> HaarDetectorPrivate::classifyRectangles(const QVector<QRect> &rectangles,
                                                     qreal eps, int *nClasses) const
{
    QVector<int> labels(rectangles.size(), -1);
    int label = 0;

    for (int i = 0; i < rectangles.size(); i++)
        if (labels[i] < 0) {
            this->markRectangle(rectangles, labels, i, label, eps);
            label++;
        }

    if (nClasses)
        *nClasses = label;

    return labels;
}

RectVector HaarDetectorPrivate::groupRectangles(const RectVector &rects,
                                                int minNeighbors,
                                                qreal eps) const
{
    if (minNeighbors < 1 || rects.isEmpty())
        return rects;

    int nclasses = 0;
    QVector<int> labels = this->classifyRectangles(rects, eps, &nclasses);

    if (nclasses < 1)
        return rects;

    RectVector rrects(nclasses, QRect(0, 0, 0, 0));
    QVector<int> rweights(nclasses, 0);

    for (int i = 0; i < rects.size(); i++ ) {
        int cls = labels[i];

        rrects[cls] = QRect(rrects[cls].x() + rects[i].x(),
                            rrects[cls].y() + rects[i].y(),
                            rrects[cls].width() + rects[i].width(),
                            rrects[cls].height() + rects[i].height());

        rweights[cls]++;
    }

    for (int i = 0; i < nclasses; i++) {
        QRect r = rrects[i];
        qreal s = 1.0 / rweights[i];

        rrects[i] = QRect(qRound(r.x() * s),
                          qRound(r.y() * s),
                          qRound(r.width() * s),
                          qRound(r.height() * s));
    }

    QList<QRect> filtered;

    for (int i = 0; i < nclasses; i++) {
        QRect r1 = rrects[i];
        int n1 = rweights[i];

        // filter out rectangles which don't have enough similar rectangles
        if (n1 <= minNeighbors)
            continue;

        int j;

        // filter out small face rectangles inside large rectangles
        for (j = 0; j < nclasses; j++) {
            int n2 = rweights[j];

            if (j == i || n2 <= minNeighbors)
                continue;

            QRect r2 = rrects[j];

            int dx = qRound(r2.width() * eps);
            int dy = qRound(r2.height() * eps);

            if (i != j
                && r2.left() - r1.left() <= dx
                && r2.top() - r1.top() <= dy
                && r1.right() - r2.right() <= dx
                && r1.bottom() - r2.bottom() <= dy
                && (n2 > qMax(3, n1) || n1 < 3))
                break;
        }

        if (j == nclasses)
            filtered << r1;
    }

    return filtered.toVector();
}

HaarDetector::HaarDetector(QObject *parent): QObject(parent)
{
    this->d = new HaarDetectorPrivate;
    this->d->m_weight = this->d->makeWeightTable(1024);
}

HaarDetector::~HaarDetector()
{
    delete this->d;
}

bool HaarDetector::equalize() const
{
    return this->d->m_equalize;
}

bool &HaarDetector::equalize()
{
    return this->d->m_equalize;
}

int HaarDetector::denoiseRadius() const
{
    return this->d->m_denoiseRadius;
}

int &HaarDetector::denoiseRadius()
{
    return this->d->m_denoiseRadius;
}

int HaarDetector::denoiseMu() const
{
    return this->d->m_denoiseMu;
}

int &HaarDetector::denoiseMu()
{
    return this->d->m_denoiseMu;
}

int HaarDetector::denoiseSigma() const
{
    return this->d->m_denoiseSigma;
}

int &HaarDetector::denoiseSigma()
{
    return this->d->m_denoiseSigma;
}

bool HaarDetector::cannyPruning() const
{
    return this->d->m_cannyPruning;
}

bool &HaarDetector::cannyPruning()
{
    return this->d->m_cannyPruning;
}

qreal HaarDetector::lowCannyThreshold() const
{
    return this->d->m_lowCannyThreshold;
}

qreal &HaarDetector::lowCannyThreshold()
{
    return this->d->m_lowCannyThreshold;
}

qreal HaarDetector::highCannyThreshold() const
{
    return this->d->m_highCannyThreshold;
}

qreal &HaarDetector::highCannyThreshold()
{
    return this->d->m_highCannyThreshold;
}

int HaarDetector::minNeighbors() const
{
    return this->d->m_minNeighbors;
}

int &HaarDetector::minNeighbors()
{
    return this->d->m_minNeighbors;
}

bool HaarDetector::loadCascade(const QString &fileName)
{
    this->d->m_mutex.lock();
    bool r = this->d->m_cascade.load(fileName);
    this->d->m_mutex.unlock();

    return r;
}

QVector<QRect> HaarDetector::detect(const QImage &image, qreal scaleFactor,
                                    QSize minObjectSize, QSize maxObjectSize) const
{
    QVector<quint8> gray;
    this->d->computeGray(image, this->d->m_equalize, gray);

    if (this->d->m_denoiseRadius > 0) {
        QVector<quint8> denoised;
        this->d->denoise(image.width(), image.height(), gray,
                         this->d->m_denoiseRadius,
                         this->d->m_denoiseMu,
                         this->d->m_denoiseSigma,
                         denoised);

        gray = denoised;
    }

    QVector<quint32> integral;
    QVector<quint64> integral2;
    QVector<quint32> tiltedIntegral;
    this->d->computeIntegral(image.width(), image.height(), gray,
                             integral, integral2, tiltedIntegral);

    QVector<quint32> integralCanny;
    bool cannyPruning = this->d->m_cannyPruning;

    if (cannyPruning) {
        QVector<quint8> canny = this->d->canny(image.width(), image.height(), gray);
        this->d->computeIntegral(image.width(), image.height(),
                                 canny, 1, integralCanny);
    }

    if (scaleFactor <= 1)
        scaleFactor = 1.1;

    int oWidth = image.width() + 1;

    const quint32 *p[4];
    const quint64 *pq[4];
    const quint32 *ip[4];
    const quint32 *icp[4];

    QList<QRect> roi;
    QThreadPool threadPool;
    QMutex mutex;
    static const int border = 1;

    if (threadPool.maxThreadCount() < 8)
        threadPool.setMaxThreadCount(8);

    this->d->m_mutex.lock();

    for (qreal scale = 1; ; scale *= scaleFactor) {
        int windowWidth = qRound(scale * this->d->m_cascade.windowSize().width());
        int windowHeight = qRound(scale * this->d->m_cascade.windowSize().height());

        if (windowWidth > image.width()
            || windowHeight > image.height())
            break;

        if (!minObjectSize.isEmpty())
            if (windowWidth < minObjectSize.width()
                || windowHeight < minObjectSize.height())
                continue;

        if (!maxObjectSize.isEmpty())
            if (windowWidth > maxObjectSize.width()
                || windowHeight > maxObjectSize.height())
                break;

        size_t offset0;
        size_t offset1;
        size_t offset2;
        size_t offset3;

        if (cannyPruning) {
            int x = qRound(0.15 * windowWidth);
            int y = qRound(0.15 * windowHeight);
            int width = qRound(0.7 * windowWidth);
            int height = qRound(0.7 * windowHeight);

            offset0 = size_t(x + y * oWidth);
            offset1 = size_t(x + width + y * oWidth);
            offset2 = size_t(x + (y + height) * oWidth);
            offset3 = size_t(x + width + (y + height) * oWidth);

            ip[0] = integral.constData() + offset0;
            ip[1] = integral.constData() + offset1;
            ip[2] = integral.constData() + offset2;
            ip[3] = integral.constData() + offset3;

            icp[0] = integralCanny.constData() + offset0;
            icp[1] = integralCanny.constData() + offset1;
            icp[2] = integralCanny.constData() + offset2;
            icp[3] = integralCanny.constData() + offset3;
        }

        int rectX = qRound(scale * border);
        int rectY = qRound(scale * border);
        int rectWidth = qRound(scale * (this->d->m_cascade.windowSize().width() - 2 * border));
        int rectHeight = qRound(scale * (this->d->m_cascade.windowSize().height() - 2 * border));

        offset0 = size_t(rectX + rectY * oWidth);
        offset1 = size_t(rectX + rectWidth + rectY * oWidth);
        offset2 = size_t(rectX + (rectY + rectHeight) * oWidth);
        offset3 = size_t(rectX + rectWidth + (rectY + rectHeight) * oWidth);

        p[0] = integral.constData() + offset0;
        p[1] = integral.constData() + offset1;
        p[2] = integral.constData() + offset2;
        p[3] = integral.constData() + offset3;

        pq[0] = integral2.constData() + offset0;
        pq[1] = integral2.constData() + offset1;
        pq[2] = integral2.constData() + offset2;
        pq[3] = integral2.constData() + offset3;

        qreal invArea = 1.0 / (rectWidth * rectHeight);
        qreal step = qMax(2.0, scale);

        int startX = 0;
        int startY = 0;
        int endX = qRound((image.width() - windowWidth) / step);
        int endY = qRound((image.height() - windowHeight) / step);

        auto cascade = new HaarCascadeHID(this->d->m_cascade,
                                          startX, endX, startY, endY,
                                          windowWidth, windowHeight,
                                          oWidth,
                                          integral.constData(),
                                          tiltedIntegral.constData(),
                                          step,
                                          invArea,
                                          scale,
                                          cannyPruning,
                                          p, pq, ip, icp,
                                          &roi,
                                          &mutex);

        auto result =
                QtConcurrent::run(&threadPool, HaarCascadeHID::run, cascade);
        Q_UNUSED(result)
    }

    threadPool.waitForDone();
    this->d->m_mutex.unlock();

    return this->d->groupRectangles(roi.toVector(), this->d->m_minNeighbors);
}

void HaarDetector::setEqualize(bool equalize)
{
    if (this->d->m_equalize == equalize)
        return;

    this->d->m_equalize = equalize;
    emit this->equalizeChanged(equalize);
}

void HaarDetector::setDenoiseRadius(int denoiseRadius)
{
    if (this->d->m_denoiseRadius == denoiseRadius)
        return;

    this->d->m_denoiseRadius = denoiseRadius;
    emit this->denoiseRadiusChanged(denoiseRadius);
}

void HaarDetector::setDenoiseMu(int denoiseMu)
{
    if (this->d->m_denoiseMu == denoiseMu)
        return;

    this->d->m_denoiseMu = denoiseMu;
    emit this->denoiseMuChanged(denoiseMu);
}

void HaarDetector::setDenoiseSigma(int denoiseSigma)
{
    if (this->d->m_denoiseSigma == denoiseSigma)
        return;

    this->d->m_denoiseSigma = denoiseSigma;
    emit this->denoiseSigmaChanged(denoiseSigma);
}

void HaarDetector::setCannyPruning(bool cannyPruning)
{
    if (this->d->m_cannyPruning == cannyPruning)
        return;

    this->d->m_cannyPruning = cannyPruning;
    emit this->cannyPruningChanged(cannyPruning);
}

void HaarDetector::setLowCannyThreshold(qreal lowCannyThreshold)
{
    if (qFuzzyCompare(this->d->m_lowCannyThreshold, lowCannyThreshold))
        return;

    this->d->m_lowCannyThreshold = lowCannyThreshold;
    emit this->lowCannyThresholdChanged(lowCannyThreshold);
}

void HaarDetector::setHighCannyThreshold(qreal highCannyThreshold)
{
    if (qFuzzyCompare(this->d->m_highCannyThreshold, highCannyThreshold))
        return;

    this->d->m_highCannyThreshold = highCannyThreshold;
    emit this->highCannyThresholdChanged(highCannyThreshold);
}

void HaarDetector::setMinNeighbors(int minNeighbors)
{
    if (this->d->m_minNeighbors == minNeighbors)
        return;

    this->d->m_minNeighbors = minNeighbors;
    emit this->minNeighborsChanged(minNeighbors);
}

void HaarDetector::resetEqualize()
{
    this->setEqualize(false);
}

void HaarDetector::resetDenoiseRadius()
{
    this->setDenoiseRadius(0);
}

void HaarDetector::resetDenoiseMu()
{
    this->setDenoiseMu(0);
}

void HaarDetector::resetDenoiseSigma()
{
    this->setDenoiseSigma(0);
}

void HaarDetector::resetCannyPruning()
{
    this->setCannyPruning(false);
}

void HaarDetector::resetLowCannyThreshold()
{
    this->setLowCannyThreshold(0);
}

void HaarDetector::resetHighCannyThreshold()
{
    this->setHighCannyThreshold(50);
}

void HaarDetector::resetMinNeighbors()
{
    this->setMinNeighbors(3);
}
