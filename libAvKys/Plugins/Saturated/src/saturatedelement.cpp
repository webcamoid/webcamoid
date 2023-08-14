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
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "saturatedelement.h"

class SaturatedElementPrivate
{
    public:
        int m_factor {127};
        quint8 *m_colorTable {nullptr};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void createTable();
};

SaturatedElement::SaturatedElement(): AkElement()
{
    this->d = new SaturatedElementPrivate;
    this->d->createTable();
}

SaturatedElement::~SaturatedElement()
{
    if (this->d->m_colorTable)
        delete [] this->d->m_colorTable;

    delete this->d;
}

int SaturatedElement::factor() const
{
    return this->d->m_factor;
}

QString SaturatedElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Saturated/share/qml/main.qml");
}

void SaturatedElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Saturated", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket SaturatedElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto table = this->d->m_colorTable
                 + 256 * qBound(0, this->d->m_factor, 255);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto destLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            destLine[x] = qRgba(table[qRed(pixel)],
                                table[qGreen(pixel)],
                                table[qBlue(pixel)],
                                qAlpha(pixel));
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void SaturatedElement::setFactor(int factor)
{
    if (this->d->m_factor == factor)
        return;

    this->d->m_factor = factor;
    emit this->factorChanged(factor);
}

void SaturatedElement::resetFactor()
{
    this->setFactor(127);
}

void SaturatedElementPrivate::createTable()
{
    this->m_colorTable = new quint8 [256 * 256];

    for (int f = 0; f < 256; f++) {
        int mean = 255 - f;
        auto table = this->m_colorTable + 256 * f;

        for (int c = 0; c < 256; c++)
            table[c] = c > mean? 255: 0;
    }
}

#include "moc_saturatedelement.cpp"
