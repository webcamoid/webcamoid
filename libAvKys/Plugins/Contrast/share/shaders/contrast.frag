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
uniform mediump float uContrast; // [-1.0, 1.0] mapped from [-255, 255]

varying mediump vec2 vTexCoord;

void main()
{
    mediump vec4 pixel = texture2D(uTex, vTexCoord);

    // Contrast formula:
    // f = 259 * (255 + contrast) / (255 * (259 - contrast))
    // output = f * (input - 128) + 128
    //
    // In shader space [0,1]:
    // f = 259.0 * (1.0 + uContrast) / (255.0 * (259.0/255.0 - uContrast))
    //   = 259.0 * (1.0 + uContrast) / (259.0 - 255.0 * uContrast)
    //
    // But we can simplify: the original formula works on [0,255] values.
    // We'll convert to [0,1], apply, and convert back.

    mediump float f = 259.0 * (1.0 + uContrast) / (259.0 - 255.0 * uContrast);

    pixel.rgb = clamp(f * (pixel.rgb - 0.5) + 0.5, 0.0, 1.0);

    gl_FragColor = pixel;
}
