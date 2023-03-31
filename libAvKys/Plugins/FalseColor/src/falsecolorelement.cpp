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
#include <QVariant>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "falsecolorelement.h"

class FalseColorElementPrivate
{
    public:
        QMutex m_mutex;
        QList<QRgb> m_table {
            qRgb(0, 0, 0),
            qRgb(255, 0, 0),
            qRgb(255, 255, 255),
            qRgb(255, 255, 255)
        };
        QRgb m_colorTable[256];
        bool m_soft {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_graya8pack, 0, 0, {}}};

        inline void updateColorTable();
};

FalseColorElement::FalseColorElement(): AkElement()
{
    this->d = new FalseColorElementPrivate;
    this->d->updateColorTable();
}

FalseColorElement::~FalseColorElement()
{
    delete this->d;
}

QVariantList FalseColorElement::table() const
{
    QVariantList table;

    for (auto &color: this->d->m_table)
        table << color;

    return table;
}

bool FalseColorElement::soft() const
{
    return this->d->m_soft;
}

QRgb FalseColorElement::colorAt(int index)
{
    auto color = this->d->m_table.at(index);

    return color;
}

QString FalseColorElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FalseColor/share/qml/main.qml");
}

void FalseColorElement::controlInterfaceConfigure(QQmlContext *context,
                                                  const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FalseColor", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket FalseColorElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto ocaps = src.caps();
    ocaps.setFormat(AkVideoCaps::Format_argbpack);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    this->d->m_mutex.lock();

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &ipixel = srcLine[x];
            auto opixel = this->d->m_colorTable[ipixel >> 8];
            dstLine[x] = qRgba(qRed(opixel),
                               qGreen(opixel),
                               qBlue(opixel),
                               ipixel & 0xff);
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void FalseColorElement::addColor(QRgb color)
{
    QVariantList table;

    for (auto &color: this->d->m_table)
        table << color;

    table << color;
    this->setTable(table);
}

void FalseColorElement::setColor(int index, QRgb color)
{
    QVariantList table;
    int i = 0;

    for (auto &color_: this->d->m_table) {
        if (i == index)
            table << color;
        else
            table << color_;

        i++;
    }

    this->setTable(table);
}

void FalseColorElement::removeColor(int index)
{
    QVariantList table;
    int i = 0;

    for (auto &color: this->d->m_table) {
        if (i != index)
            table << color;

        i++;
    }

    this->setTable(table);
}

void FalseColorElement::clearTable()
{
    this->setTable({});
}

void FalseColorElement::setTable(const QVariantList &table)
{
    QList<QRgb> tableRgb;

    for (auto &color: table)
        tableRgb << color.value<QRgb>();

    if (this->d->m_table == tableRgb)
        return;

    this->d->m_table = tableRgb;
    this->d->updateColorTable();
    emit this->tableChanged(table);
}

void FalseColorElement::setSoft(bool soft)
{
    if (this->d->m_soft == soft)
        return;

    this->d->m_soft = soft;
    this->d->updateColorTable();
    emit this->softChanged(soft);
}

void FalseColorElement::resetTable()
{
    static const QVariantList table = {
        qRgb(0, 0, 0),
        qRgb(255, 0, 0),
        qRgb(255, 255, 255),
        qRgb(255, 255, 255)
    };

    this->setTable(table);
}

void FalseColorElement::resetSoft()
{
    this->setSoft(false);
}

void FalseColorElementPrivate::updateColorTable()
{
    this->m_mutex.lock();
    auto tableSize = this->m_table.size();

    for (int i = 0; i < 256; i++) {
        QRgb color;

        if (this->m_soft) {
            int low = i * (tableSize - 1) / 255;
            low = qBound(0, low, tableSize - 2);
            int high = low + 1;

            auto &colorLow = this->m_table[low];
            int rl = qRed(colorLow);
            int gl = qGreen(colorLow);
            int bl = qBlue(colorLow);

            auto &colorHigh = this->m_table[high];
            int rh = qRed(colorHigh);
            int gh = qGreen(colorHigh);
            int bh = qBlue(colorHigh);

            int l = 255 * low / (tableSize - 1);
            int h = 255 * high / (tableSize - 1);

            qreal k = qreal(i - l) / (h - l);

            int r = int(k * (rh - rl) + rl);
            int g = int(k * (gh - gl) + gl);
            int b = int(k * (bh - bl) + bl);

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            color = qRgb(r, g, b);
        } else {
            int t = tableSize * i / 255;
            t = qBound(0, t, tableSize - 1);
            auto &tcolor = this->m_table[t];
            int r = qRed(tcolor);
            int g = qGreen(tcolor);
            int b = qBlue(tcolor);
            color = qRgb(r, g, b);
        }

        this->m_colorTable[i] = color;
    }

    this->m_mutex.unlock();
}

#include "moc_falsecolorelement.cpp"
