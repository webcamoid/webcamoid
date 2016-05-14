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

#include "scratch.h"

Scratch::Scratch(QObject *parent):
    QObject(parent),
    m_life(0.0),
    m_dlife(0.0),
    m_x(0.0),
    m_dx(0.0),
    m_y(0),
    m_life0(0.0)
{
}

Scratch::Scratch(qreal minLife, qreal maxLife,
                 qreal minDLife, qreal maxDLife,
                 qreal minX, qreal maxX,
                 qreal minDX, qreal maxDX,
                 int minY, int maxY)
{
    this->m_life = this->m_life0 = qrand() * (maxLife - minLife) / RAND_MAX + minLife;
    this->m_dlife = qrand() * (maxDLife - minDLife) / RAND_MAX + minDLife;

    if (!qIsNull(this->m_dlife))
        this->m_dlife = maxDLife - minDLife;

    this->m_x = qrand() * (maxX - minX) / RAND_MAX + minX;
    this->m_dx = qrand() * (maxDX - minDX) / RAND_MAX + minDX;

    if (!qIsNull(this->m_dx))
        this->m_dx = maxDX - minDX;

//    this->m_dx *= (qrand() & 0x1? 1.0: -1.0);

    this->m_y = int(qrand() * (maxY - minY) / RAND_MAX) + minY;
}

Scratch::Scratch(const Scratch &other):
    QObject(other.parent()),
    m_life(other.m_life),
    m_dlife(other.m_dlife),
    m_x(other.m_x),
    m_dx(other.m_dx),
    m_y(other.m_y),
    m_life0(other.m_life0)
{
}

Scratch &Scratch::operator =(const Scratch &other)
{
    if (this != &other) {
        this->m_life = other.m_life;
        this->m_dlife = other.m_dlife;
        this->m_x = other.m_x;
        this->m_dx = other.m_dx;
        this->m_y = other.m_y;
        this->m_life0 = other.m_life0;
    }

    return *this;
}

Scratch Scratch::operator ++(int)
{
    this->m_life -= this->m_dlife;
    this->m_x += this->m_dx;

    return *this;
}

qreal Scratch::life() const
{
    return this->m_life;
}

qreal &Scratch::life()
{
    return this->m_life;
}

qreal Scratch::dlife() const
{
    return this->m_dlife;
}

qreal &Scratch::dlife()
{
    return this->m_dlife;
}

qreal Scratch::x() const
{
    return this->m_x;
}

qreal &Scratch::x()
{
    return this->m_x;
}

qreal Scratch::dx() const
{
    return this->m_dx;
}

qreal &Scratch::dx()
{
    return this->m_dx;
}

int Scratch::y() const
{
    return this->m_y;
}

int &Scratch::y()
{
    return this->m_y;
}

bool Scratch::isAboutToDie() const
{
    qreal threshold = 0.75;

    if (this->m_life <= this->m_dlife * (1.0 + threshold))
        return true;

    return false;
}

void Scratch::setLife(qreal life)
{
    this->m_life = life;
}

void Scratch::setDLife(qreal dlife)
{
    this->m_dlife = dlife;
}

void Scratch::setX(qreal x)
{
    this->m_x = x;
}

void Scratch::setDx(qreal dx)
{
    this->m_dx = dx;
}

void Scratch::setY(int y)
{
    this->m_y = y;
}

void Scratch::resetLife()
{
    this->setLife(0.0);
}

void Scratch::resetDLife()
{
    this->setDLife(0.0);
}

void Scratch::resetX()
{
    this->setX(0.0);
}

void Scratch::resetDx()
{
    this->setDx(0.0);
}

void Scratch::resetY()
{
    this->setY(0);
}
