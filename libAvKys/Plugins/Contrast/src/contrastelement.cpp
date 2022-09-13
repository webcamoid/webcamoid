/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "contrastelement.h"

class ContrastElementPrivate
{
    public:
        int m_contrast {0};

        const QVector<quint8> &contrastTable() const;
        QVector<quint8> initContrastTable() const;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

ContrastElement::ContrastElement():
    AkElement()
{
    this->d = new ContrastElementPrivate;
}

ContrastElement::~ContrastElement()
{
    delete this->d;
}

int ContrastElement::contrast() const
{
    return this->d->m_contrast;
}

QString ContrastElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Contrast/share/qml/main.qml");
}

void ContrastElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Contrast", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ContrastElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_contrast == 0) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    QImage oFrame(src.size(), src.format());
    auto dataCt = this->d->contrastTable();
    auto contrast = qBound(-255, this->d->m_contrast, 255);
    size_t contrastOffset = size_t(contrast + 255) << 8;

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto destLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            auto r = dataCt[contrastOffset | qRed(srcLine[x])];
            auto g = dataCt[contrastOffset | qGreen(srcLine[x])];
            auto b = dataCt[contrastOffset | qBlue(srcLine[x])];
            destLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void ContrastElement::setContrast(int contrast)
{
    if (this->d->m_contrast == contrast)
        return;

    this->d->m_contrast = contrast;
    emit this->contrastChanged(contrast);
}

void ContrastElement::resetContrast()
{
    this->setContrast(0);
}

const QVector<quint8> &ContrastElementPrivate::contrastTable() const
{
    static auto contrastTable = this->initContrastTable();

    return contrastTable;
}

QVector<quint8> ContrastElementPrivate::initContrastTable() const
{
    QVector<quint8> contrastTable;

    for (int contrast = -255; contrast < 256; contrast++) {
        double f = 259. * (255 + contrast) / (255. * (259 - contrast));

        for (int i = 0; i < 256; i++) {
            int ic = int(f * (i - 128) + 128.);
            contrastTable << quint8(qBound(0, ic, 255));
        }
    }

    return contrastTable;
}

#include "moc_contrastelement.cpp"
