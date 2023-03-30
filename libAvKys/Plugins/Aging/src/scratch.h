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

#ifndef SCRATCH_H
#define SCRATCH_H

#include <QtCore/qglobal.h>

class ScratchPrivate;

class Scratch
{
    public:
        Scratch();
        Scratch(qreal minLife, qreal maxLife,
                qreal minDLife, qreal maxDLife,
                qreal minX, qreal maxX,
                qreal minDX, qreal maxDX,
                int minY, int maxY);
        Scratch(const Scratch &other);
        ~Scratch();
        Scratch &operator =(const Scratch &other);
        Scratch operator ++(int);
        Scratch &operator ++();

        qreal life() const;
        qreal &life();
        qreal dlife() const;
        qreal &dlife();
        qreal x() const;
        qreal &x();
        qreal dx() const;
        qreal &dx();
        int y() const;
        int &y();

        bool isAboutToDie() const;
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

    private:
        ScratchPrivate *d;
};

#endif // SCRATCH_H
