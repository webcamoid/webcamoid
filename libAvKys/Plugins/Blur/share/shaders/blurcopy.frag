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

uniform highp sampler2D uTex;
varying mediump vec2 vTexCoord;

/* Trivial passthrough used when uRadius == 0, so that the radius==0 case
 * doesn't pay for a two-pass blur (with its loop-based shaders and
 * intermediate FBO round-trip) just to reproduce the input unchanged.
 */
void main()
{
    gl_FragColor = texture2D(uTex, vTexCoord);
}
