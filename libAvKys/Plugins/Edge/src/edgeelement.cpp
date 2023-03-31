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

#include <QQmlContext>
#include <QVector>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "edgeelement.h"

class EdgeElementPrivate
{
    public:
        int m_thLow {510};
        int m_thHi {1020};
        bool m_canny {false};
        bool m_equalize {false};
        bool m_invert {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_graya8pack, 0, 0, {}}};

        AkVideoPacket equalize(const AkVideoPacket &src);
        void sobel(const AkVideoPacket &gray,
                   AkVideoPacket &gradient,
                   AkVideoPacket &direction) const;
        AkVideoPacket thinning(const AkVideoPacket &gradient,
                               const AkVideoPacket &direction) const;
        AkVideoPacket threshold(const AkVideoPacket &thinned,
                                const QVector<int> &thresholds,
                                const QVector<int> &map) const;
        void trace(AkVideoPacket &canny,
                   int x,
                   int y) const;
        AkVideoPacket hysteresisThresholding(const AkVideoPacket &thresholded) const;
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

AkPacket EdgeElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    AkVideoPacket src_;

    if (this->d->m_equalize)
        src_ = this->d->equalize(src);
    else
        src_ = src;

    AkVideoPacket gradient;
    AkVideoPacket direction;
    this->d->sobel(src_, gradient, direction);
    auto invert = this->d->m_invert;

    if (this->d->m_canny) {
        auto thinned = this->d->thinning(gradient, direction);
        QVector<int> thresholds {this->d->m_thLow, this->d->m_thHi};
        QVector<int> colors {0, 127, 255};
        auto thresholded = this->d->threshold(thinned, thresholds, colors);
        auto canny = this->d->hysteresisThresholding(thresholded);

        for (int y = 0; y < src.caps().height(); y++) {
            auto cannyLine = canny.constLine(0, y);
            auto srcLine = reinterpret_cast<const quint16 *>(src_.constLine(0, y));
            auto dstLine = reinterpret_cast<quint16 *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++) {
                auto &pixel = cannyLine[x];
                quint16 y = invert? 255 - pixel: pixel;
                quint16 a = srcLine[x] & 0xff;
                dstLine[x] = y << 8 | a;
            }
        }
    } else {
        for (int y = 0; y < src.caps().height(); y++) {
            auto gradientLine = reinterpret_cast<const quint16 *>(gradient.constLine(0, y));
            auto srcLine = reinterpret_cast<const quint16 *>(src_.constLine(0, y));
            auto dstLine = reinterpret_cast<quint16 *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++) {
                auto pixel = quint8(qBound<int>(0, gradientLine[x], 255));
                quint16 y = invert? 255 - pixel: pixel;
                quint16 a = srcLine[x] & 0xff;
                dstLine[x] = y << 8 | a;
            }
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
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

AkVideoPacket EdgeElementPrivate::equalize(const AkVideoPacket &src)
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    int minGray = 255;
    int maxGray = 0;

    for (int y = 0; y < src.caps().height(); y++) {
        auto line = reinterpret_cast<const quint16 *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto gray = line[x] >> 8;

            if (gray < minGray)
                minGray = gray;

            if (gray > maxGray)
                maxGray = gray;
        }
    }

    if (maxGray == minGray) {
        for (int y = 0; y < src.caps().height(); y++) {
            auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
            auto dstLine = reinterpret_cast<quint16 *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++)
                dstLine[x] = minGray << 8 | srcLine[x] & 0xff;
        }
    } else {
        int diffGray = maxGray - minGray;
        quint8 colorTable[256];

        for (int i = 0; i < 256; i++)
            colorTable[i] = quint8(255 * (i - minGray) / diffGray);

        for (int y = 0; y < src.caps().height(); y++) {
            auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
            auto dstLine = reinterpret_cast<quint16 *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++) {
                auto &pixel = srcLine[x];
                auto y = pixel >> 8;
                auto a = pixel & 0xff;
                dstLine[x] = colorTable[y] << 8 | a;
            }
        }
    }

    return dst;
}

