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

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class DiceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int diceSize
               READ diceSize
               WRITE setDiceSize
               RESET resetDiceSize
               NOTIFY diceSizeChanged)

    public:
        explicit DiceElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int diceSize() const;

    private:
        int m_diceSize;

        QMutex m_mutex;
        QImage m_diceMap;
        QSize m_frameSize;

    signals:
        void diceSizeChanged(int diceSize);
        void frameSizeChanged(const QSize &frameSize);

    public slots:
        void setDiceSize(int diceSize);
        void resetDiceSize();
        QbPacket iStream(const QbPacket &packet);

    private slots:
        void updateDiceMap();
};

#endif // DICEELEMENT_H
