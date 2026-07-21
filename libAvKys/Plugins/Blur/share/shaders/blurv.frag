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

#define MAX_RADIUS 16

uniform highp sampler2D uTex;
uniform mediump float uTexelHeight; // Normalized height of one texel (1.0 / textureHeight).
uniform mediump int uRadius;        // Half-height of the blur kernel in pixels [0..10].
varying mediump vec2 vTexCoord;

/* Vertical pass of a separable box blur. Applied after blurh.frag.
 * See blurh.frag for a full description of the algorithm and the reason for the
 * fixed [-16, 16] loop bounds.
 */
void main()
{
    highp vec4 sum = vec4(0.0);
    mediump float count = 0.0;

    for (int i = -MAX_RADIUS; i <= MAX_RADIUS; i++) {
        if (i < -uRadius || i > uRadius)
            continue;

        highp vec2 offset = vec2(0.0, float(i) * uTexelHeight);
        sum += texture2D(uTex, vTexCoord + offset);
        count += 1.0;
    }

    gl_FragColor = sum / count;
}
