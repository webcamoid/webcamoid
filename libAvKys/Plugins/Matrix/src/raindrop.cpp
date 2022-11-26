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

#include <QRandomGenerator>
#include <cstring>

#include "raindrop.h"

class RainDropPrivate
{
    public:
        int m_height {0};
        int m_nCharacters {0};
        int *m_line {nullptr};
        int m_length {0};
        int m_x {0};
        qreal m_y {0.0};
        int m_prevY {0};
        qreal m_speed {0.0};

        inline qreal boundedReal(qreal min, qreal max);
};

RainDrop::RainDrop(int width,
                   int height,
                   int nCharacters,
                   int minLength,
                   int maxLength,
                   qreal minSpeed,
                   qreal maxSpeed,
                   bool randomStart)
{
    this->d = new RainDropPrivate;
    this->d->m_nCharacters = nCharacters;

    this->d->m_height = height;
    this->d->m_x = QRandomGenerator::global()->bounded(width);
    this->d->m_y = randomStart?
                       QRandomGenerator::global()->bounded(height):
                       0;
    this->d->m_prevY = this->d->m_y;
    this->d->m_length =
            QRandomGenerator::global()->bounded(minLength, maxLength);

    if (this->d->m_length < 1)
        this->d->m_length = 1;

    this->d->m_speed = this->d->boundedReal(minSpeed, maxSpeed);

    if (this->d->m_speed < 0.1)
        this->d->m_speed = 0.1;

    if (this->d->m_length > 0) {
        this->d->m_line = new int [this->d->m_length];

        if (nCharacters > 0)
            for (int i = 0; i < this->d->m_length; i++)
                this->d->m_line[i] = QRandomGenerator::global()->bounded(nCharacters);
        else
            memset(this->d->m_line, 0, this->d->m_length * sizeof(int));
    }
}

RainDrop::RainDrop(const RainDrop &other)
{
    this->d = new RainDropPrivate;
    this->d->m_height = other.d->m_height;
    this->d->m_nCharacters = other.d->m_nCharacters;
    this->d->m_length = other.d->m_length;
    this->d->m_x = other.d->m_x;
    this->d->m_y = other.d->m_y;
    this->d->m_prevY = other.d->m_prevY;
    this->d->m_speed = other.d->m_speed;

    if (this->d->m_line)
        delete [] this->d->m_line;

    this->d->m_line = new int [this->d->m_length];
    memcpy(this->d->m_line, other.d->m_line, this->d->m_length * sizeof(int));
}

RainDrop::~RainDrop()
{
    if (this->d->m_line)
        delete [] this->d->m_line;

    delete this->d;
}

RainDrop &RainDrop::operator =(const RainDrop &other)
{
    if (this != &other) {
        this->d->m_height = other.d->m_height;
        this->d->m_nCharacters = other.d->m_nCharacters;
        this->d->m_length = other.d->m_length;
        this->d->m_x = other.d->m_x;
        this->d->m_y = other.d->m_y;
        this->d->m_prevY = other.d->m_prevY;
        this->d->m_speed = other.d->m_speed;

        if (this->d->m_line)
            delete [] this->d->m_line;

        this->d->m_line = new int [this->d->m_length];
        memcpy(this->d->m_line, other.d->m_line, this->d->m_length * sizeof(int));
    }

    return *this;
}

RainDrop RainDrop::operator ++(int)
{
    auto rainDrop = *this;
    this->d->m_y += this->d->m_speed;

    if (this->d->m_prevY != int(this->d->m_y)) {
        memcpy(this->d->m_line + 1, this->d->m_line, (this->d->m_length - 1) * sizeof(int));
        this->d->m_prevY = int(this->d->m_y);
    }

    this->d->m_line[0] =
            this->d->m_nCharacters > 0?
                QRandomGenerator::global()->bounded(this->d->m_nCharacters):
                0;

    return rainDrop;
}

RainDrop &RainDrop::operator ++()
{
    this->d->m_y += this->d->m_speed;

    if (this->d->m_prevY != int(this->d->m_y)) {
        memcpy(this->d->m_line + 1, this->d->m_line, (this->d->m_length - 1) * sizeof(int));
        this->d->m_prevY = int(this->d->m_y);
    }

    this->d->m_line[0] =
            this->d->m_nCharacters > 0?
                QRandomGenerator::global()->bounded(this->d->m_nCharacters):
                0;

    return *this;
}

int RainDrop::length() const
{
    return this->d->m_length;
}

int RainDrop::chr(int index) const
{
    return this->d->m_line[index];
}

bool RainDrop::isVisible() const
{
    return int(this->d->m_y - this->d->m_length + 1) < this->d->m_height;
}

int RainDrop::x() const
{
    return this->d->m_x;
}

int RainDrop::y() const
{
    return int(this->d->m_y - this->d->m_length + 1);
}

qreal RainDropPrivate::boundedReal(qreal min, qreal max)
{
    std::uniform_real_distribution<qreal> distribution(min, max);

    return distribution(*QRandomGenerator::global());
}
