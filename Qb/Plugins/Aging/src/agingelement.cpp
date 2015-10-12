/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QTime>

#include "agingelement.h"

AgingElement::AgingElement(): QbElement()
{
    this->m_scratches.resize(7);
    this->m_addDust = true;

    qsrand(QTime::currentTime().msec());
}

QObject *AgingElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Aging/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Aging", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int AgingElement::nScratches() const
{
    return this->m_scratches.size();
}

int AgingElement::addDust() const
{
    return this->m_addDust;
}

QImage AgingElement::colorAging(const QImage &src)
{
    QImage dest(src.size(), src.format());

    int lumaVariance = 8;
    int colorVariance = 24;
    int luma = -32 + qrand() % lumaVariance;

    const QRgb *srcBits = (const QRgb *) src.constBits();
    QRgb *destBits = (QRgb *) dest.bits();
    int videoArea = dest.width() * dest.height();

    for (int i = 0; i < videoArea; i++) {
        int c = qrand() % colorVariance;
        int r = qRed(srcBits[i]) + luma + c;
        int g = qGreen(srcBits[i]) + luma + c;
        int b = qBlue(srcBits[i]) + luma + c;

        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);

        destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
    }

    return dest;
}

void AgingElement::scratching(QImage &dest)
{
    QMutexLocker locker(&this->m_mutex);

    for (int i = 0; i < this->m_scratches.size(); i++) {
        if (this->m_scratches[i].life() < 1.0) {
            if (qrand() <= 0.06 * RAND_MAX) {
                this->m_scratches[i] = Scratch(2.0, 33.0,
                                               1.0, 1.0,
                                               0.0, dest.width() - 1,
                                               0.0, 512.0,
                                               0.0, dest.height() - 1);
            } else
                continue;
        }

        if (this->m_scratches[i].x() < 0.0
            || this->m_scratches[i].x() >= dest.width()) {
            this->m_scratches[i]++;

            continue;
        }

        int lumaVariance = 8;
        int luma = 32 + qrand() % lumaVariance;
        int x = this->m_scratches[i].x();

        int y1 = this->m_scratches[i].y();
        int y2 = this->m_scratches[i].isAboutToDie()?
                     qrand() % dest.height():
                     dest.height();

        for (int y = y1; y < y2; y++) {
            QRgb *line = (QRgb *) dest.scanLine(y);
            int r = qRed(line[x]) + luma;
            int g = qGreen(line[x]) + luma;
            int b = qBlue(line[x]) + luma;

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }

        this->m_scratches[i]++;
    }
}

void AgingElement::pits(QImage &dest)
{
    int pnum;
    int pnumscale = 0.03 * qMax(dest.width(), dest.height());
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

            QRgb *line = (QRgb *) dest.scanLine(y);
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

    int areaScale = 0.02 * qMax(dest.width(), dest.height());
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

            QRgb *line = (QRgb *) dest.scanLine(y);
            line[x] = qRgb(16, 16, 16);
        }
    }
}

void AgingElement::setNScratches(int nScratches)
{
    if (this->m_scratches.size() == nScratches)
        return;

    QMutexLocker locker(&this->m_mutex);
    this->m_scratches.resize(nScratches);
    emit this->nScratchesChanged(nScratches);
}

void AgingElement::setAddDust(int addDust)
{
    if (this->m_addDust == addDust)
        return;

    this->m_addDust = addDust;
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

QbPacket AgingElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    oFrame = this->colorAging(oFrame);
    this->scratching(oFrame);
    this->pits(oFrame);

    if (this->m_addDust)
        this->dusts(oFrame);

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
