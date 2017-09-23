/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QApplication>

#include "charifyelement.h"

typedef QMap<CharifyElement::ColorMode, QString> ColorModeToStr;

inline ColorModeToStr initColorModeToStr()
{
    ColorModeToStr colorModeToStr;
    colorModeToStr[CharifyElement::ColorModeNatural] = "natural";
    colorModeToStr[CharifyElement::ColorModeFixed] = "fixed";

    return colorModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(ColorModeToStr, colorModeToStr, (initColorModeToStr()))

typedef QMap<QFont::HintingPreference, QString> HintingPreferenceToStr;

inline HintingPreferenceToStr initHintingPreferenceToStr()
{
    HintingPreferenceToStr hintingPreferenceToStr;
    hintingPreferenceToStr[QFont::PreferDefaultHinting] = "PreferDefaultHinting";
    hintingPreferenceToStr[QFont::PreferNoHinting] = "PreferNoHinting";
    hintingPreferenceToStr[QFont::PreferVerticalHinting] = "PreferVerticalHinting";
    hintingPreferenceToStr[QFont::PreferFullHinting] = "PreferFullHinting";

    return hintingPreferenceToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(HintingPreferenceToStr, hintingPreferenceToStr, (initHintingPreferenceToStr()))

typedef QMap<QFont::StyleStrategy, QString> StyleStrategyToStr;

inline StyleStrategyToStr initStyleStrategyToStr()
{
    StyleStrategyToStr styleStrategyToStr;
    styleStrategyToStr[QFont::PreferDefault] = "PreferDefault";
    styleStrategyToStr[QFont::PreferBitmap] = "PreferBitmap";
    styleStrategyToStr[QFont::PreferDevice] = "PreferDevice";
    styleStrategyToStr[QFont::PreferOutline] = "PreferOutline";
    styleStrategyToStr[QFont::ForceOutline] = "ForceOutline";
    styleStrategyToStr[QFont::PreferMatch] = "PreferMatch";
    styleStrategyToStr[QFont::PreferQuality] = "PreferQuality";
    styleStrategyToStr[QFont::PreferAntialias] = "PreferAntialias";
    styleStrategyToStr[QFont::NoAntialias] = "NoAntialias";
    styleStrategyToStr[QFont::OpenGLCompatible] = "OpenGLCompatible";
    styleStrategyToStr[QFont::ForceIntegerMetrics] = "ForceIntegerMetrics";
    styleStrategyToStr[QFont::NoSubpixelAntialias] = "NoSubpixelAntialias";
    styleStrategyToStr[QFont::NoFontMerging] = "NoFontMerging";

    return styleStrategyToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(StyleStrategyToStr, styleStrategyToStr, (initStyleStrategyToStr()))

CharifyElement::CharifyElement(): AkElement()
{
    this->m_mode = ColorModeNatural;

    for (int i = 32; i < 127; i++)
        this->m_charTable.append(QChar(i));

    this->m_font = QApplication::font();
    this->m_font.setHintingPreference(QFont::PreferFullHinting);
    this->m_font.setStyleStrategy(QFont::NoAntialias);
    this->m_foregroundColor = qRgb(255, 255, 255);
    this->m_backgroundColor = qRgb(0, 0, 0);
    this->m_reversed = false;

    this->updateCharTable();

    QObject::connect(this,
                     &CharifyElement::modeChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::charTableChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::fontChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::hintingPreferenceChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::styleStrategyChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::foregroundColorChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::backgroundColorChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::reversedChanged,
                     this,
                     &CharifyElement::updateCharTable);
}

QString CharifyElement::mode() const
{
    return colorModeToStr->value(this->m_mode);
}

QString CharifyElement::charTable() const
{
    return this->m_charTable;
}

QFont CharifyElement::font() const
{
    return this->m_font;
}

QString CharifyElement::hintingPreference() const
{
    return hintingPreferenceToStr->value(this->m_font.hintingPreference(), "PreferFullHinting");
}

QString CharifyElement::styleStrategy() const
{
    return styleStrategyToStr->value(this->m_font.styleStrategy(), "NoAntialias");
}

QRgb CharifyElement::foregroundColor() const
{
    return this->m_foregroundColor;
}

QRgb CharifyElement::backgroundColor() const
{
    return this->m_backgroundColor;
}

bool CharifyElement::reversed() const
{
    return this->m_reversed;
}

QSize CharifyElement::fontSize(const QString &chrTable, const QFont &font) const
{
    QFontMetrics metrics(font);
    int width = -1;
    int height = -1;

    for (const QChar &chr: chrTable) {
        QSize size = metrics.size(Qt::TextSingleLine, chr);

        if (size.width() > width)
            width = size.width();

        if (size.height() > height)
            height = size.height();
    }

    return QSize(width, height);
}

QImage CharifyElement::drawChar(const QChar &chr, const QFont &font,
                                const QSize &fontSize,
                                QRgb foreground, QRgb background) const
{
    QImage fontImg(fontSize, QImage::Format_RGB32);
    fontImg.fill(background);

    QPainter painter;

    painter.begin(&fontImg);
    painter.setPen(foreground);
    painter.setFont(font);
    painter.drawText(fontImg.rect(), chr, Qt::AlignHCenter | Qt::AlignVCenter);
    painter.end();

    return fontImg;
}

int CharifyElement::imageWeight(const QImage &image, bool reversed) const
{
    int weight = 0;

    for (int y = 0; y < image.height(); y++) {
        const QRgb *imageLine = reinterpret_cast<const QRgb *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); x++)
            weight += qGray(imageLine[x]);
    }

    weight /= image.width() * image.height();

    if (reversed)
        weight = 255 - weight;

    return weight;
}

bool CharifyElement::chrLessThan(const Character &chr1, const Character &chr2)
{
    return chr1.weight < chr2.weight;
}

QString CharifyElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Charify/share/qml/main.qml");
}

