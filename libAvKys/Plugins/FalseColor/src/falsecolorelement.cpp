/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "falsecolorelement.h"

FalseColorElement::FalseColorElement(): AkElement()
{
    this->m_table = {
        qRgb(0, 0, 0),
        qRgb(255, 0, 0),
        qRgb(255, 255, 255),
        qRgb(255, 255, 255)
    };

    this->m_soft = false;
}

QObject *FalseColorElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/FalseColor/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("FalseColor", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QVariantList FalseColorElement::table() const
{
    QVariantList table;

    for (const QRgb &color: this->m_table)
        table << color;

    return table;
}

bool FalseColorElement::soft() const
{
    return this->m_soft;
}

void FalseColorElement::setTable(const QVariantList &table)
{
    QList<QRgb> tableRgb;

    for (const QVariant &color: table)
        tableRgb << color.value<QRgb>();

    if (this->m_table == tableRgb)
        return;

    this->m_table = tableRgb;
    emit this->tableChanged(table);
}

void FalseColorElement::setSoft(bool soft)
{
    if (this->m_soft == soft)
        return;

    this->m_soft = soft;
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

AkPacket FalseColorElement::iStream(const AkPacket &packet)
{
    if (this->m_table.isEmpty())
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_Grayscale8);
    QImage oFrame(src.size(), QImage::Format_ARGB32);

    QRgb table[256];
    QList<QRgb> tableRgb = this->m_table;

    for (int i = 0; i < 256; i++) {
        QRgb color;

        if (this->m_soft) {
            int low = i * (tableRgb.size() - 1) / 255;
            low = qBound(0, low, tableRgb.size() - 2);
            int high = low + 1;

            int rl = qRed(tableRgb[low]);
            int gl = qGreen(tableRgb[low]);
            int bl = qBlue(tableRgb[low]);

            int rh = qRed(tableRgb[high]);
            int gh = qGreen(tableRgb[high]);
            int bh = qBlue(tableRgb[high]);

            int l = 255 * low / (tableRgb.size() - 1);
            int h = 255 * high / (tableRgb.size() - 1);

            qreal k = qreal(i - l) / (h - l);

            int r = int(k * (rh - rl) + rl);
            int g = int(k * (gh - gl) + gl);
            int b = int(k * (bh - bl) + bl);

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            color = qRgb(r, g, b);
        } else {
            int t = tableRgb.size() * i / 255;
            t = qBound(0, t, tableRgb.size() - 1);
            color = tableRgb[t];
        }

        table[i] = color;
    }

    for (int y = 0; y < src.height(); y++) {
        const quint8 *srcLine = src.constScanLine(y);
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++)
            dstLine[x] = table[srcLine[x]];
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
