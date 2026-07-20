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
uniform mediump vec4  uSrcCrop;    // Source crop in UVs (x, y, w, h)
uniform mediump vec4  uDstCrop;    // Destination crop in UVs (x, y, w, h)
uniform mediump vec4  uFillColor;  // Fill color (RGBA)
uniform mediump float uEditMode;   // 0 = normal, 1 = edit mode
uniform mediump vec4  uEditColor;  // Border color for edit mode
uniform mediump int   uWidth;
uniform mediump int   uHeight;

varying mediump vec2 vTexCoord;

void main()
{
    mediump float fw = float(uWidth);
    mediump float fh = float(uHeight);

    // Pixel position in output
    mediump float px = vTexCoord.x * fw;
    mediump float py = vTexCoord.y * fh;

    if (uEditMode > 0.5) {
        // Edit mode: show full image, draw red border around crop area
        highp vec4 pixel = texture2D(uTex, vTexCoord);

        // Crop border in pixel coords
        mediump float cx1 = uSrcCrop.x * fw;
        mediump float cx2 = (uSrcCrop.x + uSrcCrop.z) * fw;
        mediump float cy1 = uSrcCrop.y * fh;
        mediump float cy2 = (uSrcCrop.y + uSrcCrop.w) * fh;

        // Border thickness in pixels
        mediump float border = 2.0;

        // Each border segment is clamped to the opposite axis
        bool onLeftBorder   = abs(px - cx1) < border && py >= cy1 && py <= cy2;
        bool onRightBorder  = abs(px - cx2) < border && py >= cy1 && py <= cy2;
        bool onBottomBorder = abs(py - cy1) < border && px >= cx1 && px <= cx2;
        bool onTopBorder    = abs(py - cy2) < border && px >= cx1 && px <= cx2;
        bool onBorder = onLeftBorder || onRightBorder || onBottomBorder || onTopBorder;

        gl_FragColor = onBorder ? uEditColor : pixel;
    } else {
        // Normal crop mode
        bool inDst = vTexCoord.x >= uDstCrop.x &&
                     vTexCoord.x <= uDstCrop.x + uDstCrop.z &&
                     vTexCoord.y >= uDstCrop.y &&
                     vTexCoord.y <= uDstCrop.y + uDstCrop.w;

        if (inDst) {
            // Map destination UV to source UV
            mediump vec2 srcUV = (vTexCoord - uDstCrop.xy) / uDstCrop.zw * uSrcCrop.zw + uSrcCrop.xy;
            highp vec4 src = texture2D(uTex, srcUV);

            // Alpha-blend the crop pixel over the fill color.
            // This ensures transparent parts of the crop show the fill color
            // instead of replacing it entirely.
            mediump float outA = src.a + uFillColor.a * (1.0 - src.a);
            if (outA > 0.0) {
                mediump vec3 outRgb = (src.rgb * src.a + uFillColor.rgb * uFillColor.a * (1.0 - src.a)) / outA;
                gl_FragColor = vec4(outRgb, outA);
            } else {
                gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
            }
        } else {
            gl_FragColor = uFillColor;
        }
    }
}
