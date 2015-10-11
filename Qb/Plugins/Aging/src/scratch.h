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

#ifndef SCRATCH_H
#define SCRATCH_H

#include <QObject>

class Scratch: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal life
               READ life
               WRITE setLife
               RESET resetLife)
    Q_PROPERTY(qreal dlife
               READ dlife
               WRITE setDLife
               RESET resetDLife)
    Q_PROPERTY(qreal x
               READ x
               WRITE setX
               RESET resetX)
    Q_PROPERTY(qreal dx
               READ dx
               WRITE setDx
               RESET resetDx)
    Q_PROPERTY(int y
               READ y
               WRITE setY
               RESET resetY)

    public:
        explicit Scratch(QObject *parent=NULL);
        Scratch(qreal minLife, qreal maxLife,
                qreal minDLife, qreal maxDLife,
                qreal minX, qreal maxX,
                qreal minDX, qreal maxDX,
                int minY, int maxY);
        Scratch(const Scratch &other);
        Scratch &operator =(const Scratch &other);
        Scratch operator ++(int);

        Q_INVOKABLE qreal life() const;
        Q_INVOKABLE qreal &life();
        Q_INVOKABLE qreal dlife() const;
        Q_INVOKABLE qreal &dlife();
        Q_INVOKABLE qreal x() const;
        Q_INVOKABLE qreal &x();
        Q_INVOKABLE qreal dx() const;
        Q_INVOKABLE qreal &dx();
        Q_INVOKABLE int y() const;
        Q_INVOKABLE int &y();

        Q_INVOKABLE bool isAboutToDie() const;

    private:
        qreal m_life;
        qreal m_dlife;
        qreal m_x;
        qreal m_dx;
        int m_y;

        qreal m_life0;

    public slots:
        void setLife(qreal life);
        void setDLife(qreal dlife);
        void setX(qreal x);
        void setDx(qreal dx);
        void setY(int y);
        void resetLife();
        void resetDLife();
        void resetX();
        void resetDx();
        void resetY();
};

#endif // SCRATCH_H
