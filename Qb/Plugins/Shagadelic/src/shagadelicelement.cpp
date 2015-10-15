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

#include <QtMath>

#include "shagadelicelement.h"

ShagadelicElement::ShagadelicElement(): QbElement()
{
    this->m_mask = 0xffffff;
}

QObject *ShagadelicElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Shagadelic/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Shagadelic", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

quint32 ShagadelicElement::mask() const
{
    return this->m_mask;
}

QImage ShagadelicElement::makeRipple(const QSize &size) const
{
    QImage ripple(2 * size, QImage::Format_Grayscale8);

    for (int y = 0; y < ripple.height(); y++) {
        qreal yy = qreal(y) / size.width() - 1.0;
        quint8 *oLine = (quint8 *) ripple.scanLine(y);

        for (int x = 0; x < ripple.width(); x++) {
            qreal xx = qreal(x) / size.width() - 1.0;
            oLine[x] = ((unsigned int) (sqrt(xx * xx + yy * yy) * 3000)) & 255;
        }
    }

    return ripple;
}

QImage ShagadelicElement::makeSpiral(const QSize &size) const
{
    QImage spiral(size, QImage::Format_Grayscale8);
    int yc = spiral.height() / 2;

    for (int y = 0; y < spiral.height(); y++) {
        qreal yy = qreal(y - yc) / spiral.width();
        quint8 *oLine = (quint8 *) spiral.scanLine(y);

        for (int x = 0; x < spiral.width(); x++) {
            qreal xx = qreal(x) / spiral.width() - 0.5;

            oLine[x] = ((unsigned int) (256 * 9 * atan2(xx, yy) / M_PI
                                        + 1800 * sqrt(xx * xx + yy * yy)))
                       & 255;
        }
    }

    return spiral;
}

void ShagadelicElement::init(const QSize &size)
{
    this->m_ripple = this->makeRipple(size);
    this->m_spiral = this->makeSpiral(size);

    this->m_rx = qrand() % size.width();
    this->m_ry = qrand() % size.height();
    this->m_bx = qrand() % size.width();
    this->m_by = qrand() % size.height();

    this->m_rvx = -2;
    this->m_rvy = -2;
    this->m_bvx = 2;
    this->m_bvy = 2;

    this->m_phase = 0;
}

void ShagadelicElement::setMask(quint32 mask)
{
    if (this->m_mask == mask)
        return;

    this->m_mask = mask;
    emit this->maskChanged(mask);
}

void ShagadelicElement::resetMask()
{
    this->setMask(0xffffff);
}

QbPacket ShagadelicElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = QImage(src.size(), src.format());

    if (src.size() != this->m_curSize) {
        this->init(src.size());
        this->m_curSize = src.size();
    }

    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = (const QRgb *) src.constScanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);
        const quint8 *rLine = (const quint8 *) this->m_ripple.constScanLine(y + this->m_ry);
        const quint8 *gLine = (const quint8 *) this->m_spiral.constScanLine(y);
        const quint8 *bLine = (const quint8 *) this->m_ripple.constScanLine(y + this->m_by);

        for (int x = 0; x < src.width(); x++) {
            // Color saturation
            int r = qRed(iLine[x]) > 127? 255: 0;
            int g = qGreen(iLine[x]) > 127? 255: 0;
            int b = qBlue(iLine[x]) > 127? 255: 0;
            int a = qAlpha(iLine[x]);

            int pr = char(rLine[x + this->m_rx] + this->m_phase * 2) >> 7;
            int pg = char(gLine[x] + this->m_phase * 3) >> 7;
            int pb = char(bLine[x + this->m_by] - this->m_phase) >> 7;

            oLine[x] = qRgba(r, g, b, a) & qRgb(pr, pg, pb) & (this->m_mask | 0xff000000);
        }
    }

    this->m_phase -= 8;

    if ((this->m_rx + this->m_rvx) < 0
        || (this->m_rx + this->m_rvx) >= src.width())
        this->m_rvx = -this->m_rvx;

    if ((this->m_ry + this->m_rvy) < 0
        || (this->m_ry + this->m_rvy) >= src.height())
        this->m_rvy = -this->m_rvy;

    if ((this->m_bx + this->m_bvx) < 0
        || (this->m_bx + this->m_bvx) >= src.width())
        this->m_bvx = -this->m_bvx;

    if ((this->m_by + this->m_bvy) < 0
        || (this->m_by + this->m_bvy) >= src.height())
        this->m_bvy = -this->m_bvy;

    this->m_rx += this->m_rvx;
    this->m_ry += this->m_rvy;
    this->m_bx += this->m_bvx;
    this->m_by += this->m_bvy;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
