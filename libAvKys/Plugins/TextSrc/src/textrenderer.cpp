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

#include <QColor>
#include <QFont>
#include <QGlyphRun>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QRawFont>
#include <QStringList>
#include <QtMath>
#include <QTextLayout>
#include <QTextLine>
#include <QTextOption>
#include <memory>
#include <vector>

#include "textrenderer.h"

const int TextRenderer::minFrameSize = 16;
const int TextRenderer::maxFrameSize = 4096;

namespace {
    struct TextBlock
    {
        std::vector<std::unique_ptr<QTextLayout>> paragraphs;
        QRectF inkRect;   // Union of the text lines, origin at the top left.
        qreal width {0.0};
        qreal height {0.0};
    };

    QColor toColor(QRgb color)
    {
        return QColor(qRed(color), qGreen(color), qBlue(color), qAlpha(color));
    }

    QStringList splitParagraphs(const QString &text)
    {
        auto normalized = text;
        normalized.replace("\r\n", "\n");
        normalized.replace('\r', '\n');
        normalized.replace(QChar(0x2028), '\n');
        normalized.replace(QChar(0x2029), '\n');

        return normalized.split('\n', Qt::KeepEmptyParts);
    }

    Qt::Alignment toQtAlignment(TextRenderer::HorizontalAlignment alignment)
    {
        switch (alignment) {
        case TextRenderer::HorizontalAlignmentLeft:
            return Qt::AlignLeft;
        case TextRenderer::HorizontalAlignmentRight:
            return Qt::AlignRight;
        case TextRenderer::HorizontalAlignmentJustify:
            return Qt::AlignJustify;
        case TextRenderer::HorizontalAlignmentCenter:
        default:
            break;
        }

        return Qt::AlignHCenter;
    }

    QFont makeFont(const TextRenderer::Params &params)
    {
        QFont font;

        if (!params.fontFamily.isEmpty())
            font.setFamily(params.fontFamily);

        font.setPixelSize(qMax(1, params.fontSize));
        font.setBold(params.bold);
        font.setItalic(params.italic);
        font.setUnderline(params.underline);
        font.setStrikeOut(params.strikeout);

        if (!qFuzzyIsNull(params.letterSpacing))
            font.setLetterSpacing(QFont::AbsoluteSpacing, params.letterSpacing);

        /* Hinting changes the shape of the glyphs for small sizes, which
         * makes the glyphs drawn by the text layout differ from the outlines
         * used for the stroke. Disable it, the frames get scaled anyway.
         */
        font.setHintingPreference(QFont::PreferNoHinting);
        font.setStyleStrategy(params.antialiasing?
                                  QFont::PreferAntialias:
                                  QFont::NoAntialias);

        return font;
    }

    TextBlock layoutBlock(const QStringList &paragraphs,
                          const QFont &font,
                          qreal lineWidth,
                          bool wordWrap,
                          Qt::Alignment alignment,
                          qreal lineSpacing)
    {
        TextBlock block;
        QTextOption option;
        option.setAlignment(alignment);
        option.setWrapMode(wordWrap?
                               QTextOption::WrapAtWordBoundaryOrAnywhere:
                               QTextOption::NoWrap);
        option.setTextDirection(Qt::LayoutDirectionAuto);

        auto factor = lineSpacing / 100.0;
        qreal y = 0.0;
        QRectF ink;

        for (auto &paragraph: paragraphs) {
            auto layout = std::make_unique<QTextLayout>(paragraph, font);
            layout->setTextOption(option);
            layout->setCacheEnabled(true);
            layout->beginLayout();

            forever {
                auto line = layout->createLine();

                if (!line.isValid())
                    break;

                line.setLineWidth(lineWidth);
                line.setPosition(QPointF(0.0, y));
                auto rect = line.naturalTextRect();

                // Empty lines still need to occupy their height.
                if (rect.height() <= 0.0)
                    rect.setHeight(line.height());

                ink = ink.isNull()? rect: ink.united(rect);
                block.width = qMax(block.width, line.naturalTextWidth());
                y += line.height() * factor;
            }

            layout->endLayout();
            block.paragraphs.push_back(std::move(layout));
        }

        block.inkRect = ink;
        block.height = ink.isNull()? 0.0: ink.bottom();

        return block;
    }

