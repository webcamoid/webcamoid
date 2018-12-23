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

#include <cstdlib>
#include <QImage>
#include <QVector>
#include <QTime>
#include <QMutex>
#include <QQmlContext>
#include <akutils.h>
#include <akpacket.h>

#include "agingelement.h"
#include "scratch.h"

class AgingElementPrivate
{
    public:
        QVector<Scratch> m_scratches;
        QMutex m_mutex;
        bool m_addDust {true};
};

AgingElement::AgingElement():
    AkElement()
{
    this->d = new AgingElementPrivate;
    this->d->m_scratches.resize(7);

    qsrand(uint(QTime::currentTime().msec()));
}

AgingElement::~AgingElement()
{
    delete this->d;
}

int AgingElement::nScratches() const
{
    return this->d->m_scratches.size();
}

bool AgingElement::addDust() const
{
    return this->d->m_addDust;
}

QImage AgingElement::colorAging(const QImage &src)
{
    QImage dst(src.size(), src.format());

    int lumaVariance = 8;
    int colorVariance = 24;
    int luma = -32 + qrand() % lumaVariance;

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int c = qrand() % colorVariance;
            int r = qRed(srcLine[x]) + luma + c;
            int g = qGreen(srcLine[x]) + luma + c;
            int b = qBlue(srcLine[x]) + luma + c;

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            dstLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
    }

    return dst;
}

void AgingElement::scratching(QImage &dest)
{
    QMutexLocker locker(&this->d->m_mutex);

    for (auto &scratch: this->d->m_scratches) {
        if (scratch.life() < 1.0) {
            if (qrand() <= 0.06 * RAND_MAX) {
                scratch = Scratch(2.0, 33.0,
                                  1.0, 1.0,
                                  0.0, dest.width() - 1,
                                  0.0, 512.0,
                                  0, dest.height() - 1);
            } else {
                continue;
            }
        }

        if (scratch.x() < 0.0 || scratch.x() >= dest.width()) {
            scratch++;

            continue;
        }

        int lumaVariance = 8;
        int luma = 32 + qrand() % lumaVariance;
        int x = int(scratch.x());

        int y1 = scratch.y();
        int y2 = scratch.isAboutToDie()?
                     qrand() % dest.height():
                     dest.height();

        for (int y = y1; y < y2; y++) {
            QRgb *line = reinterpret_cast<QRgb *>(dest.scanLine(y));
            int r = qRed(line[x]) + luma;
            int g = qGreen(line[x]) + luma;
            int b = qBlue(line[x]) + luma;

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }

        scratch++;
    }
}

void AgingElement::pits(QImage &dest)
{
    int pnum;
    int pnumscale = int(0.03 * qMax(dest.width(), dest.height()));
    static int pitsInterval = 0;

    if (pitsInterval) {
        pnum = pnumscale + (qrand() % pnumscale);
        pitsInterval--;
    } else {
        pnum = qrand() % pnumscale;

        if (qrand() <= 0.03 * RAND_MAX)
            pitsInterval = (qrand() % 16) + 20;
    }

    for (int i = 0; i < pnum; i++) {
        int x = qrand() % (dest.width() - 1);
        int y = qrand() % (dest.height() - 1);
        int size = qrand() % 16;

        for (int j = 0; j < size; j++) {
            x += qrand() % 3 - 1;
            y += qrand() % 3 - 1;

            if (x < 0 || x >= dest.width()
                || y < 0 || y >= dest.height())
                continue;

            QRgb *line = reinterpret_cast<QRgb *>(dest.scanLine(y));
            line[x] = qRgb(192, 192, 192);
        }
    }
}

void AgingElement::dusts(QImage &dest)
{
    static int dustInterval = 0;

    if (dustInterval == 0) {
        if (qrand() <= 0.03 * RAND_MAX)
            dustInterval = qrand() % 8;

        return;
    }

    dustInterval--;

    int areaScale = int(0.02 * qMax(dest.width(), dest.height()));
    int dnum = areaScale * 4 + (qrand() % 32);

    for (int i = 0; i < dnum; i++) {
        int x = qrand() % (dest.width() - 1);
        int y = qrand() % (dest.height() - 1);
        int len = qrand() % areaScale + 5;

        for (int j = 0; j < len; j++) {
            x += qrand() % 3 - 1;
            y += qrand() % 3 - 1;

            if (x < 0 || x >= dest.width()
                || y < 0 || y >= dest.height())
                continue;

            QRgb *line = reinterpret_cast<QRgb *>(dest.scanLine(y));
            line[x] = qRgb(16, 16, 16);
        }
    }
}

QString AgingElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Aging/share/qml/main.qml");
}

void AgingElement::controlInterfaceConfigure(QQmlContext *context,
                                             const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Aging", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void AgingElement::setNScratches(int nScratches)
{
    if (this->d->m_scratches.size() == nScratches)
        return;

    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_scratches.resize(nScratches);
    emit this->nScratchesChanged(nScratches);
}

void AgingElement::setAddDust(bool addDust)
{
    if (this->d->m_addDust == addDust)
        return;

    this->d->m_addDust = addDust;
    emit this->addDustChanged(addDust);
}

void AgingElement::resetNScratches()
{
    this->setNScratches(7);
}

void AgingElement::resetAddDust()
{
    this->setAddDust(true);
}

AkPacket AgingElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    oFrame = this->colorAging(oFrame);
    this->scratching(oFrame);
    this->pits(oFrame);

    if (this->d->m_addDust)
        this->dusts(oFrame);

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_agingelement.cpp"
