/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "shagadelicelement.h"

ShagadelicElement::ShagadelicElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetMask();
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

QByteArray ShagadelicElement::makeRipple(const QSize &size) const
{
    QByteArray ripple;

    ripple.resize(4 * size.width() * size.height());
    int i = 0;

    for (int y = 0; y < size.height() * 2; y++) {
        double yy = (double) y / size.width() - 1.0;
        yy *= yy;

        for (int x = 0; x < size.width() * 2; x++) {
            double xx = (double) x / size.width() - 1.0;
            ripple.data()[i++] = ((unsigned int) (sqrt(xx * xx + yy) * 3000)) & 255;
        }
    }

    return ripple;
}

QByteArray ShagadelicElement::makeSpiral(const QSize &size) const
{
    QByteArray spiral;

    spiral.resize(size.width() * size.height());
    int i = 0;

    for (int y = 0; y < size.height(); y++) {
        double yy = (double) (y - size.height() / 2) / size.width();

        for (int x = 0; x < size.width(); x++) {
            double xx = (double) x / size.width() - 0.5;

            spiral.data()[i++] = ((unsigned int) ((atan2(xx, yy) / M_PI * 256 * 9)
                                                  + (sqrt(xx * xx + yy * yy) * 1800)))
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
    if (mask != this->m_mask) {
        this->m_mask = mask;
        emit this->maskChanged();
    }
}

void ShagadelicElement::resetMask()
{
    this->setMask(0xffffff);
}

QbPacket ShagadelicElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    if (src.size() != this->m_curSize) {
        this->init(src.size());
        this->m_curSize = src.size();
    }

    char *pr = &this->m_ripple.data()[this->m_ry * src.width() * 2 + this->m_rx];
    char *pg = this->m_spiral.data();
    char *pb = &this->m_ripple.data()[this->m_by * src.width() * 2 + this->m_bx];

    for (int y = 0; y < src.height(); y++) {
        for (int x = 0; x < src.width(); x++) {
            quint32 v = *srcBits++ | 0x1010100;
            v = (v - 0x707060) & 0x1010100;
            v -= v >> 8;

            uchar r = (char) (*pr + this->m_phase * 2) >> 7;
            uchar g = (char) (*pg + this->m_phase * 3) >> 7;
            uchar b = (char) (*pb - this->m_phase) >> 7;

            *destBits++ = v & ((r << 16) | (g << 8) | b) & this->m_mask;

            pr++;
            pg++;
            pb++;
        }

        pr += src.width();
        pb += src.width();
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

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
