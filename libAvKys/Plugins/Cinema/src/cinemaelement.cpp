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
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "cinemaelement.h"

class CinemaElementPrivate
{
    public:
        qreal m_stripSize {0.5};
        QRgb m_stripColor {qRgb(0, 0, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        qint64 *m_aiMultTable {nullptr};
        qint64 *m_aoMultTable {nullptr};
        qint64 *m_alphaDivTable {nullptr};
};

CinemaElement::CinemaElement(): AkElement()
{
    this->d = new CinemaElementPrivate;

    constexpr qint64 maxAi = 255;
    constexpr qint64 maxAi2 = maxAi * maxAi;
    constexpr qint64 alphaMult = 1 << 16;
    this->d->m_aiMultTable = new qint64 [alphaMult];
    this->d->m_aoMultTable = new qint64 [alphaMult];
    this->d->m_alphaDivTable = new qint64 [alphaMult];

    for (qint64 ai = 0; ai < 256; ai++)
        for (qint64 ao = 0; ao < 256; ao++) {
            auto alphaMask = (ai << 8) | ao;
            auto a = maxAi2 - (maxAi - ai) * (maxAi - ao);
            this->d->m_aiMultTable[alphaMask] = a? alphaMult * ai * maxAi / a: 0;
            this->d->m_aoMultTable[alphaMask] = a? alphaMult * ao * (maxAi - ai) / a: 0;
            this->d->m_alphaDivTable[alphaMask] = a / maxAi;
        }
}

CinemaElement::~CinemaElement()
{
    delete [] this->d->m_aiMultTable;
    delete [] this->d->m_aoMultTable;
    delete [] this->d->m_alphaDivTable;

    delete this->d;
}

qreal CinemaElement::stripSize() const
{
    return this->d->m_stripSize;
}

QRgb CinemaElement::stripColor() const
{
    return this->d->m_stripColor;
}

QString CinemaElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Cinema/share/qml/main.qml");
}

void CinemaElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Cinema", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket CinemaElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    int cy = src.caps().height() >> 1;
    auto stripSize = int(cy * this->d->m_stripSize);
    qint64 ri = qRed(this->d->m_stripColor);
    qint64 gi = qGreen(this->d->m_stripColor);
    qint64 bi = qBlue(this->d->m_stripColor);
    qint64 ai = qAlpha(this->d->m_stripColor);
    auto iLineSize = src.lineSize(0);
    auto oLineSize = dst.lineSize(0);
    auto lineSize = qMin(iLineSize, oLineSize);

    for (int y = 0; y < src.caps().height(); y++) {
        int k = cy - qAbs(y - cy);
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        if (k > stripSize)
            memcpy(oLine, iLine, lineSize);
        else
            for (int x = 0; x < src.caps().width(); x++) {
                auto &pixel = iLine[x];

                qint64 ro = qRed(pixel);
                qint64 go = qGreen(pixel);
                qint64 bo = qBlue(pixel);
                qint64 ao = qAlpha(pixel);

                auto alphaMask = (ai << 8) | ao;
                qint64 rt = (ri * this->d->m_aiMultTable[alphaMask] + ro * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 gt = (gi * this->d->m_aiMultTable[alphaMask] + go * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 bt = (bi * this->d->m_aiMultTable[alphaMask] + bo * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 &at = this->d->m_alphaDivTable[alphaMask];

                oLine[x] = qRgba(int(rt), int(gt), int(bt), int(at));
            }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void CinemaElement::setStripSize(qreal stripSize)
{
    if (qFuzzyCompare(this->d->m_stripSize, stripSize))
        return;

    this->d->m_stripSize = stripSize;
    emit this->stripSizeChanged(stripSize);
}

void CinemaElement::setStripColor(QRgb stripColor)
{
    if (this->d->m_stripColor == stripColor)
        return;

    this->d->m_stripColor = stripColor;
    emit this->stripColorChanged(stripColor);
}

void CinemaElement::resetStripSize()
{
    this->setStripSize(0.5);
}

void CinemaElement::resetStripColor()
{
    this->setStripColor(qRgb(0, 0, 0));
}

#include "moc_cinemaelement.cpp"
