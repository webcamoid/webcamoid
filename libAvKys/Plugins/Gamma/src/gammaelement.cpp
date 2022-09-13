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
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "gammaelement.h"

class GammaElementPrivate
{
    public:
        int m_gamma {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};

        const QVector<quint8> &gammaTable() const;
        QVector<quint8> initGammaTable() const;
};

GammaElement::GammaElement():
    AkElement()
{
    this->d = new GammaElementPrivate;
}

GammaElement::~GammaElement()
{
    delete this->d;
}

int GammaElement::gamma() const
{
    return this->d->m_gamma;
}

QString GammaElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Gamma/share/qml/main.qml");
}

void GammaElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Gamma", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket GammaElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_gamma == 0) {
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
    auto dataCt = this->d->gammaTable();
    auto gamma = qBound(-255, this->d->m_gamma, 255);
    size_t gammaOffset = size_t(gamma + 255) << 8;

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto destLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            auto r = dataCt[gammaOffset | qRed(srcLine[x])];
            auto g = dataCt[gammaOffset | qGreen(srcLine[x])];
            auto b = dataCt[gammaOffset | qBlue(srcLine[x])];
            destLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void GammaElement::setGamma(int gamma)
{
    if (this->d->m_gamma == gamma)
        return;

    this->d->m_gamma = gamma;
    emit this->gammaChanged(gamma);
}

void GammaElement::resetGamma()
{
    this->setGamma(0);
}

const QVector<quint8> &GammaElementPrivate::gammaTable() const
{
    static auto gammaTable = this->initGammaTable();

    return gammaTable;
}

QVector<quint8> GammaElementPrivate::initGammaTable() const
{
    QVector<quint8> gammaTable;

    for (int i = 0; i < 256; i++) {
        auto ig = uint8_t(255. * qPow(i / 255., 255));
        gammaTable.push_back(ig);
    }

    for (int gamma = -254; gamma < 256; gamma++) {
        double k = 255. / (gamma + 255);

        for (int i = 0; i < 256; i++) {
            auto ig = uint8_t(255. * qPow(i / 255., k));
            gammaTable << ig;
        }
    }

    return gammaTable;
}

#include "moc_gammaelement.cpp"
