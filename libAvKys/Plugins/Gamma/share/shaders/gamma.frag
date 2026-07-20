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
uniform mediump float uGamma; // [-1.0, 1.0] mapped from [-255, 255]

varying mediump vec2 vTexCoord;

void main()
{
    mediump vec4 pixel = texture2D(uTex, vTexCoord);

    // Gamma correction formula:
    // k = 255.0 / (gamma + 255.0)
    // output = 255.0 * pow(input / 255.0, k)
    //
    // In shader space [0,1]:
    // k = 1.0 / (1.0 + uGamma)
    // output = pow(input, k)

    mediump float k = 1.0 / (1.0 + uGamma);
    pixel.rgb = pow(pixel.rgb, vec3(k));

    gl_FragColor = pixel;
}
