/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
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

CharifyElement::CharifyElement(): AkElement()
{
    this->m_mode = ColorModeNatural;

    for (int i = 32; i < 127; i++)
        this->m_charTable.append(QChar(i));

    this->m_font = QApplication::font();
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

QObject *CharifyElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Charify/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Charify", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
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

    this->m_font = font;
    emit this->fontChanged(font);
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
    const QRgb *textImageBits = (const QRgb *) textImage.constBits();
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
            QImage image = drawChar(chr, this->m_font, fontSize, foreground, this->m_backgroundColor);
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

    foreach (QChar chr, this->m_charTable) {
        QImage image = drawChar(chr,
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
    qSort(characters.begin(), characters.end(), this->chrLessThan);

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        this->m_characters[i] = characters[c];
    }
}
