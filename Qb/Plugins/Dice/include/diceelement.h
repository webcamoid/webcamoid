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

#ifndef DICEELEMENT_H
#define DICEELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

#define DICEDIR_UP    (char) 0
#define DICEDIR_RIGHT (char) 1
#define DICEDIR_DOWN  (char) 2
#define DICEDIR_LEFT  (char) 3

class DiceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int cubeBits
               READ cubeBits
               WRITE setCubeBits
               RESET resetCubeBits
               NOTIFY cubeBitsChanged)

    public:
        explicit DiceElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int cubeBits() const;

    private:
        int m_cubeBits;
        QByteArray m_diceMap;
        QbElementPtr m_convert;

        QByteArray makeDiceMap(const QSize &size, int cubeBits) const;

    signals:
        void cubeBitsChanged();

    public slots:
        void setCubeBits(int cubeBits);
        void resetCubeBits();
        QbPacket iStream(const QbPacket &packet);
};

#endif // DICEELEMENT_H
