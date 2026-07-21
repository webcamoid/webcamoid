/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

uniform highp sampler2D uTexture;
uniform mediump vec2 uTexelSize;
varying mediump vec2 vTexCoord;

void main()
{
    highp vec2 texel = uTexelSize;

    highp float s00 = texture2D(uTexture, vTexCoord + texel * vec2(-1.0,-1.0)).r;
    highp float s10 = texture2D(uTexture, vTexCoord + texel * vec2( 0.0,-1.0)).r;
    highp float s20 = texture2D(uTexture, vTexCoord + texel * vec2( 1.0,-1.0)).r;
    highp float s01 = texture2D(uTexture, vTexCoord + texel * vec2(-1.0, 0.0)).r;
    highp float s21 = texture2D(uTexture, vTexCoord + texel * vec2( 1.0, 0.0)).r;
    highp float s02 = texture2D(uTexture, vTexCoord + texel * vec2(-1.0, 1.0)).r;
    highp float s12 = texture2D(uTexture, vTexCoord + texel * vec2( 0.0, 1.0)).r;
    highp float s22 = texture2D(uTexture, vTexCoord + texel * vec2( 1.0, 1.0)).r;

    highp float gx = -s00 - 2.0 * s10 - s20 + s02 + 2.0 * s12 + s22;
    highp float gy = -s00 - 2.0 * s01 - s02 + s20 + 2.0 * s21 + s22;
    highp float mag = sqrt(gx * gx + gy * gy);
    mag = clamp(mag, 0.0, 1.0);

    mediump float dir;

    if (abs(gy) < 0.0001 && abs(gx) < 0.0001)
        dir = 0.0;
    else if (abs(gx) < 0.0001)
        dir = 3.0;
    else {
        highp float a = degrees(atan(gy, gx));

        if (a >= -22.5 && a < 22.5)
            dir = 0.0;
        else if (a >= 22.5 && a < 67.5)
            dir = 1.0;
        else if (a >= -67.5 && a < -22.5)
            dir = 2.0;
        else
            dir = 3.0;
    }

    gl_FragColor = vec4(mag, dir / 3.0, 0.0, 1.0);
}
