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
uniform mediump float uTexelWidth; // Normalized width of one texel (1.0 / textureWidth).
uniform mediump int uRadius;       // Half-width of the blur kernel in pixels [0..10].
varying mediump vec2 vTexCoord;

/* Horizontal pass of a separable box blur.
 *
 * Samples uRadius pixels to the left and right of the current pixel and
 * averages them with equal weights. The vertical pass (blurv.frag) is applied
 * afterward to complete the 2D blur.
 *
 * The loop runs over a fixed range [-16, 16] to satisfy GLSL ES loop
 * constraints (loop bounds must be compile-time constants). Samples outside
 * [-uRadius, uRadius] are skipped, so the effective kernel width is
 * (2 * uRadius + 1) taps.
 */
void main()
{
    highp vec4 sum = vec4(0.0);
    mediump float count = 0.0;

    for (int i = -MAX_RADIUS; i <= MAX_RADIUS; i++) {
        if (i < -uRadius || i > uRadius)
            continue;

        highp vec2 offset = vec2(float(i) * uTexelWidth, 0.0);
        sum += texture2D(uTex, vTexCoord + offset);
        count += 1.0;
    }

    gl_FragColor = sum / count;
}
