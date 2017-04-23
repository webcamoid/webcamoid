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

#include <iostream>

extern "C"
{
    #include <libavutil/pixfmt.h>
}

int main()
{
    std::cout << AV_PIX_FMT_RGB0
              << AV_PIX_FMT_BGR0
              << AV_PIX_FMT_NV16
              << AV_PIX_FMT_BAYER_BGGR8
              << AV_PIX_FMT_BAYER_GBRG8
              << AV_PIX_FMT_BAYER_GRBG8
              << AV_PIX_FMT_BAYER_RGGB8
              << AV_PIX_FMT_BAYER_BGGR16LE;

    return 0;
}
