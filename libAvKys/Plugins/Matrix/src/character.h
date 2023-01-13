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

#ifndef CHARACTER_H
#define CHARACTER_H

class CharacterPrivate;
class QChar;
class QFont;
class QSize;
class AkVideoPacket;

class Character
{
    public:
        Character();
        Character(const QChar &chr, const QFont &font, const QSize &fontSize);
        Character(const Character &other);
        ~Character();
        Character &operator =(const Character &other);

        QChar chr() const;
        const AkVideoPacket &image() const;
        int weight() const;

    private:
        CharacterPrivate *d;
};

#endif // CHARACTER_H
