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

uniform highp sampler2D uGradTex;
uniform mediump vec2 uTexelSize;
varying mediump vec2 vTexCoord;

void main()
{
    highp vec2 texel = uTexelSize;
    highp vec4 grad = texture2D(uGradTex, vTexCoord);
    highp float mag = grad.r;
    mediump float dir = grad.g * 3.0;
    highp float left, right;

    if (dir < 0.5) {
        left = texture2D(uGradTex, vTexCoord + texel * vec2(-1.0, 0.0)).r;
        right = texture2D(uGradTex, vTexCoord + texel * vec2( 1.0, 0.0)).r;
    } else if (dir < 1.5) {
        left = texture2D(uGradTex, vTexCoord + texel * vec2(-1.0,-1.0)).r;
        right = texture2D(uGradTex, vTexCoord + texel * vec2( 1.0, 1.0)).r;
    } else if (dir < 2.5) {
        left = texture2D(uGradTex, vTexCoord + texel * vec2( 1.0,-1.0)).r;
        right = texture2D(uGradTex, vTexCoord + texel * vec2(-1.0, 1.0)).r;
    } else {
        left = texture2D(uGradTex, vTexCoord + texel * vec2( 0.0,-1.0)).r;
        right = texture2D(uGradTex, vTexCoord + texel * vec2( 0.0, 1.0)).r;
    }

    highp float outMag = (mag >= left && mag >= right) ? mag : 0.0;
    gl_FragColor = vec4(outMag, 0.0, 0.0, 1.0);
}
