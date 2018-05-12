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

#include <qrgb.h>

class RainDropPrivate;
class QImage;
class QPoint;
class QSize;
class QFont;

class RainDrop
{
    public:
        explicit RainDrop(const QSize &textArea,
                          const QString &charTable,
                          const QFont &font,
                          const QSize &fontSize,
                          QRgb cursorColor,
                          QRgb startColor,
                          QRgb endColor,
                          int minLength,
                          int maxLength,
                          qreal minSpeed,
                          qreal maxSpeed,
                          bool randomStart);
        RainDrop(const RainDrop &other);
        ~RainDrop();
        RainDrop &operator =(const RainDrop &other);
        RainDrop operator ++(int);
        bool isVisible() const;
        QImage render(QRgb tailColor, bool showCursor);
        QPoint pos() const;
        QPoint tail() const;

    private:
        RainDropPrivate *d;
};

#endif // RAINDROP_H
