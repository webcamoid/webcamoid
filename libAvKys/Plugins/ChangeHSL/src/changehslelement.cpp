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

#include <QVariant>
#include <QImage>
#include <QQmlContext>
#include <akvideopacket.h>

#include "changehslelement.h"

class ChangeHSLElementPrivate
{
    public:
        QVector<qreal> m_kernel;
};

ChangeHSLElement::ChangeHSLElement(): AkElement()
{
    this->d = new ChangeHSLElementPrivate;

    this->d->m_kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };
}

ChangeHSLElement::~ChangeHSLElement()
{
    delete this->d;
}

QVariantList ChangeHSLElement::kernel() const
{
    QVariantList kernel;

    for (const qreal &e: this->d->m_kernel)
        kernel << e;

    return kernel;
}

QString ChangeHSLElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ChangeHSL/share/qml/main.qml");
}

void ChangeHSLElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ChangeHSL", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void ChangeHSLElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->d->m_kernel == k)
        return;

    this->d->m_kernel = k;
    emit this->kernelChanged(kernel);
}

void ChangeHSLElement::resetKernel()
{
    static const QVariantList kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };

    this->setKernel(kernel);
}

AkPacket ChangeHSLElement::iStream(const AkPacket &packet)
{
    if (this->d->m_kernel.size() < 12)
        akSend(packet)

    AkVideoPacket videoPacket(packet);
    auto src = videoPacket.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    QVector<qreal> kernel = this->d->m_kernel;

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int h;
            int s;
            int l;
            int a;

            QColor(srcLine[x]).getHsl(&h, &s, &l, &a);

            int ht = int(h * kernel[0] + s * kernel[1] + l * kernel[2]  + kernel[3]);
            int st = int(h * kernel[4] + s * kernel[5] + l * kernel[6]  + kernel[7]);
            int lt = int(h * kernel[8] + s * kernel[9] + l * kernel[10] + kernel[11]);

            ht = qBound(0, ht, 359);
            st = qBound(0, st, 255);
            lt = qBound(0, lt, 255);

            QColor color;
            color.setHsl(ht, st, lt, a);

            dstLine[x] = color.rgba();
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, videoPacket).toPacket();
    akSend(oPacket)
}

#include "moc_changehslelement.cpp"
