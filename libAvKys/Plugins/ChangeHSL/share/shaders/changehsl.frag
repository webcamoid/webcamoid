/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
 *
 * Web-Site: http://webcamoid.github.io/
 */

uniform highp sampler2D uTex;
uniform mediump vec4 uKernelRow0;
uniform mediump vec4 uKernelRow1;
uniform mediump vec4 uKernelRow2;

varying mediump vec2 vTexCoord;

mediump float hueToRgb(mediump float p, mediump float q, mediump float t)
{
    if (t < 0.0)
        t += 1.0;

    if (t > 1.0)
        t -= 1.0;

    if (t < 1.0/6.0)
        return p + (q - p) * 6.0 * t;

    if (t < 1.0/2.0)
        return q;

    if (t < 2.0/3.0)
        return p + (q - p) * (2.0/3.0 - t) * 6.0;

    return p;
}

mediump vec3 rgbToHsl(mediump vec3 rgb)
{
    mediump float maxC = max(rgb.r, max(rgb.g, rgb.b));
    mediump float minC = min(rgb.r, min(rgb.g, rgb.b));
    mediump float l = (maxC + minC) * 0.5;

    if (maxC == minC)
        return vec3(0.0, 0.0, l);

    mediump float d = maxC - minC;
    mediump float s = l > 0.5 ? d / (2.0 - maxC - minC) : d / (maxC + minC);

    mediump float h;

    if (maxC == rgb.r)
        h = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.0 : 0.0);
    else if (maxC == rgb.g)
        h = (rgb.b - rgb.r) / d + 2.0;
    else
        h = (rgb.r - rgb.g) / d + 4.0;

    h /= 6.0;

    return vec3(h, s, l);
}

mediump vec3 hslToRgb(mediump vec3 hsl)
{
    mediump float h = hsl.x;
    mediump float s = hsl.y;
    mediump float l = hsl.z;

    if (s == 0.0)
        return vec3(l, l, l);

    mediump float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    mediump float p = 2.0 * l - q;

    mediump vec3 rgb;
    rgb.r = hueToRgb(p, q, h + 1.0/3.0);
    rgb.g = hueToRgb(p, q, h);
    rgb.b = hueToRgb(p, q, h - 1.0/3.0);

    return rgb;
}

// True modulo for floats (always positive result)
mediump float modFloat(mediump float x, mediump float y)
{
    mediump float result = x - y * floor(x / y);

    if (result < 0.0)
        result += y;

    return result;
}

void main()
{
    mediump vec4 pixel = texture2D(uTex, vTexCoord);
    mediump vec3 rgb = pixel.rgb;

    // RGB -> HSL
    mediump vec3 hsl = rgbToHsl(rgb);

    // Scale to CPU ranges
    mediump float h = hsl.x * 360.0;
    mediump float s = hsl.y * 255.0;
    mediump float l = hsl.z * 255.0;

    // Apply kernel
    mediump float ht = h * uKernelRow0.x + s * uKernelRow0.y + l * uKernelRow0.z + uKernelRow0.w;
    mediump float st = h * uKernelRow1.x + s * uKernelRow1.y + l * uKernelRow1.z + uKernelRow1.w;
    mediump float lt = h * uKernelRow2.x + s * uKernelRow2.y + l * uKernelRow2.z + uKernelRow2.w;

    ht = modFloat(ht, 360.0);
    st = clamp(st, 0.0, 255.0);
    lt = clamp(lt, 0.0, 255.0);

    // Scale back
    hsl.x = ht / 360.0;
    hsl.y = st / 255.0;
    hsl.z = lt / 255.0;

    // HSL -> RGB
    mediump vec3 outRgb = hslToRgb(hsl);

    gl_FragColor = vec4(outRgb, pixel.a);
}
