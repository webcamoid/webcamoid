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

#include <QMutex>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QTime>
#include <QVector>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "agingelement.h"
#include "scratch.h"

class AgingElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QVector<Scratch> m_scratches;
        QMutex m_mutex;
        bool m_addDust {true};

        AkVideoPacket colorAging(const AkVideoPacket &src);
        void scratching(AkVideoPacket &dst);
        void pits(AkVideoPacket &dest);
        void dusts(AkVideoPacket &dest);
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
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto dst = this->d->colorAging(src);
    this->d->scratching(dst);
    this->d->pits(dst);

    if (this->d->m_addDust)
        this->d->dusts(dst);

    if (dst)
        emit this->oStream(dst);

    return dst;
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

AkVideoPacket AgingElementPrivate::colorAging(const AkVideoPacket &src)
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    int luma = QRandomGenerator::global()->bounded(-32, -25);

    for (int y = 0; y < src.caps().height(); ++y) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); ++x) {
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

void AgingElementPrivate::scratching(AkVideoPacket &dst)
{
    QMutexLocker locker(&this->m_mutex);

    for (auto &scratch: this->m_scratches) {
        if (scratch.life() < 1.0) {
            if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.06 * RAND_MAX) {
                scratch = Scratch(2.0, 33.0,
                                  1.0, 1.0,
                                  0.0, dst.caps().width() - 1,
                                  0.0, 512.0,
                                  0, dst.caps().height() - 1);
            } else {
                continue;
            }
        }

        if (scratch.x() < 0.0 || scratch.x() >= dst.caps().width()) {
            ++scratch;

            continue;
        }

        int luma = QRandomGenerator::global()->bounded(32, 40);
        int x = int(scratch.x());

        int y1 = scratch.y();
        int y2 = scratch.isAboutToDie()?
                     QRandomGenerator::global()->bounded(dst.caps().height()):
                     dst.caps().height();

        for (int y = y1; y < y2; ++y) {
            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));
            int r = qRed(line[x]) + luma;
            int g = qGreen(line[x]) + luma;
            int b = qBlue(line[x]) + luma;

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }

        ++scratch;
    }
}

void AgingElementPrivate::pits(AkVideoPacket &dst)
{
    int pnumscale = qRound(0.03 * qMax(dst.caps().width(),
                                       dst.caps().height()));
    static int pitsInterval = 0;
    int pnum = QRandomGenerator::global()->bounded(pnumscale);

    if (pitsInterval) {
        pnum += pnumscale;
        pitsInterval--;
    } else if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.03 * RAND_MAX) {
        pitsInterval = QRandomGenerator::global()->bounded(20, 36);
    }

    for (int i = 0; i < pnum; ++i) {
        int x = QRandomGenerator::global()->bounded(dst.caps().width());
        int y = QRandomGenerator::global()->bounded(dst.caps().height());
        int size = QRandomGenerator::global()->bounded(16);

        for (int j = 0; j < size; ++j) {
            x += QRandomGenerator::global()->bounded(-1, 2);
            y += QRandomGenerator::global()->bounded(-1, 2);

            if (x < 0 || x >= dst.caps().width()
                || y < 0 || y >= dst.caps().height())
                continue;

            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));
            line[x] = qRgb(192, 192, 192);
        }
    }
}

void AgingElementPrivate::dusts(AkVideoPacket &dst)
{
    static int dustInterval = 0;

    if (dustInterval == 0) {
        if (QRandomGenerator::global()->bounded(RAND_MAX) <= 0.03 * RAND_MAX)
            dustInterval = QRandomGenerator::global()->bounded(8);

        return;
    }

    dustInterval--;
    int areaScale = qRound(0.02 * qMax(dst.caps().width(),
                                       dst.caps().height()));
    int dnum = 4 * areaScale + QRandomGenerator::global()->bounded(32);

    for (int i = 0; i < dnum; ++i) {
        int x = QRandomGenerator::global()->bounded(dst.caps().width());
        int y = QRandomGenerator::global()->bounded(dst.caps().height());
        int size = QRandomGenerator::global()->bounded(areaScale) + 5;

        for (int j = 0; j < size; ++j) {
            x += QRandomGenerator::global()->bounded(-1, 2);
            y += QRandomGenerator::global()->bounded(-1, 2);

            if (x < 0 || x >= dst.caps().width()
                || y < 0 || y >= dst.caps().height())
                continue;

            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));
            line[x] = qRgb(16, 16, 16);
        }
    }
}

#include "moc_agingelement.cpp"
