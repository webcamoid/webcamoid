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

uniform highp sampler2D uTex; // Source frame

// Percentile luma bounds found on the CPU from a downsampled thumbnail,
// normalized to [0, 1] (BT.601 studio-swing scale, i.e. Y = 16/255 .. 235/255).
uniform mediump float uYLow;
uniform mediump float uYHigh;

// Target output luma range (16/255-235/255 for studio swing, or 0-1 for full swing).
uniform mediump float uMinY;
uniform mediump float uMaxY;

varying mediump vec2 vTexCoord;

// BT.601, studio swing (16-235 luma, 16-240 chroma), RGB and YUV both in [0, 1].

mediump float rgb2y(mediump vec3 rgb)
{
    return 16.0 / 255.0 + dot(rgb, vec3(0.257, 0.504, 0.098));
}

mediump vec2 rgb2uv(mediump vec3 rgb)
{
    mediump float u = 128.0 / 255.0 + dot(rgb, vec3(-0.148, -0.291,  0.439));
    mediump float v = 128.0 / 255.0 + dot(rgb, vec3( 0.439, -0.368, -0.071));

    return vec2(u, v);
}

mediump vec3 yuv2rgb(mediump float y, mediump vec2 uv)
{
    mediump float yy = y      - 16.0  / 255.0;
    mediump float u  = uv.x   - 128.0 / 255.0;
    mediump float v  = uv.y   - 128.0 / 255.0;

    mediump float r = 1.164 * yy + 1.596 * v;
    mediump float g = 1.164 * yy - 0.392 * u - 0.813 * v;
    mediump float b = 1.164 * yy + 2.017 * u;

    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main()
{
    highp vec4 pixel = texture2D(uTex, vTexCoord);

    mediump float y  = rgb2y(pixel.rgb);
    mediump vec2  uv = rgb2uv(pixel.rgb);

    mediump float q = uYHigh - uYLow;
    mediump float newY;

    if (q <= 0.0001) {
        newY = y;
    } else {
        mediump float yClamped = clamp(y, uMinY, uMaxY);
        mediump float yDiff    = uMaxY - uMinY;
        newY = (yDiff * (yClamped - uYLow) + q * uMinY) / q;
        newY = clamp(newY, uMinY, uMaxY);
    }

    pixel.rgb = yuv2rgb(newY, uv);
    gl_FragColor = pixel;
}
