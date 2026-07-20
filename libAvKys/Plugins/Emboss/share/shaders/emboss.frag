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
uniform mediump float uFactor;
uniform mediump float uBias;
uniform mediump vec2 uTexelSize;
varying mediump vec2 vTexCoord;

highp float luminance(highp vec3 rgb)
{
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main()
{
    highp vec2 texel = uTexelSize;

    highp float s00 = luminance(texture2D(uTexture, vTexCoord + texel * vec2(-1.0, -1.0)).rgb);
    highp float s10 = luminance(texture2D(uTexture, vTexCoord + texel * vec2( 0.0, -1.0)).rgb);
    highp float s01 = luminance(texture2D(uTexture, vTexCoord + texel * vec2(-1.0,  0.0)).rgb);
    highp float s11 = luminance(texture2D(uTexture, vTexCoord).rgb);
    highp float s12 = luminance(texture2D(uTexture, vTexCoord + texel * vec2( 0.0,  1.0)).rgb);
    highp float s22 = luminance(texture2D(uTexture, vTexCoord + texel * vec2( 1.0,  1.0)).rgb);

    highp float gray = 2.0*s00 + s10 + s01 - s11 - s12 - 2.0*s22;
    gray = uFactor * gray + uBias;
    gray = clamp(gray, 0.0, 1.0);

    highp vec4 color = texture2D(uTexture, vTexCoord);
    gl_FragColor = vec4(vec3(gray), color.a);
}
