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

#ifndef FALSECOLORELEMENT_H
#define FALSECOLORELEMENT_H

#include <ak.h>
#include <akutils.h>

class FalseColorElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QVariantList table
               READ table
               WRITE setTable
               RESET resetTable
               NOTIFY tableChanged)
    Q_PROPERTY(bool soft
               READ soft
               WRITE setSoft
               RESET resetSoft
               NOTIFY softChanged)

    public:
        explicit FalseColorElement();

        Q_INVOKABLE QVariantList table() const;
        Q_INVOKABLE bool soft() const;

    private:
        QList<QRgb> m_table;
        bool m_soft;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void tableChanged(const QVariantList &table);
        void softChanged(bool soft);

    public slots:
        void setTable(const QVariantList &table);
        void setSoft(bool soft);
        void resetTable();
        void resetSoft();
        AkPacket iStream(const AkPacket &packet);
};

#endif // FALSECOLORELEMENT_H
