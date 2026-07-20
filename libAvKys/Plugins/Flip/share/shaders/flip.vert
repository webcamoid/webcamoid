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

attribute highp vec3 aPos;
attribute mediump vec2 aTexCoord;
uniform mediump int uHorizontalFlip;
uniform mediump int uVerticalFlip;
varying mediump vec2 vTexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);

    mediump vec2 texCoord = aTexCoord;

    if (uHorizontalFlip > 0)
        texCoord.x = 1.0 - texCoord.x;

    if (uVerticalFlip > 0)
        texCoord.y = 1.0 - texCoord.y;

    vTexCoord = texCoord;
}
