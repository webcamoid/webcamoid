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

#ifndef PARAMS_H
#define PARAMS_H

#include "pixel.h"

struct DenoiseParams
{
    int xp;
    int yp;
    int kw;
    int kh;
    PixelU8 iPixel;
    QRgb *oPixel;
    int alpha;
};

struct DenoiseStaticParams
{
    const PixelU8 *planes;
    const PixelU32 *integral;
    const PixelU64 *integral2;

    int width;
    int oWidth;

    const int *weights;

    int mu;
    qreal sigma;
};

#endif // PARAMS_H
