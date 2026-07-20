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
uniform mediump vec2 uBlockSize;   // Block size in pixels (width, height)
uniform mediump vec2 uOutputSize;  // Framebuffer width and height
varying mediump vec2 vTexCoord;

void main()
{
    // Calculate pixel coordinates
    highp vec2 pixelPos = vTexCoord * uOutputSize;

    // Calculate which block this pixel belongs to
    highp vec2 blockPos = floor(pixelPos / uBlockSize);

    // Calculate the block center (in pixel coordinates)
    highp vec2 blockCenter = blockPos * uBlockSize + uBlockSize * 0.5;

    // Convert back to UV coordinates
    highp vec2 sampleUV = blockCenter / uOutputSize;

    gl_FragColor = texture2D(uTexture, sampleUV);
}
