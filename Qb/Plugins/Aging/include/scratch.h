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
    Q_PROPERTY(int life READ life WRITE setLife RESET resetLife)
    Q_PROPERTY(int x READ x WRITE setX RESET resetX)
    Q_PROPERTY(int dx READ dx WRITE setDx RESET resetDx)
    Q_PROPERTY(int init READ init WRITE setInit RESET resetInit)

    public:
        explicit Scratch(QObject *parent=NULL);
        Scratch(const Scratch &other);
        virtual ~Scratch();
        Scratch &operator =(const Scratch &other);

        Q_INVOKABLE int life() const;
        Q_INVOKABLE int x() const;
        Q_INVOKABLE int dx() const;
        Q_INVOKABLE int init() const;

    private:
        int m_life;
        int m_x;
        int m_dx;
        int m_init;

    public slots:
        void setLife(int life);
        void setX(int x);
        void setDx(int dx);
        void setInit(int init);
        void resetLife();
        void resetX();
        void resetDx();
        void resetInit();
};

#endif // SCRATCH_H
