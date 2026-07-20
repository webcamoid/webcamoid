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
uniform mediump float uHue;        // degrees [-180, 180]
uniform mediump float uSaturation; // offset [-255, 255], scaled to [-1, 1]
uniform mediump float uLuminance;  // offset [-255, 255], scaled to [-1, 1]

varying mediump vec2 vTexCoord;

// Convert RGB [0,1] to HSL; h in [0,360), s and l in [0,1]
mediump vec3 rgbToHsl(mediump vec3 rgb)
{
    mediump float maxC = max(rgb.r, max(rgb.g, rgb.b));
    mediump float minC = min(rgb.r, min(rgb.g, rgb.b));
    mediump float l = (maxC + minC) * 0.5;

    if (maxC == minC)
        return vec3(0.0, 0.0, l); // achromatic

    mediump float d = maxC - minC;
    mediump float s = l > 0.5 ? d / (2.0 - maxC - minC) : d / (maxC + minC);

    mediump float h;

    if (maxC == rgb.r)
        h = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.0 : 0.0);
    else if (maxC == rgb.g)
        h = (rgb.b - rgb.r) / d + 2.0;
    else
        h = (rgb.r - rgb.g) / d + 4.0;

    h *= 60.0;

    return vec3(h, s, l);
}

// Helper: hue to RGB channel
mediump float hue2rgb(mediump float p, mediump float q, mediump float t)
{
    if (t < 0.0)
        t += 1.0;

    if (t > 1.0)
        t -= 1.0;

    if (t < 1.0 / 6.0)
        return p + (q - p) * 6.0 * t;

    if (t < 1.0 / 2.0)
        return q;

    if (t < 2.0 / 3.0)
        return p + (q - p) * (2.0 / 3.0 - t) * 6.0;

    return p;
}

// Convert HSL [h in degrees, s and l in [0,1]] to RGB [0,1]
mediump vec3 hslToRgb(mediump vec3 hsl)
{
    mediump float h = hsl.x;
    mediump float s = hsl.y;
    mediump float l = hsl.z;

    if (s == 0.0)
        return vec3(l, l, l); // achromatic

    mediump float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    mediump float p = 2.0 * l - q;
    mediump float hd = h / 360.0;

    mediump float r = hue2rgb(p, q, hd + 1.0 / 3.0);
    mediump float g = hue2rgb(p, q, hd);
    mediump float b = hue2rgb(p, q, hd - 1.0 / 3.0);

    return vec3(r, g, b);
}

void main()
{
    mediump vec4 pixel = texture2D(uTex, vTexCoord);

    // RGB [0,1] -> HSL
    mediump vec3 hsl = rgbToHsl(pixel.rgb);

    // Apply adjustments
    hsl.x = mod(hsl.x + uHue, 360.0);

    if (hsl.x < 0.0)
        hsl.x += 360.0;

    hsl.y = clamp(hsl.y + uSaturation, 0.0, 1.0);
    hsl.z = clamp(hsl.z + uLuminance, 0.0, 1.0);

    // HSL -> RGB
    pixel.rgb = hslToRgb(hsl);

    gl_FragColor = pixel;
}
