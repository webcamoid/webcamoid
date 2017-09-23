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

#include <QtMath>

#include "edgeelement.h"

EdgeElement::EdgeElement(): AkElement()
{
    this->m_canny = false;
    this->m_thLow = 510;
    this->m_thHi = 1020;
    this->m_equalize = false;
    this->m_invert = false;
}

EdgeElement::~EdgeElement()
{
}

bool EdgeElement::canny() const
{
    return this->m_canny;
}

int EdgeElement::thLow() const
{
    return this->m_thLow;
}

int EdgeElement::thHi() const
{
    return this->m_thHi;
}

bool EdgeElement::equalize() const
{
    return this->m_equalize;
}

bool EdgeElement::invert() const
{
    return this->m_invert;
}

QVector<quint8> EdgeElement::equalize(const QImage &image)
{
    int videoArea = image.width() * image.height();
    const quint8 *imgPtr = image.constBits();
    QVector<quint8> out(videoArea);
    quint8 *outPtr = out.data();
    int minGray = 255;
    int maxGray = 0;

    for (int i = 0; i < videoArea; i++) {
        if (imgPtr[i] < minGray)
            minGray = imgPtr[i];

        if (imgPtr[i] > maxGray)
            maxGray = imgPtr[i];
    }

    if (maxGray == minGray)
        memset(outPtr, minGray, size_t(videoArea));
    else {
        int diffGray = maxGray - minGray;

        for (int i = 0; i < videoArea; i++)
            outPtr[i] = quint8(255 * (imgPtr[i] - minGray) / diffGray);
    }

    return out;
}

void EdgeElement::sobel(int width, int height, const QVector<quint8> &gray,
                        QVector<quint16> &gradient,
                        QVector<quint8> &direction) const
{
    gradient.resize(gray.size());
    direction.resize(gray.size());

    for (int y = 0; y < height; y++) {
        int yOffset = y * width;
        const quint8 *grayLine = gray.constData() + yOffset;

        const quint8 *grayLine_m1 = y < 1? grayLine: grayLine - width;
        const quint8 *grayLine_p1 = y >= height - 1? grayLine: grayLine + width;

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

QVector<quint16> EdgeElement::thinning(int width, int height,
                                       const QVector<quint16> &gradient,
                                       const QVector<quint8> &direction) const
{
    QVector<quint16> thinned(gradient.size(), 0);

    for (int y = 0; y < height; y++) {
        int yOffset = y * width;
        const quint16 *edgesLine = gradient.constData() + yOffset;
        const quint16 *edgesLine_m1 = y < 1? edgesLine: edgesLine - width;
        const quint16 *edgesLine_p1 = y >= height - 1? edgesLine: edgesLine + width;
        const quint8 *edgesAngleLine = direction.constData() + yOffset;
        quint16 *thinnedLine = thinned.data() + yOffset;

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

QVector<quint8> EdgeElement::threshold(int width, int height,
                                       const QVector<quint16> &image,
                                       const QVector<int> &thresholds,
                                       const QVector<int> &map) const
{
    int size = width * height;
    const quint16 *in = image.constData();
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

void EdgeElement::trace(int width, int height, QVector<quint8> &canny,
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

QVector<quint8> EdgeElement::hysteresisThresholding(int width, int height,
                                                    const QVector<quint8> &thresholded) const
{
    QVector<quint8> canny = thresholded;

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            this->trace(width, height, canny, x, y);

    for (int i = 0; i < canny.size(); i++)
        if (canny[i] == 127)
            canny[i] = 0;

    return canny;
}

QString EdgeElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Edge/share/qml/main.qml");
}

void EdgeElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Edge", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void EdgeElement::setCanny(bool canny)
{
    if (this->m_canny == canny)
        return;

    this->m_canny = canny;
    emit this->cannyChanged(canny);
}

void EdgeElement::setThLow(int thLow)
{
    if (this->m_thLow == thLow)
        return;

    this->m_thLow = thLow;
    emit this->thLowChanged(thLow);
}

void EdgeElement::setThHi(int thHi)
{
    if (this->m_thHi == thHi)
        return;

    this->m_thHi = thHi;
    emit this->thHiChanged(thHi);
}

void EdgeElement::setEqualize(bool equalize)
{
    if (this->m_equalize == equalize)
        return;

    this->m_equalize = equalize;
    emit this->equalizeChanged(equalize);
}

void EdgeElement::setInvert(bool invert)
{
    if (this->m_invert == invert)
        return;

    this->m_invert = invert;
    emit this->invertChanged(invert);
}

void EdgeElement::resetCanny()
{
    this->setCanny(false);
}

void EdgeElement::resetThLow()
{
    this->setThLow(510);
}

void EdgeElement::resetThHi()
{
    this->setThHi(1020);
}

void EdgeElement::resetEqualize()
{
    this->setEqualize(false);
}

void EdgeElement::resetInvert()
{
    this->setInvert(false);
}

AkPacket EdgeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_Grayscale8);
    QImage oFrame(src.size(), src.format());

    QVector<quint8> in;

    if (this->m_equalize)
        in = this->equalize(src);
    else {
        int videoArea = src.width() * src.height();
        in.resize(videoArea);
        memcpy(in.data(), src.constBits(), size_t(videoArea));
    }

    QVector<quint16> gradient;
    QVector<quint8> direction;
    this->sobel(src.width(), src.height(), in, gradient, direction);

    if (this->m_canny) {
        QVector<quint16> thinned = this->thinning(src.width(), src.height(),
                                                  gradient, direction);

        QVector<int> thresholds(2);
        thresholds[0] = this->m_thLow;
        thresholds[1] = this->m_thHi;

        QVector<int> colors(3);
        colors[0] = 0;
        colors[1] = 127;
        colors[2] = 255;
        QVector<quint8> thresholded = this->threshold(src.width(), src.height(),
                                                      thinned, thresholds, colors);

        QVector<quint8> canny = this->hysteresisThresholding(src.width(), src.height(),
                                                             thresholded);

        for (int y = 0; y < src.height(); y++) {
            const quint8 *srcLine = canny.constData() + y * src.width();
            quint8 *dstLine = oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++)
                dstLine[x] = this->m_invert? 255 - srcLine[x]: srcLine[x];
        }
    } else
        for (int y = 0; y < src.height(); y++) {
            const quint16 *srcLine = gradient.constData() + y * src.width();
            quint8 *dstLine = oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++) {
                int gray = qBound<int>(0, srcLine[x], 255);
                dstLine[x] = this->m_invert? quint8(255 - gray): quint8(gray);
            }
        }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
