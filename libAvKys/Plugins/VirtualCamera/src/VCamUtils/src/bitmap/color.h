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

#ifndef COLOR_H
#define COLOR_H

#include <cstdint>

namespace AkVCam
{
    namespace Color
    {
        inline uint32_t rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
        {
            return (a << 24) | (r << 16) | (g << 8) | b;
        }

        inline uint32_t red(uint32_t rgba)
        {
            return (rgba >> 16) & 0xff;
        }

        inline uint32_t green(uint32_t rgba)
        {
            return (rgba >> 8) & 0xff;
        }

        inline uint32_t blue(uint32_t rgba)
        {
            return rgba & 0xff;
        }

        inline uint32_t alpha(uint32_t rgba)
        {
            return rgba >> 24;
        }
    }
}

#endif // COLOR_H
