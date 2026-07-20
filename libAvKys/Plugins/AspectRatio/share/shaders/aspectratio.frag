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
uniform mediump vec4  uSrcCrop;  // Source crop in UVs (x, y, w, h)

varying mediump vec2 vTexCoord;

void main()
{
    // Map the full output quad to the source crop region
    mediump vec2 srcUV = vTexCoord * uSrcCrop.zw + uSrcCrop.xy;
    highp vec4 pixel = texture2D(uTex, srcUV);
    gl_FragColor = pixel;
}
