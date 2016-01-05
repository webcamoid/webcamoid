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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef COLORTAPELEMENT_H
#define COLORTAPELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

class ColorTapElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString table
               READ table
               WRITE setTable
               RESET resetTable
               NOTIFY tableChanged)

    public:
        explicit ColorTapElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString table() const;

    private:
        QImage m_table;
        QString m_tableName;

    signals:
        void tableChanged(const QString &table);

    public slots:
        void setTable(const QString &table);
        void resetTable();
        AkPacket iStream(const AkPacket &packet);
};

#endif // COLORTAPELEMENT_H
