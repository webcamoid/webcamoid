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

#ifndef CLOCK_H
#define CLOCK_H

#include <QObject>

#define THREAD_WAIT_LIMIT 500

class ClockPrivate;

class Clock: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal clock
               READ clock
               WRITE setClock
               RESET resetClock)

    public:
        Clock(QObject *parent=nullptr);
        ~Clock();

        Q_INVOKABLE qreal clock();

    private:
        ClockPrivate *d;

    public slots:
        void setClock(qreal clock);
        void resetClock();
};

#endif // CLOCK_H
