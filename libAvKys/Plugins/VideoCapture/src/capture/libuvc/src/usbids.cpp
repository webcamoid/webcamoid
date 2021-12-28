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

#include <QFile>
#include <QtDebug>

#include "usbids.h"

UsbIds::UsbIds(QObject *parent):
    QObject(parent)
{
    QFile file(":/libuvc/share/usbdb/usb.ids");

    file.open(QIODevice::ReadOnly);
    QByteArray line;
    bool end = false;

    while (!end && !file.atEnd()) {
        line = file.readLine().trimmed();

        if (line.startsWith('#') || line.isEmpty())
            continue;

        auto start = line.indexOf(' ');
        auto vendorId = quint16(line.mid(0, start).toUInt(Q_NULLPTR, 16));
        auto vendor = line.mid(start).trimmed();
        QMap<quint16, QString> products;

        while (!file.atEnd()) {
            auto pos = file.pos();
            line = file.readLine();

            if (!line.startsWith('\t')) {
                if (line.trimmed().isEmpty())
                    end = true;

                file.seek(pos);

                break;
            }

            line = line.trimmed();
            auto start = line.indexOf(' ');
            auto productId = quint16(line.mid(0, start).toUInt(Q_NULLPTR, 16));
            auto product = line.mid(start).trimmed();
            products[productId] = product;
        }

        this->m_ids << UsbIdsElement {vendorId, vendor, products};
    }

    file.close();
}

const UsbIdsElement *UsbIds::operator [](quint16 vendorId) const
{
    for (auto &id: this->m_ids)
        if (id.vendorId == vendorId)
            return &id;

    return nullptr;
}

QString UsbIds::description(quint16 vendorId, quint16 productId) const
{
    auto element = this->operator[](vendorId);

    if (!element)
        return {};

    if (element->products.contains(productId))
        return element->products[productId];

    return element->vendor;
}

#include "moc_usbids.cpp"
