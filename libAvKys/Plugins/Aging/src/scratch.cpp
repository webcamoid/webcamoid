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

#include <cstdlib>
#include <random>
#include <QDateTime>
#include <QRandomGenerator>

#include "scratch.h"

class ScratchPrivate
{
    public:
        qreal m_life0 {0.0};
        qreal m_life {0.0};
        qreal m_dlife {0.0};
        qreal m_x {0.0};
        qreal m_dx {0.0};
        int m_y {0};

        inline qreal boundedReal(qreal min, qreal max);
};

Scratch::Scratch()
{
    this->d = new ScratchPrivate;
}

Scratch::Scratch(qreal minLife, qreal maxLife,
                 qreal minDLife, qreal maxDLife,
                 qreal minX, qreal maxX,
                 qreal minDX, qreal maxDX,
                 int minY, int maxY)
{
    this->d = new ScratchPrivate;
    this->d->m_life = this->d->m_life0 = this->d->boundedReal(minLife, maxLife);
    this->d->m_dlife = this->d->boundedReal(minDLife, maxDLife);

    if (!qIsNull(this->d->m_dlife))
        this->d->m_dlife = maxDLife - minDLife;

    this->d->m_x = this->d->boundedReal(minX, maxX);
    this->d->m_dx = this->d->boundedReal(minDX, maxDX);

    if (!qIsNull(this->d->m_dx))
        this->d->m_dx = maxDX - minDX;

    this->d->m_y = QRandomGenerator::global()->bounded(minY, maxY);
}

Scratch::Scratch(const Scratch &other)
{
    this->d = new ScratchPrivate;
    this->d->m_life0 = other.d->m_life0;
    this->d->m_life = other.d->m_life;
    this->d->m_dlife = other.d->m_dlife;
    this->d->m_x = other.d->m_x;
    this->d->m_dx = other.d->m_dx;
    this->d->m_y = other.d->m_y;
}

Scratch::~Scratch()
{
    delete this->d;
}

Scratch &Scratch::operator =(const Scratch &other)
{
    if (this != &other) {
        this->d->m_life0 = other.d->m_life0;
        this->d->m_life = other.d->m_life;
        this->d->m_dlife = other.d->m_dlife;
        this->d->m_x = other.d->m_x;
        this->d->m_dx = other.d->m_dx;
        this->d->m_y = other.d->m_y;
    }

    return *this;
}

Scratch Scratch::operator ++(int)
{
    auto scratch = *this;
    this->d->m_life -= this->d->m_dlife;
    this->d->m_x += this->d->m_dx;

    return scratch;
}

Scratch &Scratch::operator ++()
{
    this->d->m_life -= this->d->m_dlife;
    this->d->m_x += this->d->m_dx;

    return *this;
}

qreal Scratch::life() const
{
    return this->d->m_life;
}

qreal &Scratch::life()
{
    return this->d->m_life;
}

qreal Scratch::dlife() const
{
    return this->d->m_dlife;
}

qreal &Scratch::dlife()
{
    return this->d->m_dlife;
}

qreal Scratch::x() const
{
    return this->d->m_x;
}

qreal &Scratch::x()
{
    return this->d->m_x;
}

qreal Scratch::dx() const
{
    return this->d->m_dx;
}

qreal &Scratch::dx()
{
    return this->d->m_dx;
}

int Scratch::y() const
{
    return this->d->m_y;
}

int &Scratch::y()
{
    return this->d->m_y;
}

bool Scratch::isAboutToDie() const
{
    const qreal threshold = 0.75;

    return this->d->m_life <= this->d->m_dlife * (1.0 + threshold);
}

void Scratch::setLife(qreal life)
{
    this->d->m_life = life;
}

void Scratch::setDLife(qreal dlife)
{
    this->d->m_dlife = dlife;
}

void Scratch::setX(qreal x)
{
    this->d->m_x = x;
}

void Scratch::setDx(qreal dx)
{
    this->d->m_dx = dx;
}

void Scratch::setY(int y)
{
    this->d->m_y = y;
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

qreal ScratchPrivate::boundedReal(qreal min, qreal max)
{
    std::uniform_real_distribution<qreal> distribution(min, max);

    return distribution(*QRandomGenerator::global());
}
