/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "blip.h"

Blip::Blip(QObject *parent): QObject(parent)
{
    this->resetMode();
    this->resetY();
    this->resetTimer();
    this->resetSpeed();
}

Blip::Blip(const Blip &other):
    QObject(other.parent()),
    m_mode(other.m_mode),
    m_y(other.m_y),
    m_timer(other.m_timer),
    m_speed(other.m_speed)
{
}

Blip &Blip::operator =(const Blip &other)
{
    if (this != &other)
    {
        this->m_mode = other.m_mode;
        this->m_y = other.m_y;
        this->m_timer = other.m_timer;
        this->m_speed = other.m_speed;
    }

    return *this;
}

int Blip::mode() const
{
    return this->m_mode;
}

int Blip::y() const
{
    return this->m_y;
}

int Blip::timer() const
{
    return this->m_timer;
}

int Blip::speed() const
{
    return this->m_speed;
}

void Blip::setMode(int mode)
{
    this->m_mode = mode;
}

void Blip::setY(int y)
{
    this->m_y = y;
}

void Blip::setTimer(int timer)
{
    this->m_timer = timer;
}

void Blip::setSpeed(int speed)
{
    this->m_speed = speed;
}

void Blip::resetMode()
{
    this->setMode(0);
}

void Blip::resetY()
{
    this->setY(0);
}

void Blip::resetTimer()
{
    this->setTimer(0);
}

void Blip::resetSpeed()
{
    this->setSpeed(0);
}