    QPainterPath glyphPath(const TextBlock &block, const QPointF &origin)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);

        for (auto &layout: block.paragraphs) {
            for (auto &run: layout->glyphRuns()) {
                auto rawFont = run.rawFont();
                auto indexes = run.glyphIndexes();
                auto positions = run.positions();

                for (int i = 0; i < indexes.size(); i++) {
                    auto glyph = rawFont.pathForGlyph(indexes[i]);

                    if (glyph.isEmpty())
                        continue;

                    glyph.translate(origin + positions[i]);
                    path.addPath(glyph);
                }
            }
        }

        return path;
    }

    void blurLine(quint32 *data,
                  int stride,
                  int length,
                  int radius,
                  std::vector<quint32> &tmp)
    {
        for (int i = 0; i < length; i++)
            tmp[size_t(i)] = data[i * stride];

        auto window = quint32(2 * radius + 1);
        quint32 sa = 0;
        quint32 sr = 0;
        quint32 sg = 0;
        quint32 sb = 0;

        auto accumulate = [&] (int i, bool add) {
            if (i < 0 || i >= length)
                return;

            auto pixel = tmp[size_t(i)];
            auto a = pixel >> 24;
            auto r = (pixel >> 16) & 0xff;
            auto g = (pixel >> 8) & 0xff;
            auto b = pixel & 0xff;

            if (add) {
                sa += a;
                sr += r;
                sg += g;
                sb += b;
            } else {
                sa -= a;
                sr -= r;
                sg -= g;
                sb -= b;
            }
        };

        for (int i = -radius; i <= radius; i++)
            accumulate(i, true);

        auto half = window / 2;

        for (int x = 0; x < length; x++) {
            auto a = (sa + half) / window;
            auto r = (sr + half) / window;
            auto g = (sg + half) / window;
            auto b = (sb + half) / window;
            data[x * stride] = (a << 24) | (r << 16) | (g << 8) | b;

            accumulate(x - radius, false);
            accumulate(x + radius + 1, true);
        }
    }

    /* Approximates a gaussian blur with three box blur passes. The image
     * must be in the QImage::Format_ARGB32_Premultiplied format.
     */
    void blurImage(QImage &image, int blur)
    {
        if (blur < 1)
            return;

        // Sigma ~ blur / 2, like the blur radius from CSS shadows.
        auto radius = qMax(1, (blur + 1) / 2);
        auto width = image.width();
        auto height = image.height();
        std::vector<quint32> tmp(size_t(qMax(width, height)));
        auto stride = int(image.bytesPerLine() / sizeof(quint32));

        for (int pass = 0; pass < 3; pass++) {
            for (int y = 0; y < height; y++)
                blurLine(reinterpret_cast<quint32 *>(image.scanLine(y)),
                         1,
                         width,
                         radius,
                         tmp);

            auto bits = reinterpret_cast<quint32 *>(image.bits());

            for (int x = 0; x < width; x++)
                blurLine(bits + x, stride, height, radius, tmp);
        }
    }


    const qreal sqrt2 = 1.41421356237309505;
    const qreal pi = 3.14159265358979324;

    // How much the bumps or spikes stick out of the base ellipse.
    qreal shapeOvershoot(TextRenderer::BackgroundShape shape)
    {
        switch (shape) {
        case TextRenderer::BackgroundShapeCloud:
            return 0.14;
        case TextRenderer::BackgroundShapeBurst:
            return 0.22;
        default:
            break;
        }

        return 0.0;
    }

    QRectF scaledAboutCenter(const QRectF &rect, qreal xScale, qreal yScale)
    {
        QSizeF size(rect.width() * xScale, rect.height() * yScale);
        QRectF scaled(QPointF(), size);
        scaled.moveCenter(rect.center());

        return scaled;
    }

    qreal ellipsePerimeter(qreal a, qreal b)
    {
        // Ramanujan's approximation.
        return pi * (3.0 * (a + b) - qSqrt((3.0 * a + b) * (a + 3.0 * b)));
    }

    // Deterministic pseudo random number in [0, 1] so the shape is stable.
    qreal noise(int i)
    {
        quint32 x = quint32(i + 1) * 2654435761u;
        x ^= x >> 15;
        x *= 2246822519u;
        x ^= x >> 13;

        return qreal(x & 0xffff) / 65535.0;
    }

    QPointF ellipsePoint(const QRectF &rect, qreal angle, qreal scale = 1.0)
    {
        auto c = rect.center();

        return {c.x() + rect.width() / 2.0 * scale * qCos(angle),
                c.y() + rect.height() / 2.0 * scale * qSin(angle)};
    }

    QPainterPath cloudPath(const QRectF &rect, qreal overshoot)
    {
        auto a = rect.width() / 2.0;
        auto b = rect.height() / 2.0;
        auto bumps = qBound(8,
                            qRound(ellipsePerimeter(a, b)
                                   / (0.9 * qMin(a, b) + 30.0)),
                            30);
        auto c = rect.center();
        QPainterPath path;
        path.moveTo(ellipsePoint(rect, 0.0));

        for (int i = 0; i < bumps; i++) {
            auto t0 = 2.0 * pi * i / bumps;
            auto t1 = 2.0 * pi * (i + 1) / bumps;
            auto p1 = ellipsePoint(rect, t1);
            auto chord = (ellipsePoint(rect, t0) + p1) / 2.0;

            // The peak of a quadratic curve is halfway to its control point.
            auto peak = c + (ellipsePoint(rect, (t0 + t1) / 2.0) - c)
                            * (1.0 + overshoot);
            path.quadTo(2.0 * peak - chord, p1);
        }

        path.closeSubpath();

        return path;
    }

    QPainterPath burstPath(const QRectF &rect, qreal overshoot)
    {
        auto a = rect.width() / 2.0;
        auto b = rect.height() / 2.0;
        auto spikes = qBound(10,
                             qRound(ellipsePerimeter(a, b)
                                    / (0.5 * qMin(a, b) + 20.0)),
                             40);
        auto c = rect.center();
        QPainterPath path;

        for (int i = 0; i < spikes; i++) {
            auto t0 = 2.0 * pi * i / spikes;
            auto t1 = 2.0 * pi * (i + 0.5) / spikes;

            // Every spike has a slightly different length.
            auto outer = 1.0 + overshoot * (0.55 + 0.45 * noise(i));
            auto spike = c + (ellipsePoint(rect, t0) - c) * outer;
            auto valley = ellipsePoint(rect, t1, 1.0 - 0.04 * noise(i + 97));

            if (i == 0)
                path.moveTo(spike);
            else
                path.lineTo(spike);

            path.lineTo(valley);
        }

        path.closeSubpath();

        return path;
    }

    QPainterPath bodyPath(const TextRenderer::Params &params,
                          const QRectF &rect,
                          qreal overshoot)
    {
        QPainterPath path;

        switch (params.backgroundShape) {
        case TextRenderer::BackgroundShapeEllipse:
            path.addEllipse(rect);

            break;
        case TextRenderer::BackgroundShapeCloud:
            path = cloudPath(rect, overshoot);

            break;
        case TextRenderer::BackgroundShapeBurst:
            path = burstPath(rect, overshoot);

            break;
        default: {
            auto radius = qBound(0.0,
                                 qreal(params.backgroundRadius),
                                 qMin(rect.width(), rect.height()) / 2.0);
            path.addRoundedRect(rect, radius, radius);

            break;
        }
        }

        return path;
    }

    // Direction of the tail, as a unit vector.
    QPointF tailDirection(const TextRenderer::Params &params)
    {
        auto angle = params.tailAngle * pi / 180.0;

        return {qCos(angle), qSin(angle)};
    }

    QPainterPath tailPath(const TextRenderer::Params &params,
                          const QRectF &rect,
                          qreal overshoot)
    {
        QPainterPath path;

        if (params.tailStyle == TextRenderer::TailStyleNone
            || params.tailLength <= 0)
            return path;

        auto dir = tailDirection(params);
        QPointF side(-dir.y(), dir.x());
        auto center = rect.center();
        auto length = qreal(params.tailLength);
        auto halfWidth = qMax(1.0, params.tailWidth / 2.0);

        // Where the tail leaves the balloon.
        qreal reach;

        if (params.backgroundShape == TextRenderer::BackgroundShapeRectangle) {
            auto tx = qFuzzyIsNull(dir.x())?
                          1.0e9: rect.width() / 2.0 / qAbs(dir.x());
            auto ty = qFuzzyIsNull(dir.y())?
                          1.0e9: rect.height() / 2.0 / qAbs(dir.y());
            reach = qMin(tx, ty);
        } else {
            auto a = rect.width() / 2.0;
            auto b = rect.height() / 2.0;
            reach = 1.0 / qSqrt(qPow(dir.x() / a, 2.0) + qPow(dir.y() / b, 2.0));
        }

        // Clouds and bursts stick out past the base ellipse.
        auto inner = center + dir * reach;
        auto outer = center + dir * (reach * (1.0 + overshoot));

        switch (params.tailStyle) {
        case TextRenderer::TailStylePointer: {
            /* The base is hidden inside the balloon, so it merges with the
             * body when joined.
             */
            auto base = inner - dir * (halfWidth * 0.9);
            auto tip = outer + dir * length;
            auto distance = QPointF::dotProduct(tip - base, dir);
            auto bend = side * (halfWidth * 0.2);

            // Slightly concave sides give it the usual comic look.
            path.moveTo(base + side * halfWidth);
            path.quadTo(base + dir * (distance * 0.45) + bend, tip);
            path.quadTo(base + dir * (distance * 0.45) - bend,
                        base - side * halfWidth);
            path.closeSubpath();

            break;
        }
        case TextRenderer::TailStyleThought: {
            /* Three circles of decreasing size. The width sets the size
             * of the first one, the length sets how far the last one is,
             * so the circles shrink if the tail is too short for them.
             */
            const qreal ratios[] = {1.0, 0.65, 0.4};
            auto overlap = 0.25;
            auto unit = ratios[0] * (1.0 + overlap)
                        + 2.0 * ratios[1]
                        + 2.0 * ratios[2];
            auto scale = qMin(1.0,
                              length / (halfWidth * (unit + 0.6 * ratios[0])));
            qreal r[3];

            for (int i = 0; i < 3; i++)
                r[i] = halfWidth * ratios[i] * scale;

            auto fixed = r[0] * (1.0 + overlap) + 2.0 * r[1] + 2.0 * r[2];
            auto gap = qMax(0.3 * r[0], (length - fixed) / 2.0);
            auto d1 = r[0] * overlap;
            auto d2 = d1 + r[0] + gap + r[1];
            auto d3 = d2 + r[1] + gap + r[2];
            const qreal distances[] = {d1, d2, d3};

            for (int i = 0; i < 3; i++)
                path.addEllipse(outer + dir * distances[i], r[i], r[i]);

            break;
        }
        case TextRenderer::TailStyleLightning: {
            auto base = inner - dir * (halfWidth * 0.3);
            auto total = QPointF::dotProduct(outer + dir * length - base, dir);

            // Polygon in a unit square: x across the tail, y along it.
            static const QPointF points[] = {
                {0.00, 0.00}, {1.00, 0.00}, {0.62, 0.45}, {0.92, 0.45},
                {0.35, 1.00}, {0.42, 0.60}, {0.10, 0.60}
            };

            for (size_t i = 0; i < sizeof(points) / sizeof(QPointF); i++) {
                auto p = base
                         + side * ((points[i].x() - 0.5) * 2.0 * halfWidth)
                         + dir * (points[i].y() * total);

                if (i == 0)
                    path.moveTo(p);
                else
                    path.lineTo(p);
            }

            path.closeSubpath();

            break;
        }
        default:
            break;
        }

        return path;
    }

    void setRenderHints(QPainter &painter, bool antialiasing)
    {
        painter.setRenderHint(QPainter::Antialiasing, antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing, antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, antialiasing);
    }
}

