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
uniform mediump float uAngle;       // rotation angle in degrees
uniform mediump int   uKeep;        // 1 = keep original size, 0 = fit to bounds
uniform mediump int   uSrcWidth;    // input width
uniform mediump int   uSrcHeight;   // input height
uniform mediump int   uDstWidth;    // output width
uniform mediump int   uDstHeight;   // output height

varying mediump vec2 vTexCoord;

#define PI 3.14159265359

void main()
{
    mediump float srcW = float(uSrcWidth);
    mediump float srcH = float(uSrcHeight);
    mediump float dstW = float(uDstWidth);
    mediump float dstH = float(uDstHeight);

    // Centre of the output frame in pixel space.
    mediump float dcx = dstW * 0.5;
    mediump float dcy = dstH * 0.5;

    // Centre of the input frame in pixel space.
    mediump float scx = srcW * 0.5;
    mediump float scy = srcH * 0.5;

    // Output fragment position in pixel space (from UV).
    mediump float dx = vTexCoord.x * dstW - dcx;
    mediump float dy = vTexCoord.y * dstH - dcy;

    // Rotation matrix (inverse = transpose for pure rotation).
    mediump float rad = uAngle * PI / 180.0;
    mediump float c = cos(rad);
    mediump float s = sin(rad);

    // Rotate the output pixel position back to input space.
    mediump float sx =  dx * c + dy * s + scx;
    mediump float sy = -dx * s + dy * c + scy;

    // Convert to input UV.
    mediump vec2 srcUV = vec2(sx / srcW, sy / srcH);

    // Sample with clamp-to-edge behaviour for out-of-bounds pixels.
    if (srcUV.x < 0.0 || srcUV.x > 1.0 || srcUV.y < 0.0 || srcUV.y > 1.0)
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    else
        gl_FragColor = texture2D(uTex, srcUV);
}
