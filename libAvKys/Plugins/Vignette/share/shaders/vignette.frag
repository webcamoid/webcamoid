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

// Ellipse semi-axes in normalised UV space (centre at 0.5, 0.5).
uniform mediump float uA;
uniform mediump float uB;

// Pre-computed normalisation factor: sqrt((0.5/a)^2 + (0.5/b)^2).
uniform mediump float uMaxRadius;

// Vignette colour (RGBA, linear [0,1]).
uniform mediump vec4 uColor;

// Softness in [-1, 1].
uniform mediump float uSoftness;

varying mediump vec2 vTexCoord;

void main()
{
    highp vec4 src = texture2D(uTex, vTexCoord);

    // UV offset from centre.
    highp vec2 uv = vTexCoord - vec2(0.5, 0.5);

    // Normalised ellipse coordinates.
    highp float dxa = uv.x / uA;
    highp float dyb = uv.y / uB;

    // Is the fragment inside the ellipse?
    if (dxa * dxa + dyb * dyb < 1.0) {
        // Inside: show the original pixel unchanged.
        gl_FragColor = src;
    } else {
        // Outside: blend the vignette colour over the source.
        // k is the ratio of the current radius to the corner radius.
        highp float k = sqrt(dxa * dxa + dyb * dyb) / uMaxRadius;
        mediump float opacity = clamp(k * uColor.a - uSoftness, 0.0, 1.0);

        // Alpha-blend: vignetteColour over src.
        gl_FragColor = mix(src, vec4(uColor.rgb, 1.0), opacity);
    }
}
