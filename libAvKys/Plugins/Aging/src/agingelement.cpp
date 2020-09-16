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
#include <QVector>
#include <QTime>
#include <QMutex>
#include <QQmlContext>
#include <QRandomGenerator>
#include <akpacket.h>
#include <akvideopacket.h>

#include "agingelement.h"
#include "scratch.h"

class AgingElementPrivate
{
    public:
        QVector<Scratch> m_scratches;
        QMutex m_mutex;
        bool m_addDust {true};

        QImage colorAging(const QImage &src);
        void scratching(QImage &dest);
        void pits(QImage &dest);
        void dusts(QImage &dest);
};

AgingElement::AgingElement():
    AkElement()
{
    this->d = new AgingElementPrivate;
    this->d->m_scratches.resize(7);
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

AkPacket AgingElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    oFrame = this->d->colorAging(oFrame);
    this->d->scratching(oFrame);
    this->d->pits(oFrame);

    if (this->d->m_addDust)
        this->d->dusts(oFrame);

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void AgingElement::setNScratches(int nScratches)
{
    if (this->d->m_scratches.size() == nScratches)
        return;

    this->d->m_mutex.lock();
    this->d->m_scratches.resize(nScratches);
    this->d->m_mutex.unlock();
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

QImage AgingElementPrivate::colorAging(const QImage &src)
{
    QImage dst(src.size(), src.format());
    int luma = QRandomGenerator::global()->bounded(-32, -25);

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int c = QRandomGenerator::global()->bounded(24);
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

void AgingElementPrivate::scratching(QImage &dest)
{
    QMutexLocker locker(&this->m_mutex);

    for (auto &scratch: this->m_scratches) {
        if (scratch.life() < 1.0) {
            if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.06 * RAND_MAX) {
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

        int luma = QRandomGenerator::global()->bounded(32, 40);
        int x = int(scratch.x());

        int y1 = scratch.y();
        int y2 = scratch.isAboutToDie()?
                     QRandomGenerator::global()->bounded(dest.height()):
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

void AgingElementPrivate::pits(QImage &dest)
{
    int pnumscale = qRound(0.03 * qMax(dest.width(), dest.height()));
    static int pitsInterval = 0;
    int pnum = QRandomGenerator::global()->bounded(pnumscale);

    if (pitsInterval) {
        pnum += pnumscale;
        pitsInterval--;
    } else if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.03 * RAND_MAX) {
        pitsInterval = QRandomGenerator::global()->bounded(20, 36);
    }

    for (int i = 0; i < pnum; i++) {
        int x = QRandomGenerator::global()->bounded(dest.width());
        int y = QRandomGenerator::global()->bounded(dest.height());
        int size = QRandomGenerator::global()->bounded(16);

        for (int j = 0; j < size; j++) {
            x += QRandomGenerator::global()->bounded(-1, 2);
            y += QRandomGenerator::global()->bounded(-1, 2);

            if (x < 0 || x >= dest.width()
                || y < 0 || y >= dest.height())
                continue;

            QRgb *line = reinterpret_cast<QRgb *>(dest.scanLine(y));
            line[x] = qRgb(192, 192, 192);
        }
    }
}

void AgingElementPrivate::dusts(QImage &dest)
{
    static int dustInterval = 0;

    if (dustInterval == 0) {
        if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.03 * RAND_MAX)
            dustInterval = QRandomGenerator::global()->bounded(8);

        return;
    }

    dustInterval--;
    int areaScale = qRound(0.02 * qMax(dest.width(), dest.height()));
    int dnum = 4 * areaScale + QRandomGenerator::global()->bounded(32);

    for (int i = 0; i < dnum; i++) {
        int x = QRandomGenerator::global()->bounded(dest.width());
        int y = QRandomGenerator::global()->bounded(dest.height());
        int size = QRandomGenerator::global()->bounded(areaScale) + 5;

        for (int j = 0; j < size; j++) {
            x += QRandomGenerator::global()->bounded(-1, 2);
            y += QRandomGenerator::global()->bounded(-1, 2);

            if (x < 0 || x >= dest.width()
                || y < 0 || y >= dest.height())
                continue;

            QRgb *line = reinterpret_cast<QRgb *>(dest.scanLine(y));
            line[x] = qRgb(16, 16, 16);
        }
    }
}

#include "moc_agingelement.cpp"
