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
uniform mediump float uMin;
uniform mediump float uMax;
varying mediump vec2 vTexCoord;

void main()
{
    highp vec2 uv = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
    highp vec4 color = texture2D(uTexture, uv);
    mediump float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    mediump float eq = (luma - uMin) / (uMax - uMin);
    eq = clamp(eq, 0.0, 1.0);
    gl_FragColor = vec4(vec3(eq), color.a);
}
