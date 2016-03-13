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

#ifndef AGINGELEMENT_H
#define AGINGELEMENT_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

#include "scratch.h"

class AgingElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int nScratches
               READ nScratches
               WRITE setNScratches
               RESET resetNScratches
               NOTIFY nScratchesChanged)
    Q_PROPERTY(bool addDust
               READ addDust
               WRITE setAddDust
               RESET resetAddDust
               NOTIFY addDustChanged)

    public:
        explicit AgingElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int nScratches() const;
        Q_INVOKABLE int addDust() const;

    private:
        QVector<Scratch> m_scratches;
        bool m_addDust;

        QMutex m_mutex;

        QImage colorAging(const QImage &src);
        void scratching(QImage &dest);
        void pits(QImage &dest);
        void dusts(QImage &dest);

    signals:
        void nScratchesChanged(int nScratches);
        void addDustChanged(int addDust);

    public slots:
        void setNScratches(int nScratches);
        void setAddDust(int addDust);
        void resetNScratches();
        void resetAddDust();

        AkPacket iStream(const AkPacket &packet);
};

#endif // AGINGELEMENT_H
