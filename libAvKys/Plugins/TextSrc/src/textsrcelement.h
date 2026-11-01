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

#ifndef TEXTSRCELEMENT_H
#define TEXTSRCELEMENT_H

#include <qrgb.h>
#include <iak/akmultimediasourceelement.h>

class TextSrcElementPrivate;

class TextSrcElement: public AkMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(QStringList fontFamilies
               READ fontFamilies
               CONSTANT)
    Q_PROPERTY(QString fileError
               READ fileError
               NOTIFY fileErrorChanged)
    Q_PROPERTY(QString summary
               READ summary
               NOTIFY summaryChanged)
    Q_PROPERTY(QString text
               READ text
               WRITE setText
               RESET resetText
               NOTIFY textChanged)
    Q_PROPERTY(TextSource textSource
               READ textSource
               WRITE setTextSource
               RESET resetTextSource
               NOTIFY textSourceChanged)
    Q_PROPERTY(QString textFile
               READ textFile
               WRITE setTextFile
               RESET resetTextFile
               NOTIFY textFileChanged)
    Q_PROPERTY(QString fontFamily
               READ fontFamily
               WRITE setFontFamily
               RESET resetFontFamily
               NOTIFY fontFamilyChanged)
    Q_PROPERTY(int fontSize
               READ fontSize
               WRITE setFontSize
               RESET resetFontSize
               NOTIFY fontSizeChanged)
    Q_PROPERTY(bool bold
               READ bold
               WRITE setBold
               RESET resetBold
               NOTIFY boldChanged)
    Q_PROPERTY(bool italic
               READ italic
               WRITE setItalic
               RESET resetItalic
               NOTIFY italicChanged)
    Q_PROPERTY(bool underline
               READ underline
               WRITE setUnderline
               RESET resetUnderline
               NOTIFY underlineChanged)
    Q_PROPERTY(bool strikeout
               READ strikeout
               WRITE setStrikeout
               RESET resetStrikeout
               NOTIFY strikeoutChanged)
    Q_PROPERTY(qreal letterSpacing
               READ letterSpacing
               WRITE setLetterSpacing
               RESET resetLetterSpacing
               NOTIFY letterSpacingChanged)
    Q_PROPERTY(qreal lineSpacing
               READ lineSpacing
               WRITE setLineSpacing
               RESET resetLineSpacing
               NOTIFY lineSpacingChanged)
    Q_PROPERTY(bool antialiasing
               READ antialiasing
               WRITE setAntialiasing
               RESET resetAntialiasing
               NOTIFY antialiasingChanged)
    Q_PROPERTY(HorizontalAlignment horizontalAlignment
               READ horizontalAlignment
               WRITE setHorizontalAlignment
               RESET resetHorizontalAlignment
               NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VerticalAlignment verticalAlignment
               READ verticalAlignment
               WRITE setVerticalAlignment
               RESET resetVerticalAlignment
               NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(bool wordWrap
               READ wordWrap
               WRITE setWordWrap
               RESET resetWordWrap
               NOTIFY wordWrapChanged)
    Q_PROPERTY(int padding
               READ padding
               WRITE setPadding
               RESET resetPadding
               NOTIFY paddingChanged)
    Q_PROPERTY(int frameWidth
               READ frameWidth
               WRITE setFrameWidth
               RESET resetFrameWidth
               NOTIFY frameWidthChanged)
    Q_PROPERTY(int frameHeight
               READ frameHeight
               WRITE setFrameHeight
               RESET resetFrameHeight
               NOTIFY frameHeightChanged)
    Q_PROPERTY(QRgb foregroundColor
               READ foregroundColor
               WRITE setForegroundColor
               RESET resetForegroundColor
               NOTIFY foregroundColorChanged)
    Q_PROPERTY(QRgb backgroundColor
               READ backgroundColor
               WRITE setBackgroundColor
               RESET resetBackgroundColor
               NOTIFY backgroundColorChanged)
    Q_PROPERTY(BackgroundShape backgroundShape
               READ backgroundShape
               WRITE setBackgroundShape
               RESET resetBackgroundShape
               NOTIFY backgroundShapeChanged)
    Q_PROPERTY(int backgroundRadius
               READ backgroundRadius
               WRITE setBackgroundRadius
               RESET resetBackgroundRadius
               NOTIFY backgroundRadiusChanged)
    Q_PROPERTY(bool backgroundBorderEnabled
               READ backgroundBorderEnabled
               WRITE setBackgroundBorderEnabled
               RESET resetBackgroundBorderEnabled
               NOTIFY backgroundBorderEnabledChanged)
    Q_PROPERTY(QRgb backgroundBorderColor
               READ backgroundBorderColor
               WRITE setBackgroundBorderColor
               RESET resetBackgroundBorderColor
               NOTIFY backgroundBorderColorChanged)
    Q_PROPERTY(int backgroundBorderWidth
               READ backgroundBorderWidth
               WRITE setBackgroundBorderWidth
               RESET resetBackgroundBorderWidth
               NOTIFY backgroundBorderWidthChanged)
    Q_PROPERTY(bool backgroundShadowEnabled
               READ backgroundShadowEnabled
               WRITE setBackgroundShadowEnabled
               RESET resetBackgroundShadowEnabled
               NOTIFY backgroundShadowEnabledChanged)
    Q_PROPERTY(QRgb backgroundShadowColor
               READ backgroundShadowColor
               WRITE setBackgroundShadowColor
               RESET resetBackgroundShadowColor
               NOTIFY backgroundShadowColorChanged)
    Q_PROPERTY(int backgroundShadowOffsetX
               READ backgroundShadowOffsetX
               WRITE setBackgroundShadowOffsetX
               RESET resetBackgroundShadowOffsetX
               NOTIFY backgroundShadowOffsetXChanged)
    Q_PROPERTY(int backgroundShadowOffsetY
               READ backgroundShadowOffsetY
               WRITE setBackgroundShadowOffsetY
               RESET resetBackgroundShadowOffsetY
               NOTIFY backgroundShadowOffsetYChanged)
    Q_PROPERTY(int backgroundShadowBlur
               READ backgroundShadowBlur
               WRITE setBackgroundShadowBlur
               RESET resetBackgroundShadowBlur
               NOTIFY backgroundShadowBlurChanged)
    Q_PROPERTY(TailStyle tailStyle
               READ tailStyle
               WRITE setTailStyle
               RESET resetTailStyle
               NOTIFY tailStyleChanged)
    Q_PROPERTY(int tailAngle
               READ tailAngle
               WRITE setTailAngle
               RESET resetTailAngle
               NOTIFY tailAngleChanged)
    Q_PROPERTY(int tailLength
               READ tailLength
               WRITE setTailLength
               RESET resetTailLength
               NOTIFY tailLengthChanged)
    Q_PROPERTY(int tailWidth
               READ tailWidth
               WRITE setTailWidth
               RESET resetTailWidth
               NOTIFY tailWidthChanged)
    Q_PROPERTY(bool fitBackground
               READ fitBackground
               WRITE setFitBackground
               RESET resetFitBackground
               NOTIFY fitBackgroundChanged)
    Q_PROPERTY(bool outlineEnabled
               READ outlineEnabled
               WRITE setOutlineEnabled
               RESET resetOutlineEnabled
               NOTIFY outlineEnabledChanged)
    Q_PROPERTY(QRgb outlineColor
               READ outlineColor
               WRITE setOutlineColor
               RESET resetOutlineColor
               NOTIFY outlineColorChanged)
    Q_PROPERTY(int outlineWidth
               READ outlineWidth
               WRITE setOutlineWidth
               RESET resetOutlineWidth
               NOTIFY outlineWidthChanged)
    Q_PROPERTY(bool shadowEnabled
               READ shadowEnabled
               WRITE setShadowEnabled
               RESET resetShadowEnabled
               NOTIFY shadowEnabledChanged)
    Q_PROPERTY(QRgb shadowColor
               READ shadowColor
               WRITE setShadowColor
               RESET resetShadowColor
               NOTIFY shadowColorChanged)
    Q_PROPERTY(int shadowOffsetX
               READ shadowOffsetX
               WRITE setShadowOffsetX
               RESET resetShadowOffsetX
               NOTIFY shadowOffsetXChanged)
    Q_PROPERTY(int shadowOffsetY
               READ shadowOffsetY
               WRITE setShadowOffsetY
               RESET resetShadowOffsetY
               NOTIFY shadowOffsetYChanged)
    Q_PROPERTY(int shadowBlur
               READ shadowBlur
               WRITE setShadowBlur
               RESET resetShadowBlur
               NOTIFY shadowBlurChanged)

    public:
        enum TextSource
        {
            TextSourceInline,
            TextSourceFile
        };
        Q_ENUM(TextSource)

        enum HorizontalAlignment
        {
            HorizontalAlignmentLeft,
            HorizontalAlignmentCenter,
            HorizontalAlignmentRight,
            HorizontalAlignmentJustify
        };
        Q_ENUM(HorizontalAlignment)

        enum VerticalAlignment
        {
            VerticalAlignmentTop,
            VerticalAlignmentMiddle,
            VerticalAlignmentBottom
        };
        Q_ENUM(VerticalAlignment)

        enum BackgroundShape
        {
            BackgroundShapeRectangle,
            BackgroundShapeEllipse,
            BackgroundShapeCloud,
            BackgroundShapeBurst
        };
        Q_ENUM(BackgroundShape)

        enum TailStyle
        {
            TailStyleNone,
            TailStylePointer,
            TailStyleThought,
            TailStyleLightning
        };
        Q_ENUM(TailStyle)

        TextSrcElement();
        ~TextSrcElement();

        Q_INVOKABLE QStringList medias() override;
        Q_INVOKABLE QString media() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) override;
        Q_INVOKABLE QString description(const QString &media) override;
        Q_INVOKABLE AkCaps caps(int stream) override;
        Q_INVOKABLE QStringList fontFamilies() const;
        Q_INVOKABLE QString fileError() const;
        Q_INVOKABLE QString summary() const;
        Q_INVOKABLE QString text() const;
        Q_INVOKABLE TextSource textSource() const;
        Q_INVOKABLE QString textFile() const;
        Q_INVOKABLE QString fontFamily() const;
        Q_INVOKABLE int fontSize() const;
        Q_INVOKABLE bool bold() const;
        Q_INVOKABLE bool italic() const;
        Q_INVOKABLE bool underline() const;
        Q_INVOKABLE bool strikeout() const;
        Q_INVOKABLE qreal letterSpacing() const;
        Q_INVOKABLE qreal lineSpacing() const;
        Q_INVOKABLE bool antialiasing() const;
        Q_INVOKABLE HorizontalAlignment horizontalAlignment() const;
        Q_INVOKABLE VerticalAlignment verticalAlignment() const;
        Q_INVOKABLE bool wordWrap() const;
        Q_INVOKABLE int padding() const;
        Q_INVOKABLE int frameWidth() const;
        Q_INVOKABLE int frameHeight() const;
        Q_INVOKABLE QRgb foregroundColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE BackgroundShape backgroundShape() const;
        Q_INVOKABLE int backgroundRadius() const;
        Q_INVOKABLE bool backgroundBorderEnabled() const;
        Q_INVOKABLE QRgb backgroundBorderColor() const;
        Q_INVOKABLE int backgroundBorderWidth() const;
        Q_INVOKABLE bool backgroundShadowEnabled() const;
        Q_INVOKABLE QRgb backgroundShadowColor() const;
        Q_INVOKABLE int backgroundShadowOffsetX() const;
        Q_INVOKABLE int backgroundShadowOffsetY() const;
        Q_INVOKABLE int backgroundShadowBlur() const;
        Q_INVOKABLE TailStyle tailStyle() const;
        Q_INVOKABLE int tailAngle() const;
        Q_INVOKABLE int tailLength() const;
        Q_INVOKABLE int tailWidth() const;
        Q_INVOKABLE bool fitBackground() const;
        Q_INVOKABLE bool outlineEnabled() const;
        Q_INVOKABLE QRgb outlineColor() const;
        Q_INVOKABLE int outlineWidth() const;
        Q_INVOKABLE bool shadowEnabled() const;
        Q_INVOKABLE QRgb shadowColor() const;
        Q_INVOKABLE int shadowOffsetX() const;
        Q_INVOKABLE int shadowOffsetY() const;
        Q_INVOKABLE int shadowBlur() const;

        /* Deletes the stored configuration of this source. Call it when the
         * source is removed for good, not when the application closes.
         */
        Q_INVOKABLE void removeSettings();

    private:
        TextSrcElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void fileErrorChanged(const QString &fileError);
        void summaryChanged(const QString &summary);
        void error(const QString &message);
        void textChanged(const QString &text);
        void textSourceChanged(const TextSource &textSource);
        void textFileChanged(const QString &textFile);
        void fontFamilyChanged(const QString &fontFamily);
        void fontSizeChanged(int fontSize);
        void boldChanged(bool bold);
        void italicChanged(bool italic);
        void underlineChanged(bool underline);
        void strikeoutChanged(bool strikeout);
        void letterSpacingChanged(qreal letterSpacing);
        void lineSpacingChanged(qreal lineSpacing);
        void antialiasingChanged(bool antialiasing);
        void horizontalAlignmentChanged(const HorizontalAlignment &horizontalAlignment);
        void verticalAlignmentChanged(const VerticalAlignment &verticalAlignment);
        void wordWrapChanged(bool wordWrap);
        void paddingChanged(int padding);
        void frameWidthChanged(int frameWidth);
        void frameHeightChanged(int frameHeight);
        void foregroundColorChanged(QRgb foregroundColor);
        void backgroundColorChanged(QRgb backgroundColor);
        void backgroundShapeChanged(const BackgroundShape &backgroundShape);
        void backgroundRadiusChanged(int backgroundRadius);
        void backgroundBorderEnabledChanged(bool backgroundBorderEnabled);
        void backgroundBorderColorChanged(QRgb backgroundBorderColor);
        void backgroundBorderWidthChanged(int backgroundBorderWidth);
        void backgroundShadowEnabledChanged(bool backgroundShadowEnabled);
        void backgroundShadowColorChanged(QRgb backgroundShadowColor);
        void backgroundShadowOffsetXChanged(int backgroundShadowOffsetX);
        void backgroundShadowOffsetYChanged(int backgroundShadowOffsetY);
        void backgroundShadowBlurChanged(int backgroundShadowBlur);
        void tailStyleChanged(const TailStyle &tailStyle);
        void tailAngleChanged(int tailAngle);
        void tailLengthChanged(int tailLength);
        void tailWidthChanged(int tailWidth);
        void fitBackgroundChanged(bool fitBackground);
        void outlineEnabledChanged(bool outlineEnabled);
        void outlineColorChanged(QRgb outlineColor);
        void outlineWidthChanged(int outlineWidth);
        void shadowEnabledChanged(bool shadowEnabled);
        void shadowColorChanged(QRgb shadowColor);
        void shadowOffsetXChanged(int shadowOffsetX);
        void shadowOffsetYChanged(int shadowOffsetY);
        void shadowBlurChanged(int shadowBlur);

    public slots:
        void setMedia(const QString &media) override;
        void resetMedia() override;
        bool setState(AkElement::ElementState state) override;
        void setText(const QString &text);
        void setTextSource(const TextSource &textSource);
        void setTextFile(const QString &textFile);
        void setFontFamily(const QString &fontFamily);
        void setFontSize(int fontSize);
        void setBold(bool bold);
        void setItalic(bool italic);
        void setUnderline(bool underline);
        void setStrikeout(bool strikeout);
        void setLetterSpacing(qreal letterSpacing);
        void setLineSpacing(qreal lineSpacing);
        void setAntialiasing(bool antialiasing);
        void setHorizontalAlignment(const HorizontalAlignment &horizontalAlignment);
        void setVerticalAlignment(const VerticalAlignment &verticalAlignment);
        void setWordWrap(bool wordWrap);
        void setPadding(int padding);
        void setFrameWidth(int frameWidth);
        void setFrameHeight(int frameHeight);
        void setForegroundColor(QRgb foregroundColor);
        void setBackgroundColor(QRgb backgroundColor);
        void setBackgroundShape(const BackgroundShape &backgroundShape);
        void setBackgroundRadius(int backgroundRadius);
        void setBackgroundBorderEnabled(bool backgroundBorderEnabled);
        void setBackgroundBorderColor(QRgb backgroundBorderColor);
        void setBackgroundBorderWidth(int backgroundBorderWidth);
        void setBackgroundShadowEnabled(bool backgroundShadowEnabled);
        void setBackgroundShadowColor(QRgb backgroundShadowColor);
        void setBackgroundShadowOffsetX(int backgroundShadowOffsetX);
        void setBackgroundShadowOffsetY(int backgroundShadowOffsetY);
        void setBackgroundShadowBlur(int backgroundShadowBlur);
        void setTailStyle(const TailStyle &tailStyle);
        void setTailAngle(int tailAngle);
        void setTailLength(int tailLength);
        void setTailWidth(int tailWidth);
        void setFitBackground(bool fitBackground);
        void setOutlineEnabled(bool outlineEnabled);
        void setOutlineColor(QRgb outlineColor);
        void setOutlineWidth(int outlineWidth);
        void setShadowEnabled(bool shadowEnabled);
        void setShadowColor(QRgb shadowColor);
        void setShadowOffsetX(int shadowOffsetX);
        void setShadowOffsetY(int shadowOffsetY);
        void setShadowBlur(int shadowBlur);
        void resetText();
        void resetTextSource();
        void resetTextFile();
        void resetFontFamily();
        void resetFontSize();
        void resetBold();
        void resetItalic();
        void resetUnderline();
        void resetStrikeout();
        void resetLetterSpacing();
        void resetLineSpacing();
        void resetAntialiasing();
        void resetHorizontalAlignment();
        void resetVerticalAlignment();
        void resetWordWrap();
        void resetPadding();
        void resetFrameWidth();
        void resetFrameHeight();
        void resetForegroundColor();
        void resetBackgroundColor();
        void resetBackgroundShape();
        void resetBackgroundRadius();
        void resetBackgroundBorderEnabled();
        void resetBackgroundBorderColor();
        void resetBackgroundBorderWidth();
        void resetBackgroundShadowEnabled();
        void resetBackgroundShadowColor();
        void resetBackgroundShadowOffsetX();
        void resetBackgroundShadowOffsetY();
        void resetBackgroundShadowBlur();
        void resetTailStyle();
        void resetTailAngle();
        void resetTailLength();
        void resetTailWidth();
        void resetFitBackground();
        void resetOutlineEnabled();
        void resetOutlineColor();
        void resetOutlineWidth();
        void resetShadowEnabled();
        void resetShadowColor();
        void resetShadowOffsetX();
        void resetShadowOffsetY();
        void resetShadowBlur();
};

#endif // TEXTSRCELEMENT_H
