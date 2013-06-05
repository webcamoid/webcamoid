/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "scratch.h"

Scratch::Scratch(QObject *parent): QObject(parent)
{
    this->resetLife();
    this->resetX();
    this->resetDx();
    this->resetInit();
}

Scratch::Scratch(const Scratch &other):
    QObject(other.parent()),
    m_life(other.m_life),
    m_x(other.m_x),
    m_dx(other.m_dx),
    m_init(other.m_init)
{
}

Scratch::~Scratch()
{
}

Scratch &Scratch::operator =(const Scratch &other)
{
    if (this != &other)
    {
        this->m_life = other.m_life;
        this->m_x = other.m_x;
        this->m_dx = other.m_dx;
        this->m_init = other.m_init;
    }

    return *this;
}

int Scratch::life() const
{
    return this->m_life;
}

int Scratch::x() const
{
    return this->m_x;
}

int Scratch::dx() const
{
    return this->m_dx;
}

int Scratch::init() const
{
    return this->m_init;
}

void Scratch::setLife(int life)
{
    this->m_life = life;
}

void Scratch::setX(int x)
{
    this->m_x = x;
}

void Scratch::setDx(int dx)
{
    this->m_dx = dx;
}

void Scratch::setInit(int init)
{
    this->m_init = init;
}

void Scratch::resetLife()
{
    this->setLife(0);
}

void Scratch::resetX()
{
    this->setX(0);
}

void Scratch::resetDx()
{
    this->setDx(0);
}

void Scratch::resetInit()
{
    this->setInit(0);
}