void EdgeElementPrivate::sobel(const AkVideoPacket &gray,
                               AkVideoPacket &gradient,
                               AkVideoPacket &direction) const
{
    auto caps = gray.caps();
    caps.setFormat(AkVideoCaps::Format_gray16);
    gradient = {caps};
    gradient.copyMetadata(gray);
    caps.setFormat(AkVideoCaps::Format_gray8);
    direction = {caps};
    direction.copyMetadata(gray);

    auto width_1 = gray.caps().width() - 1;
    auto height_1 = gray.caps().height() - 1;

    for (int y = 0; y < gray.caps().height(); y++) {
        auto grayLine = reinterpret_cast<const quint16 *>(gray.constLine(0, y));
        auto grayLine_m1 = reinterpret_cast<const quint16 *>(gray.constLine(0, qMax(y - 1, 0)));
        auto grayLine_p1 = reinterpret_cast<const quint16 *>(gray.constLine(0, qMin(y + 1, height_1)));

        auto gradientLine  = reinterpret_cast<quint16 *>(gradient.line(0, y));
        auto directionLine = direction.line(0, y);

        for (int x = 0; x < gray.caps().width(); x++) {
            int x_m1 = qMax(x - 1, 0);
            int x_p1 = qMin(x + 1,  width_1);

            int pixel_m1_p1 = grayLine_m1[x_p1] >> 8;
            int pixel_p1_p1 = grayLine_p1[x_p1] >> 8;
            int pixel_m1_m1 = grayLine_m1[x_m1] >> 8;
            int pixel_p1_m1 = grayLine_p1[x_m1] >> 8;

            int gradX = pixel_m1_p1
                      + 2 * int(grayLine[x_p1] >> 8)
                      + pixel_p1_p1
                      - pixel_m1_m1
                      - 2 * int(grayLine[x_m1] >> 8)
                      - pixel_p1_m1;

            int gradY = pixel_m1_m1
                      + 2 * int(grayLine_m1[x] >> 8)
                      + pixel_m1_p1
                      - pixel_p1_m1
                      - 2 * int(grayLine_p1[x] >> 8)
                      - pixel_p1_p1;

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

AkVideoPacket EdgeElementPrivate::thinning(const AkVideoPacket &gradient,
                                           const AkVideoPacket &direction) const
{
    AkVideoPacket thinned(gradient.caps(), true);
    thinned.copyMetadata(gradient);

    auto width_1 = gradient.caps().width() - 1;
    auto height_1 = gradient.caps().height() - 1;

    for (int y = 0; y < gradient.caps().height(); y++) {
        auto edgesLine = reinterpret_cast<const quint16 *>(gradient.constLine(0, y));
        auto edgesLine_m1 = reinterpret_cast<const quint16 *>(gradient.constLine(0, qMax(y - 1, 0)));
        auto edgesLine_p1 = reinterpret_cast<const quint16 *>(gradient.constLine(0, qMin(y + 1, height_1)));

        auto edgesAngleLine = direction.constLine(0, y);
        auto thinnedLine = reinterpret_cast<quint16 *>(thinned.line(0, y));

        for (int x = 0; x < gradient.caps().width(); x++) {
            int x_m1 = qMax(x - 1, 0);
            int x_p1 = qMin(x + 1,  width_1);

            auto &direction = edgesAngleLine[x];

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

AkVideoPacket EdgeElementPrivate::threshold(const AkVideoPacket &thinned,
                                            const QVector<int> &thresholds,
                                            const QVector<int> &map) const
{
    auto caps = thinned.caps();
    caps.setFormat(AkVideoCaps::Format_gray8);
    AkVideoPacket out(caps);
    out.copyMetadata(thinned);

    for (int y = 0; y < thinned.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(thinned.constLine(0, y));
        auto dstLine = out.line(0, y);

        for (int x = 0; x < thinned.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int value = -1;

            for (int j = 0; j < thresholds.size(); j++)
                if (pixel <= thresholds[j]) {
                    value = map[j];

                    break;
                }

            dstLine[x] = quint8(value < 0? map[thresholds.size()]: value);
        }
    }

    return out;
}

AkVideoPacket EdgeElementPrivate::hysteresisThresholding(const AkVideoPacket &thresholded) const
{
    auto canny = thresholded;

    for (int y = 0; y < canny.caps().height(); y++)
        for (int x = 0; x < canny.caps().width(); x++)
            this->trace(canny, x, y);

    for (int y = 0; y < canny.caps().height(); y++) {
        auto line = canny.line(0, y);

        for (int x = 0; x < canny.caps().width(); x++) {
            auto &pixel = line[x];

            if (pixel == 127)
                pixel = 0;
        }
    }

    return canny;
}

void EdgeElementPrivate::trace(AkVideoPacket &canny, int x, int y) const
{
    auto cannyLine = canny.line(0, y);

    if (cannyLine[x] != 255)
        return;

    auto lineSize = canny.lineSize(0);
    bool isPoint = true;

    for (int j = -1; j < 2; j++) {
        int nextY = y + j;

        if (nextY < 0 || nextY >= canny.caps().height())
            continue;

        auto cannyLineNext = cannyLine + j * lineSize;

        for (int i = -1; i < 2; i++) {
            int nextX = x + i;

            if (i == 0 && j == 0)
                continue;

            if (nextX < 0 || nextX >= canny.caps().width())
                continue;

            auto &pixel = cannyLineNext[nextX];

            if (pixel == 127) {
                pixel = 255;
                this->trace(canny, nextX, nextY);
            }

            if (pixel > 0)
                isPoint = false;
        }
    }

    if (isPoint)
        cannyLine[x] = 0;
}

#include "moc_edgeelement.cpp"
