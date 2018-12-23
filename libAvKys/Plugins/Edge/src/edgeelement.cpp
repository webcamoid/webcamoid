/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QImage>
#include <QQmlContext>
#include <QVector>
#include <QtMath>
#include <akutils.h>
#include <akpacket.h>

#include "edgeelement.h"

class EdgeElementPrivate
{
    public:
        bool m_canny {false};
        int m_thLow {510};
        int m_thHi {1020};
        bool m_equalize {false};
        bool m_invert {false};

        QVector<quint8> equalize(const QImage &image);
        void sobel(int width,
                   int height,
                   const QVector<quint8> &gray,
                   QVector<quint16> &gradient,
                   QVector<quint8> &direction) const;
        QVector<quint16> thinning(int width, int height,
                                  const QVector<quint16> &gradient,
                                  const QVector<quint8> &direction) const;
        QVector<quint8> threshold(int width,
                                  int height,
                                  const QVector<quint16> &image,
                                  const QVector<int> &thresholds,
                                  const QVector<int> &map) const;
        void trace(int width,
                   int height,
                   QVector<quint8> &canny,
                   int x,
                   int y) const;
        QVector<quint8> hysteresisThresholding(int width,
                                               int height,
                                               const QVector<quint8> &thresholded) const;
};

EdgeElement::EdgeElement(): AkElement()
{
    this->d = new EdgeElementPrivate;
}

EdgeElement::~EdgeElement()
{
    delete this->d;
}

bool EdgeElement::canny() const
{
    return this->d->m_canny;
}

int EdgeElement::thLow() const
{
    return this->d->m_thLow;
}

int EdgeElement::thHi() const
{
    return this->d->m_thHi;
}

bool EdgeElement::equalize() const
{
    return this->d->m_equalize;
}

bool EdgeElement::invert() const
{
    return this->d->m_invert;
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
    if (this->d->m_canny == canny)
        return;

    this->d->m_canny = canny;
    emit this->cannyChanged(canny);
}

void EdgeElement::setThLow(int thLow)
{
    if (this->d->m_thLow == thLow)
        return;

    this->d->m_thLow = thLow;
    emit this->thLowChanged(thLow);
}

void EdgeElement::setThHi(int thHi)
{
    if (this->d->m_thHi == thHi)
        return;

    this->d->m_thHi = thHi;
    emit this->thHiChanged(thHi);
}

void EdgeElement::setEqualize(bool equalize)
{
    if (this->d->m_equalize == equalize)
        return;

    this->d->m_equalize = equalize;
    emit this->equalizeChanged(equalize);
}

void EdgeElement::setInvert(bool invert)
{
    if (this->d->m_invert == invert)
        return;

    this->d->m_invert = invert;
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

    if (this->d->m_equalize)
        in = this->d->equalize(src);
    else {
        int videoArea = src.width() * src.height();
        in.resize(videoArea);
        memcpy(in.data(), src.constBits(), size_t(videoArea));
    }

    QVector<quint16> gradient;
    QVector<quint8> direction;
    this->d->sobel(src.width(), src.height(), in, gradient, direction);

    if (this->d->m_canny) {
        auto thinned = this->d->thinning(src.width(),
                                         src.height(),
                                         gradient,
                                         direction);

        QVector<int> thresholds(2);
        thresholds[0] = this->d->m_thLow;
        thresholds[1] = this->d->m_thHi;

        QVector<int> colors(3);
        colors[0] = 0;
        colors[1] = 127;
        colors[2] = 255;
        auto thresholded = this->d->threshold(src.width(),
                                              src.height(),
                                              thinned,
                                              thresholds,
                                              colors);
        auto canny = this->d->hysteresisThresholding(src.width(),
                                                     src.height(),
                                                     thresholded);

        for (int y = 0; y < src.height(); y++) {
            const quint8 *srcLine = canny.constData() + y * src.width();
            quint8 *dstLine = oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++)
                dstLine[x] = this->d->m_invert? 255 - srcLine[x]: srcLine[x];
        }
    } else
        for (int y = 0; y < src.height(); y++) {
            const quint16 *srcLine = gradient.constData() + y * src.width();
            quint8 *dstLine = oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++) {
                int gray = qBound<int>(0, srcLine[x], 255);
                dstLine[x] = this->d->m_invert? quint8(255 - gray): quint8(gray);
            }
        }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

QVector<quint8> EdgeElementPrivate::equalize(const QImage &image)
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

void EdgeElementPrivate::sobel(int width,
                               int height,
                               const QVector<quint8> &gray,
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

QVector<quint16> EdgeElementPrivate::thinning(int width,
                                              int height,
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

QVector<quint8> EdgeElementPrivate::threshold(int width,
                                              int height,
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

QVector<quint8> EdgeElementPrivate::hysteresisThresholding(int width,
                                                           int height,
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

void EdgeElementPrivate::trace(int width,
                               int height,
                               QVector<quint8> &canny,
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

#include "moc_edgeelement.cpp"
