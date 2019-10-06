/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QImage>

#include "character.h"

class CharacterPrivate
{
    public:
        QChar chr;
        QImage image;
        int weight {0};
};

Character::Character()
{
    this->d = new CharacterPrivate;
}

Character::Character(const QChar &chr, const QImage &image, int weight)
{
    this->d = new CharacterPrivate;
    this->d->chr = chr;
    this->d->image = image;
    this->d->weight = weight;
}

Character::Character(const Character &other)
{
    this->d = new CharacterPrivate;
    this->d->chr = other.d->chr;
    this->d->image = other.d->image;
    this->d->weight = other.d->weight;
}

Character::~Character()
{
    delete this->d;
}

Character &Character::operator =(const Character &other)
{
    if (this != &other) {
        this->d->chr = other.d->chr;
        this->d->image = other.d->image;
        this->d->weight = other.d->weight;
    }

    return *this;
}

QChar &Character::chr()
{
    return this->d->chr;
}

QChar Character::chr() const
{
    return this->d->chr;
}

QImage &Character::image()
{
    return this->d->image;
}

QImage Character::image() const
{
    return this->d->image;
}

int &Character::weight()
{
    return this->d->weight;
}

int Character::weight() const
{
    return this->d->weight;
}
