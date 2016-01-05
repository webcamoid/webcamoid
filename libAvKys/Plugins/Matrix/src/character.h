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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef CHARACTER_H
#define CHARACTER_H

#include <QImage>
#include <qrgb.h>

class Character
{
    public:
        Character(QChar chr, QImage image, int weight,
                  QRgb foreground=qRgba(0, 0, 0, 0), QRgb background=qRgba(0, 0, 0, 0)):
            chr(chr), image(image), weight(weight),
            foreground(foreground), background(background)
        {
        }

        QChar chr;
        QImage image;
        int weight;
        QRgb foreground;
        QRgb background;
};

#endif // CHARACTER_H
