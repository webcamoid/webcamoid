/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef FALSECOLORELEMENT_H
#define FALSECOLORELEMENT_H

#include <qrgb.h>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class FalseColorElement: public QbElement
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

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QVariantList table() const;
        Q_INVOKABLE bool soft() const;

    private:
        QList<QRgb> m_table;
        bool m_soft;

        QbElementPtr m_convert;

    signals:
        void tableChanged();
        void softChanged();

    public slots:
        void setTable(const QVariantList &table);
        void setSoft(bool soft);
        void resetTable();
        void resetSoft();
        QbPacket iStream(const QbPacket &packet);
};

#endif // FALSECOLORELEMENT_H
