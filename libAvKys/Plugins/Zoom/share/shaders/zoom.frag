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
uniform mediump float uZoom;  // Zoom factor (> 0)

varying mediump vec2 vTexCoord;

void main()
{
    // Map the full-screen quad UVs to source texture coordinates.
    // Center the texture and scale by 1/zoom.
    mediump vec2 srcTex = vec2(
        0.5 + (vTexCoord.x - 0.5) / uZoom,
        0.5 + (vTexCoord.y - 0.5) / uZoom
    );

    // If the source coordinate falls outside [0,1], the pixel is outside
    // the scaled image bounds -> transparent.
    mediump float eps = 0.0001;

    if (srcTex.x < -eps || srcTex.x > 1.0 + eps
        || srcTex.y < -eps || srcTex.y > 1.0 + eps) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);

        return;
    }

    // Clamp to valid range to avoid edge artifacts.
    srcTex = clamp(srcTex, 0.0, 1.0);

    highp vec4 pixel = texture2D(uTex, srcTex);
    gl_FragColor = pixel;
}
