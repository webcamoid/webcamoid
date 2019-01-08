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

#ifndef USBIDS_H
#define USBIDS_H

#include <QObject>
#include <QVector>
#include <QMap>

struct UsbIdsElement
{
    quint16 vendorId;
    QString vendor;
    QMap<quint16, QString> products;
};

class UsbIds: public QObject
{
    Q_OBJECT

    public:
        UsbIds(QObject *parent=nullptr);

        const UsbIdsElement *operator [](quint16 vendorId) const;
        QString description(quint16 vendorId, quint16 productId) const;

    private:
        QVector<UsbIdsElement> m_ids;

    signals:

    public slots:
};

#endif // USBIDS_H