QSize TextRenderer::clampedSize(int width, int height)
{
    return {qBound(minFrameSize, width, maxFrameSize),
            qBound(minFrameSize, height, maxFrameSize)};
}

QImage TextRenderer::render(const Params &params)
{
    auto size = clampedSize(params.width, params.height);
    auto width = size.width();
    auto height = size.height();

    QImage frame(size, QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);

    auto font = makeFont(params);
    auto padding = qMax(0, params.padding);
    auto paragraphs = splitParagraphs(params.text);
    auto alignment = toQtAlignment(params.horizontalAlignment);
    auto lineSpacing = qBound(10.0, params.lineSpacing, 1000.0);

    // Balloon geometry.

    auto borderWidth = params.backgroundBorderEnabled
                       && qAlpha(params.backgroundBorderColor) > 0?
                           qMax(0, params.backgroundBorderWidth): 0;
    auto hasBackground = qAlpha(params.backgroundColor) > 0 || borderWidth > 0;
    auto shape = params.backgroundShape;
    auto overshoot = shapeOvershoot(shape);
    QRectF frameRect(0.0, 0.0, width, height);
    QRectF balloonArea = frameRect;
    QRectF textArea = frameRect;

    if (hasBackground) {
        /* Leave room on the side where the tail points to, and for the
         * bumps and spikes of clouds and bursts.
         */
        if (params.tailStyle != TailStyleNone && params.tailLength > 0) {
            auto dir = tailDirection(params);
            auto length = qreal(params.tailLength);
            balloonArea.adjust(qMax(0.0, -dir.x()) * length,
                               qMax(0.0, -dir.y()) * length,
                               -qMax(0.0, dir.x()) * length,
                               -qMax(0.0, dir.y()) * length);
        }

        if (params.backgroundShadowEnabled
            && qAlpha(params.backgroundShadowColor) > 0) {
            qreal blur = qMax(0, params.backgroundShadowBlur);
            qreal dx = params.backgroundShadowOffsetX;
            qreal dy = params.backgroundShadowOffsetY;
            balloonArea.adjust(qMax(0.0, -dx) + blur,
                               qMax(0.0, -dy) + blur,
                               -(qMax(0.0, dx) + blur),
                               -(qMax(0.0, dy) + blur));
        }

        if (balloonArea.width() < 8.0 || balloonArea.height() < 8.0)
            balloonArea = scaledAboutCenter(frameRect, 0.5, 0.5);

        balloonArea = scaledAboutCenter(balloonArea,
                                        1.0 / (1.0 + overshoot),
                                        1.0 / (1.0 + overshoot));

        // The text must fit in the rectangle inscribed in an ellipse.
        textArea = shape == BackgroundShapeRectangle?
                       balloonArea:
                       scaledAboutCenter(balloonArea, 1.0 / sqrt2, 1.0 / sqrt2);
    }

    auto pad = padding + borderWidth;
    auto availWidth = qMax(1.0, textArea.width() - 2.0 * pad);
    auto availHeight = qMax(1.0, textArea.height() - 2.0 * pad);

    TextBlock block;
    qreal blockX = textArea.left() + pad;

    if (params.wordWrap) {
        block = layoutBlock(paragraphs,
                            font,
                            availWidth,
                            true,
                            alignment,
                            lineSpacing);
    } else {
        /* Without wrapping there is no line width to align the text to, so
         * measure the widest line first, and then align every line inside a
         * block of that width. The block itself is then positioned in the
         * available area.
         */
        auto measure = layoutBlock(paragraphs,
                                   font,
                                   1.0e6,
                                   false,
                                   Qt::AlignLeft,
                                   lineSpacing);
        auto blockWidth = qMax(1.0, measure.width);

        // Justification makes no sense if the lines can't wrap.
        if (alignment & Qt::AlignJustify)
            alignment = Qt::AlignLeft;

        block = layoutBlock(paragraphs,
                            font,
                            blockWidth,
                            false,
                            alignment,
                            lineSpacing);

        switch (params.horizontalAlignment) {
        case HorizontalAlignmentCenter:
            blockX += (availWidth - blockWidth) / 2.0;

            break;
        case HorizontalAlignmentRight:
            blockX += availWidth - blockWidth;

            break;
        default:
            break;
        }
    }

    qreal blockY = textArea.top() + pad;

    switch (params.verticalAlignment) {
    case VerticalAlignmentMiddle:
        blockY += (availHeight - block.height) / 2.0;

        break;
    case VerticalAlignmentBottom:
        blockY += availHeight - block.height;

        break;
    default:
        break;
    }

    // Snap to the pixel grid so the stroke and the glyphs match exactly.
    QPointF origin(qRound(blockX), qRound(blockY));

    // Background.

    auto drawBackground = hasBackground;
    QRectF coreRect = balloonArea;

    if (params.fitBackground) {
        if (params.text.isEmpty()) {
            drawBackground = false;
        } else {
            auto margin = pad
                          + (params.outlineEnabled?
                                 qMax(0, params.outlineWidth):
                                 0);
            coreRect = block.inkRect.translated(origin)
                                    .adjusted(-margin, -margin, margin, margin);

            // An ellipse that circumscribes the rectangle.
            if (shape != BackgroundShapeRectangle)
                coreRect = scaledAboutCenter(coreRect, sqrt2, sqrt2);
        }
    }

    QImage balloonLayer;
    QImage balloonShadow;

    if (drawBackground) {
        auto body = bodyPath(params, coreRect, overshoot);
        auto tail = tailPath(params, coreRect, overshoot);
        auto silhouette = tail.isEmpty()? body: body.united(tail);
        silhouette.setFillRule(Qt::WindingFill);

        balloonLayer = QImage(size, QImage::Format_ARGB32_Premultiplied);
        balloonLayer.fill(Qt::transparent);

        QPainter painter(&balloonLayer);
        setRenderHints(painter, params.antialiasing);
        painter.fillPath(silhouette, toColor(params.backgroundColor));

        if (borderWidth > 0) {
            /* The border goes inside the silhouette. Cut its band out of
             * the fill, so translucent colors don't mix, and draw it
             * from a separate layer, keeping only the part that lies over
             * the silhouette.
             */
            QPainterPathStroker stroker;
            stroker.setWidth(2.0 * borderWidth);
            stroker.setJoinStyle(Qt::RoundJoin);
            auto band = stroker.createStroke(silhouette);

            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.fillPath(band, Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            QImage fill(size, QImage::Format_ARGB32_Premultiplied);
            fill.fill(Qt::transparent);

            {
                QPainter fillPainter(&fill);
                setRenderHints(fillPainter, params.antialiasing);
                fillPainter.fillPath(silhouette,
                                     toColor(params.backgroundBorderColor));
            }

            QImage border(size, QImage::Format_ARGB32_Premultiplied);
            border.fill(Qt::transparent);

            {
                QPainter borderPainter(&border);
                setRenderHints(borderPainter, params.antialiasing);
                borderPainter.fillPath(band, Qt::black);
                borderPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                borderPainter.drawImage(0, 0, fill);
            }

            painter.drawImage(0, 0, border);
        }

        painter.end();

        if (params.backgroundShadowEnabled
            && qAlpha(params.backgroundShadowColor) > 0) {
            balloonShadow = QImage(size, QImage::Format_ARGB32_Premultiplied);
            balloonShadow.fill(Qt::transparent);

            {
                QPainter shadowPainter(&balloonShadow);
                setRenderHints(shadowPainter, params.antialiasing);
                shadowPainter.translate(params.backgroundShadowOffsetX,
                                        params.backgroundShadowOffsetY);
                shadowPainter.fillPath(silhouette,
                                       toColor(params.backgroundShadowColor));
            }

            blurImage(balloonShadow, params.backgroundShadowBlur);

            /* Like a CSS box shadow, keep it outside of the balloon, so it
             * doesn't show through a translucent or empty background.
             */
            QPainter shadowPainter(&balloonShadow);
            setRenderHints(shadowPainter, params.antialiasing);
            shadowPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            shadowPainter.fillPath(silhouette, Qt::black);
        }
    }

    // Text layer: outline + glyphs.

    QImage textLayer(size, QImage::Format_ARGB32_Premultiplied);
    textLayer.fill(Qt::transparent);

    {
        QPainter painter(&textLayer);
        setRenderHints(painter, params.antialiasing);

        if (params.outlineEnabled && params.outlineWidth > 0) {
            auto path = glyphPath(block, origin);
            QPainterPathStroker stroker;
            stroker.setWidth(2.0 * params.outlineWidth);
            stroker.setJoinStyle(Qt::RoundJoin);
            stroker.setCapStyle(Qt::RoundCap);
            auto stroke = stroker.createStroke(path);
            painter.fillPath(stroke, toColor(params.outlineColor));

            /* A translucent text would show the outline through it, so cut
             * the glyphs out before drawing them.
             */
            if (qAlpha(params.foregroundColor) < 255) {
                painter.setCompositionMode(QPainter::CompositionMode_Clear);
                painter.fillPath(path, Qt::transparent);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            }
        }

        painter.setPen(toColor(params.foregroundColor));

        for (auto &layout: block.paragraphs)
            layout->draw(&painter, origin);
    }

    // Shadow, built from the silhouette of the text layer.

    QImage shadow;

    if (params.shadowEnabled && qAlpha(params.shadowColor) > 0) {
        shadow = QImage(size, QImage::Format_ARGB32_Premultiplied);
        shadow.fill(Qt::transparent);

        {
            QPainter painter(&shadow);
            painter.drawImage(params.shadowOffsetX,
                              params.shadowOffsetY,
                              textLayer);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(shadow.rect(), toColor(params.shadowColor));
        }

        blurImage(shadow, params.shadowBlur);
    }

    // Compose.

    {
        QPainter painter(&frame);

        if (!balloonShadow.isNull())
            painter.drawImage(0, 0, balloonShadow);

        if (!balloonLayer.isNull())
            painter.drawImage(0, 0, balloonLayer);

        if (!shadow.isNull())
            painter.drawImage(0, 0, shadow);

        painter.drawImage(0, 0, textLayer);
    }

    return frame.convertToFormat(QImage::Format_ARGB32);
}
