/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QStandardPaths>

#include "colortapelement.h"

ColorTapElement::ColorTapElement(): AkElement()
{
    this->m_tableName = ":/ColorTap/share/tables/base.bmp";
    this->m_table = QImage(this->m_tableName).scaled(16, 16);
}

QObject *ColorTapElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ColorTap/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ColorTap", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QString ColorTapElement::table() const
{
    return this->m_tableName;
}

void ColorTapElement::setTable(const QString &table)
{
    if (this->m_tableName == table)
        return;

    QString tableName;
    QImage tableImg;

    if (!table.isEmpty()) {
        tableImg = QImage(table);

        if (tableImg.isNull()) {
            if (this->m_tableName.isNull())
                return;
        } else {
            tableName = table;
            tableImg = tableImg.scaled(16, 16);
        }
    }

    this->m_tableName = tableName;
    this->m_mutex.lock();
    this->m_table = tableImg;
    this->m_mutex.unlock();
    emit this->tableChanged(this->m_tableName);
}

void ColorTapElement::resetTable()
{
    this->setTable(":/ColorTap/share/tables/base.bmp");
}

AkPacket ColorTapElement::iStream(const AkPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_table.isNull()) {
        this->m_mutex.unlock();
        akSend(packet)
    }

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull()) {
        this->m_mutex.unlock();

        return AkPacket();
    }

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    const QRgb *tableBits = reinterpret_cast<const QRgb *>(this->m_table.constBits());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);

            int ro = qRed(tableBits[r]);
            int go = qGreen(tableBits[g]);
            int bo = qBlue(tableBits[b]);

            dstLine[x] = qRgb(ro, go, bo);
        }
    }

    this->m_mutex.unlock();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