void CharifyElement::controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Charify", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void CharifyElement::setMode(const QString &mode)
{
    ColorMode modeEnum = colorModeToStr->key(mode, ColorModeNatural);

    if (this->m_mode == modeEnum)
        return;

    this->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void CharifyElement::setCharTable(const QString &charTable)
{
    if (this->m_charTable == charTable)
        return;

    this->m_charTable = charTable;
    emit this->charTableChanged(charTable);
}

void CharifyElement::setFont(const QFont &font)
{
    if (this->m_font == font)
        return;

    QFont::HintingPreference hp =
            hintingPreferenceToStr->key(this->hintingPreference(),
                                        QFont::PreferFullHinting);
    QFont::StyleStrategy ss =
            styleStrategyToStr->key(this->styleStrategy(),
                                    QFont::NoAntialias);

    this->m_font = font;
    this->m_font.setHintingPreference(hp);
    this->m_font.setStyleStrategy(ss);
    emit this->fontChanged(font);
}

void CharifyElement::setHintingPreference(const QString &hintingPreference)
{
    QFont::HintingPreference hp =
            hintingPreferenceToStr->key(hintingPreference,
                                        QFont::PreferFullHinting);

    if (this->m_font.hintingPreference() == hp)
        return;

    this->m_font.setHintingPreference(hp);
    emit hintingPreferenceChanged(hintingPreference);
}

void CharifyElement::setStyleStrategy(const QString &styleStrategy)
{
    QFont::StyleStrategy ss =
            styleStrategyToStr->key(styleStrategy,
                                    QFont::NoAntialias);

    if (this->m_font.styleStrategy() == ss)
        return;

    this->m_font.setStyleStrategy(ss);
    emit styleStrategyChanged(styleStrategy);
}

void CharifyElement::setForegroundColor(QRgb foregroundColor)
{
    if (this->m_foregroundColor == foregroundColor)
        return;

    this->m_foregroundColor = foregroundColor;
    emit this->foregroundColorChanged(foregroundColor);
}

void CharifyElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->m_backgroundColor == backgroundColor)
        return;

    this->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void CharifyElement::setReversed(bool reversed)
{
    if (this->m_reversed == reversed)
        return;

    this->m_reversed = reversed;
    emit this->reversedChanged(reversed);
}

void CharifyElement::resetMode()
{
    this->setMode("natural");
}

void CharifyElement::resetCharTable()
{
    QString charTable;

    for (int i = 32; i < 127; i++)
        charTable.append(QChar(i));

    this->setCharTable(charTable);
}

void CharifyElement::resetFont()
{
    this->setFont(QApplication::font());
}

void CharifyElement::resetHintingPreference()
{
    this->setHintingPreference("PreferFullHinting");
}

void CharifyElement::resetStyleStrategy()
{
    this->setStyleStrategy("NoAntialias");
}

void CharifyElement::resetForegroundColor()
{
    this->setForegroundColor(qRgb(255, 255, 255));
}

void CharifyElement::resetBackgroundColor()
{
    this->setBackgroundColor(qRgb(0, 0, 0));
}

void CharifyElement::resetReversed()
{
    this->setReversed(false);
}

AkPacket CharifyElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);

    this->m_mutex.lock();
    QSize fontSize = this->m_fontSize;
    QVector<Character> characters = this->m_characters;
    this->m_mutex.unlock();

    int textWidth = src.width() / fontSize.width();
    int textHeight = src.height() / fontSize.height();

    int outWidth = textWidth * fontSize.width();
    int outHeight = textHeight * fontSize.height();

    QImage oFrame(outWidth, outHeight, src.format());

    if (characters.isEmpty()) {
        oFrame.fill(qRgb(0, 0, 0));
        AkPacket oPacket = AkUtils::imageToPacket(oFrame.scaled(src.size()), packet);
        akSend(oPacket)
    }

    QImage textImage = src.scaled(textWidth, textHeight);
    const QRgb *textImageBits = reinterpret_cast<const QRgb *>(textImage.constBits());
    int textArea = textImage.width() * textImage.height();
    QPainter painter;

    painter.begin(&oFrame);

    for (int i = 0; i < textArea; i++) {
        int x = fontSize.width() * (i % textWidth);
        int y = fontSize.height() * (i / textWidth);

        if (this->m_mode == ColorModeFixed)
            painter.drawImage(x, y, characters[qGray(textImageBits[i])].image);
        else {
            QChar chr = characters[qGray(textImageBits[i])].chr;
            QRgb foreground = textImageBits[i];
            QImage image = this->drawChar(chr, this->m_font, fontSize, foreground, this->m_backgroundColor);
            painter.drawImage(x, y, image);
        }
    }

    painter.end();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

void CharifyElement::updateCharTable()
{
    QList<Character> characters;
    QSize fontSize = this->fontSize(this->m_charTable, this->m_font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    for (const QChar &chr: this->m_charTable) {
        QImage image = this->drawChar(chr,
                                      this->m_font,
                                      fontSize,
                                      this->m_foregroundColor,
                                      this->m_backgroundColor);
        int weight = this->imageWeight(image, this->m_reversed);

        if (this->m_mode == ColorModeFixed)
            characters.append(Character(chr, image, weight));
        else
            characters.append(Character(chr, QImage(), weight));
    }

    QMutexLocker(&this->m_mutex);

    this->m_fontSize = fontSize;

    if (characters.isEmpty()) {
        this->m_characters.clear();

        return;
    }

    this->m_characters.resize(256);
    std::sort(characters.begin(), characters.end(), this->chrLessThan);

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        this->m_characters[i] = characters[c];
    }
}
