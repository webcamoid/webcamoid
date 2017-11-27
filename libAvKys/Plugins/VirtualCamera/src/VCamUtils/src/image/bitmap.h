/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef BITMAP_H
#define BITMAP_H

#include <iostream>

#include "color.h"
#include "../cstream/cstream.h"

namespace AkVCam
{
    class BitmapPrivate;

    class Bitmap
    {
        public:
            Bitmap(CStreamRead bitmapStream);
            ~Bitmap();

            int width() const;
            int height() const;
            uint32_t *data() const;
            uint32_t *line(int y) const;
            uint32_t pixel(int x, int y) const;

        private:
            BitmapPrivate *d;
    };
}

#endif // BITMAP_H
