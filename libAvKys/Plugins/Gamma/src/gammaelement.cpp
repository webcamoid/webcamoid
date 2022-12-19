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

#include <QQmlContext>
#include <QtMath>
#include <qrgb.h>
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
        quint8 *m_gammaTable {nullptr};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        inline void initGammaTable();
};

GammaElement::GammaElement():
    AkElement()
{
    this->d = new GammaElementPrivate;
    this->d->initGammaTable();
}

GammaElement::~GammaElement()
{
    if (this->d->m_gammaTable)
        delete [] this->d->m_gammaTable;

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

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto gamma = qBound(-255, this->d->m_gamma, 255);
    size_t gammaOffset = size_t(gamma + 255) << 8;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto &r = this->d->m_gammaTable[gammaOffset | qRed(pixel)];
            auto &g = this->d->m_gammaTable[gammaOffset | qGreen(pixel)];
            auto &b = this->d->m_gammaTable[gammaOffset | qBlue(pixel)];
            dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
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

void GammaElementPrivate::initGammaTable()
{
    static const int grayLevels = 256;
    static const int minGamma = -254;
    static const int maxGamma = 256;
    this->m_gammaTable = new quint8 [grayLevels * (maxGamma - minGamma + 1)];
    size_t j = 0;

    for (int i = 0; i < grayLevels; i++) {
        auto ig = uint8_t(255. * qPow(i / 255., 255));
        this->m_gammaTable[j] = ig;
        j++;
    }

    for (int gamma = minGamma; gamma < maxGamma; gamma++) {
        double k = 255. / (gamma + 255);

        for (int i = 0; i < grayLevels; i++) {
            auto ig = uint8_t(255. * qPow(i / 255., k));
            this->m_gammaTable[j] = ig;
            j++;
        }
    }
}

#include "moc_gammaelement.cpp"
