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

#ifndef RAINDROP_H
#define RAINDROP_H

#include <qglobal.h>

class RainDropPrivate;

class RainDrop
{
    public:
        RainDrop(int width,
                 int height,
                 int nCharacters,
                 int minLength,
                 int maxLength,
                 qreal minSpeed,
                 qreal maxSpeed,
                 bool randomStart);
        RainDrop(const RainDrop &other);
        ~RainDrop();
        RainDrop &operator =(const RainDrop &other);
        RainDrop operator ++(int);
        RainDrop &operator ++();
        int length() const;
        int chr(int index) const;
        bool isVisible() const;
        int x() const;
        int y() const;

    private:
        RainDropPrivate *d;
};

#endif // RAINDROP_H
