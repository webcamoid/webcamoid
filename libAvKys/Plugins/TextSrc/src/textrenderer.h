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

#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <QImage>
#include <QString>
#include <qrgb.h>

/* Renders a block of styled text into a transparent ARGB image.
 *
 * This class only depends on QtGui, it is thread safe (it has no shared state)
 * and can be called from any thread as long as a QGuiApplication exists.
 */
class TextRenderer
{
    public:
        enum HorizontalAlignment
        {
            HorizontalAlignmentLeft,
            HorizontalAlignmentCenter,
            HorizontalAlignmentRight,
            HorizontalAlignmentJustify
        };

        enum VerticalAlignment
        {
            VerticalAlignmentTop,
            VerticalAlignmentMiddle,
            VerticalAlignmentBottom
        };

        enum BackgroundShape
        {
            BackgroundShapeRectangle,
            BackgroundShapeEllipse,
            BackgroundShapeCloud,   // Thought balloon.
            BackgroundShapeBurst    // Shout balloon.
        };

        enum TailStyle
        {
            TailStyleNone,
            TailStylePointer,       // Speech balloon.
            TailStyleThought,       // Chain of circles.
            TailStyleLightning
        };

        struct Params
        {
            QString text;

            // Frame size in pixels.
            int width {1280};
            int height {720};

            // Font. Sizes are in pixels so the result doesn't depend on the DPI.
            QString fontFamily;
            int fontSize {64};
            bool bold {false};
            bool italic {false};
            bool underline {false};
            bool strikeout {false};
            qreal letterSpacing {0.0};    // Pixels.
            qreal lineSpacing {100.0};    // Percent of the line height.
            bool antialiasing {true};

            // Layout.
            HorizontalAlignment horizontalAlignment {HorizontalAlignmentCenter};
            VerticalAlignment verticalAlignment {VerticalAlignmentMiddle};
            bool wordWrap {true};
            int padding {16};

            // Colors, ARGB with straight (non premultiplied) alpha.
            QRgb foregroundColor {qRgba(255, 255, 255, 255)};
            QRgb backgroundColor {qRgba(0, 0, 0, 0)};

            // Background balloon.
            BackgroundShape backgroundShape {BackgroundShapeRectangle};
            int backgroundRadius {0};     // Only for the rectangle.
            bool fitBackground {true};    // Hug the text instead of filling the frame.
            bool backgroundBorderEnabled {false};
            QRgb backgroundBorderColor {qRgba(0, 0, 0, 255)};
            int backgroundBorderWidth {3};

            // Balloon shadow, cast by the whole silhouette (body and tail).
            bool backgroundShadowEnabled {false};
            QRgb backgroundShadowColor {qRgba(0, 0, 0, 130)};
            int backgroundShadowOffsetX {5};
            int backgroundShadowOffsetY {5};
            int backgroundShadowBlur {8};

            // Balloon tail.
            TailStyle tailStyle {TailStyleNone};
            int tailAngle {100};          // Degrees, 0 = right, 90 = down.
            int tailLength {60};
            int tailWidth {50};

            // Outline.
            bool outlineEnabled {false};
            QRgb outlineColor {qRgba(0, 0, 0, 255)};
            int outlineWidth {3};

            // Shadow.
            bool shadowEnabled {false};
            QRgb shadowColor {qRgba(0, 0, 0, 160)};
            int shadowOffsetX {4};
            int shadowOffsetY {4};
            int shadowBlur {6};
        };

        static const int minFrameSize;
        static const int maxFrameSize;

        /* Returns an image in the QImage::Format_ARGB32 format (straight
         * alpha). The image size is the frame size from the parameters,
         * clamped to [minFrameSize, maxFrameSize].
         */
        static QImage render(const Params &params);
        static QSize clampedSize(int width, int height);
};

#endif // TEXTRENDERER_H
