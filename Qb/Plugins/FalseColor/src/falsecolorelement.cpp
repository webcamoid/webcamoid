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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "falsecolorelement.h"

FalseColorElement::FalseColorElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetSoft();
    this->resetTable();
}

QObject *FalseColorElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/FalseColor/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("FalseColor", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QVariantList FalseColorElement::table() const
{
    QVariantList table;

    foreach (QRgb color, this->m_table)
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

    foreach (QVariant color, table)
        tableRgb << color.value<QRgb>();

    if (tableRgb != this->m_table) {
        this->m_table = tableRgb;
        emit this->tableChanged();
    }
}

void FalseColorElement::setSoft(bool soft)
{
    if (soft != this->m_soft) {
        this->m_soft = soft;
        emit this->softChanged();
    }
}

void FalseColorElement::resetTable()
{
    QVariantList table;

    table << qRgb(0, 0, 0)
          << qRgb(255, 0, 0)
          << qRgb(255, 255, 255)
          << qRgb(255, 255, 255);

    this->setTable(table);
}

void FalseColorElement::resetSoft()
{
    this->setSoft(false);
}

QbPacket FalseColorElement::iStream(const QbPacket &packet)
{
    if (this->m_table.isEmpty())
        qbSend(packet)

    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    quint8 grayBuffer[videoArea];

    for (int i = 0; i < videoArea; i++)
        grayBuffer[i] = qGray(srcBits[i]);

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

            qreal k = (qreal) (i - l) / (h - l);

            int r = k * (rh - rl) + rl;
            int g = k * (gh - gl) + gl;
            int b = k * (bh - bl) + bl;

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            color = qRgb(r, g, b);
        }
        else {
            int t = tableRgb.size() * i / 255;
            t = qBound(0, t, tableRgb.size() - 1);
            color = tableRgb[t];
        }

        table[i] = color;
    }

    for (int i = 0; i < videoArea; i++)
        destBits[i] = table[grayBuffer[i]];

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
