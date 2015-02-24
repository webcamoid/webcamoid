/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QApplication>

#include "charifyelement.h"

CharifyElement::CharifyElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->m_colorModeToStr[ColorModeNatural] = "natural";
    this->m_colorModeToStr[ColorModeFixed] = "fixed";

    this->m_mode = ColorModeNatural;

    for (int i = 32; i < 127; i++)
        this->m_charTable.append(QChar(i));

    this->m_font = QApplication::font();
    this->m_foregroundColor = qRgb(255, 255, 255);
    this->m_backgroundColor = qRgb(0, 0, 0);
    this->m_reversed = false;
}

QObject *CharifyElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Charify/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Charify", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QString CharifyElement::mode() const
{
    return this->m_colorModeToStr[this->m_mode];
}

QString CharifyElement::charTable() const
{
    return this->m_charTable;
}

QFont CharifyElement::font() const
{
    return this->m_font;
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

    foreach (QChar chr, chrTable) {
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
    int fontArea = image.width() * image.height();
    const QRgb *imageBits = (const QRgb *) image.constBits();
    int weight = 0;

    for (int i = 0; i < fontArea; i++)
        weight += qGray(imageBits[i]);

    weight /= fontArea;

    if (reversed)
        weight = 255 - weight;

    return weight;
}

bool CharifyElement::chrLessThan(const Character &chr1, const Character &chr2)
{
    return chr1.weight < chr2.weight;
}

void CharifyElement::createCharTable(ColorMode mode, const QString &charTable,
                                     const QFont &font,
                                     QRgb foreground, QRgb background,
                                     bool reversed)
{
    QList<Character> characters;
    this->m_fontSize = this->fontSize(charTable, font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    foreach (QChar chr, charTable) {
        QImage image = drawChar(chr, font, this->m_fontSize, foreground, background);
        int weight = this->imageWeight(image, reversed);

        if (mode == ColorModeFixed)
            characters.append(Character(chr, image, weight));
        else
            characters.append(Character(chr, QImage(), weight));
    }

    qSort(characters.begin(), characters.end(), this->chrLessThan);

    this->m_characters.clear();

    if (characters.isEmpty())
        return;

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        this->m_characters.append(characters[c]);
    }
}

void CharifyElement::setMode(const QString &mode)
{
    ColorMode modeEnum = this->m_colorModeToStr.values().contains(mode)?
                             this->m_colorModeToStr.key(mode):
                             ColorModeNatural;

    if (modeEnum != this->m_mode) {
        this->m_mode = modeEnum;
        emit this->modeChanged();
    }
}

void CharifyElement::setCharTable(const QString &charTable)
{
    if (charTable != this->m_charTable) {
        this->m_charTable = charTable;
        emit this->charTableChanged();
    }
}

void CharifyElement::setFont(const QFont &font)
{
    if (font != this->m_font) {
        this->m_font = font;
        emit this->fontChanged();
    }
}

void CharifyElement::setForegroundColor(QRgb foregroundColor)
{
    if (foregroundColor != this->m_foregroundColor) {
        this->m_foregroundColor = foregroundColor;
        emit this->foregroundColorChanged();
    }
}

void CharifyElement::setBackgroundColor(QRgb backgroundColor)
{
    if (backgroundColor != this->m_backgroundColor) {
        this->m_backgroundColor = backgroundColor;
        emit this->backgroundColorChanged();
    }
}

void CharifyElement::setReversed(bool reversed)
{
    if (reversed != this->m_reversed) {
        this->m_reversed = reversed;
        emit this->reversedChanged();
    }
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

QbPacket CharifyElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    static ColorMode mode;
    static QString charTable;
    static QFont font;
    static QRgb foregroundColor = -1;
    static QRgb backgroundColor = -1;
    static bool reversed = false;

    if (packet.caps() != this->m_caps
        || this->m_mode != mode
        || this->m_charTable != charTable
        || this->m_font!= font
        || this->m_foregroundColor!= foregroundColor
        || this->m_backgroundColor!= backgroundColor
        || this->m_reversed != reversed) {
        this->createCharTable(this->m_mode,
                              this->m_charTable,
                              this->m_font,
                              this->m_foregroundColor,
                              this->m_backgroundColor,
                              this->m_reversed);

        this->m_caps = packet.caps();
        mode = this->m_mode;
        charTable = this->m_charTable;
        font = this->m_font;
        foregroundColor = this->m_foregroundColor;
        backgroundColor = this->m_backgroundColor;
        reversed = this->m_reversed;
    }

    int textWidth = src.width() / this->m_fontSize.width();
    int textHeight = src.height() / this->m_fontSize.height();

    int outWidth = textWidth * this->m_fontSize.width();
    int outHeight = textHeight * this->m_fontSize.height();

    QImage oFrame(outWidth, outHeight, QImage::Format_RGB32);

    if (this->m_characters.isEmpty()) {
        oFrame.fill(qRgb(0, 0, 0));
        QbPacket oPacket = QbUtils::imageToPacket(oFrame.scaled(src.size()), iPacket);
        qbSend(oPacket)
    }

    QImage textImage = src.scaled(textWidth, textHeight).convertToFormat(QImage::Format_RGB32);
    const QRgb *textImageBits = (const QRgb *) textImage.constBits();
    int textArea = textImage.width() * textImage.height();
    QPainter painter;

    painter.begin(&oFrame);

    for (int i = 0; i < textArea; i++) {
        int x = this->m_fontSize.width() * (i % textWidth);
        int y = this->m_fontSize.height() * (i / textWidth);

        if (this->m_mode == ColorModeFixed)
            painter.drawImage(x, y, this->m_characters[qGray(textImageBits[i])].image);
        else {
            QChar chr = this->m_characters[qGray(textImageBits[i])].chr;
            QRgb foreground = textImageBits[i];
            QImage image = drawChar(chr, this->m_font, this->m_fontSize, foreground, this->m_backgroundColor);
            painter.drawImage(x, y, image);
        }
    }

    painter.end();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
