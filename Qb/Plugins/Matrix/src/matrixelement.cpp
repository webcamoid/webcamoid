/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "matrixelement.h"

MatrixElement::MatrixElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetNDrops();
    this->resetCharTable();
    this->resetFont();
    this->resetCursorColor();
    this->resetForegroundColor();
    this->resetBackgroundColor();
    this->resetMinDropLength();
    this->resetMaxDropLength();
    this->resetMinSpeed();
    this->resetMaxSpeed();
    this->resetShowCursor();
}

QObject *MatrixElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Matrix/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Matrix", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int MatrixElement::nDrops() const
{
    return this->m_nDrops;
}

QString MatrixElement::charTable() const
{
    return this->m_charTable;
}

QFont MatrixElement::font() const
{
    return this->m_font;
}

QRgb MatrixElement::cursorColor() const
{
    return this->m_cursorColor;
}

QRgb MatrixElement::foregroundColor() const
{
    return this->m_foregroundColor;
}

QRgb MatrixElement::backgroundColor() const
{
    return this->m_backgroundColor;
}

int MatrixElement::minDropLength() const
{
    return this->m_minDropLength;
}

int MatrixElement::maxDropLength() const
{
    return this->m_maxDropLength;
}

qreal MatrixElement::minSpeed() const
{
    return this->m_minSpeed;
}

qreal MatrixElement::maxSpeed() const
{
    return this->m_maxSpeed;
}

bool MatrixElement::showCursor() const
{
    return this->m_showCursor;
}

QSize MatrixElement::fontSize(const QString &chrTable, const QFont &font) const
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

QImage MatrixElement::drawChar(const QChar &chr, const QFont &font,
                               const QSize &fontSize, QRgb foreground,
                               QRgb background) const
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

int MatrixElement::imageWeight(const QImage &image) const
{
    int fontArea = image.width() * image.height();
    const QRgb *imageBits = (const QRgb *) image.constBits();
    int weight = 0;

    for (int i = 0; i < fontArea; i++)
        weight += qGray(imageBits[i]);

    weight /= fontArea;

    return weight;
}

bool MatrixElement::chrLessThan(const Character &chr1, const Character &chr2)
{
    return chr1.weight < chr2.weight;
}

void MatrixElement::createCharTable(const QString &charTable, const QFont &font,
                                    QRgb cursor,
                                    QRgb foreground,
                                    QRgb background)
{
    QList<Character> characters;
    this->m_fontSize = this->fontSize(charTable, font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    foreach (QChar chr, charTable) {
        QImage image = drawChar(chr, font, this->m_fontSize, foreground, background);
        int weight = this->imageWeight(image);

        characters.append(Character(chr, QImage(), weight));
    }

    qSort(characters.begin(), characters.end(), this->chrLessThan);

    this->m_characters.clear();

    if (characters.isEmpty())
        return;

    QVector<QRgb> pallete;

    int r0 = qRed(background);
    int g0 = qGreen(background);
    int b0 = qBlue(background);

    int rDiff = qRed(foreground) - r0;
    int gDiff = qGreen(foreground) - g0;
    int bDiff = qBlue(foreground) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    r0 = qRed(foreground);
    g0 = qGreen(foreground);
    b0 = qBlue(foreground);

    rDiff = qRed(cursor) - r0;
    gDiff = qGreen(cursor) - g0;
    bDiff = qBlue(cursor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        characters[c].image = drawChar(characters[c].chr, font, this->m_fontSize, pallete[i], background);
        characters[c].foreground = pallete[i];
        characters[c].background = background;
        this->m_characters.append(characters[c]);
    }
}

QImage MatrixElement::renderRain(const QSize &frameSize,
                                 const QImage &textImage)
{
    QImage rain(frameSize, QImage::Format_ARGB32);
    rain.fill(qRgba(0, 0, 0, 0));
    QPainter painter;

    bool randomStart = this->m_rain.isEmpty();

    while (this->m_rain.size() < this->m_nDrops)
        this->m_rain << RainDrop(textImage.size(),
                                 this->m_charTable,
                                 this->m_font,
                                 this->m_fontSize,
                                 this->m_cursorColor,
                                 this->m_foregroundColor,
                                 this->m_backgroundColor,
                                 this->m_minDropLength,
                                 this->m_maxDropLength,
                                 this->m_minSpeed,
                                 this->m_maxSpeed,
                                 randomStart);

    painter.begin(&rain);

    for (int i = 0; i < this->m_rain.size(); i++) {
        QPoint tail = this->m_rain[i].tail();
        QRgb tailColor;

        if (textImage.rect().contains(tail))
            tailColor = textImage.pixel(tail);
        else
            tailColor = this->m_backgroundColor;

        QImage sprite = this->m_rain[i].render(tailColor, this->m_showCursor);

        if (!sprite.isNull())
            painter.drawImage(this->m_rain[i].pos(), sprite);

        this->m_rain[i]++;

        if (!this->m_rain[i].isVisible()) {
            this->m_rain.removeAt(i);
            i--;
        }
    }

    painter.end();

    return rain;
}

void MatrixElement::setNDrops(int nDrops)
{
    if (nDrops != this->m_nDrops) {
        this->m_nDrops = nDrops;
        emit this->nDropsChanged();
    }
}

void MatrixElement::setCharTable(const QString &charTable)
{
    if (charTable != this->m_charTable) {
        this->m_charTable = charTable;
        emit this->charTableChanged();
    }
}

void MatrixElement::setFont(const QFont &font)
{
    if (font != this->m_font) {
        this->m_font = font;
        emit this->fontChanged();
    }
}

void MatrixElement::setCursorColor(QRgb cursorColor)
{
    if (cursorColor != this->m_cursorColor) {
        this->m_cursorColor = cursorColor;
        emit this->cursorColorChanged();
    }
}

void MatrixElement::setForegroundColor(QRgb foregroundColor)
{
    if (foregroundColor != this->m_foregroundColor) {
        this->m_foregroundColor = foregroundColor;
        emit this->foregroundColorChanged();
    }
}

void MatrixElement::setBackgroundColor(QRgb backgroundColor)
{
    if (backgroundColor != this->m_backgroundColor) {
        this->m_backgroundColor = backgroundColor;
        emit this->backgroundColorChanged();
    }
}

void MatrixElement::setMinDropLength(int minDropLength)
{
    if (minDropLength != this->m_minDropLength) {
        this->m_minDropLength = minDropLength;
        emit this->minDropLengthChanged();
    }
}

void MatrixElement::setMaxDropLength(int maxDropLength)
{
    if (maxDropLength != this->m_maxDropLength) {
        this->m_maxDropLength = maxDropLength;
        emit this->maxDropLengthChanged();
    }
}

void MatrixElement::setMinSpeed(qreal minSpeed)
{
    if (minSpeed != this->m_minSpeed) {
        this->m_minSpeed = minSpeed;
        emit this->minSpeedChanged();
    }
}

void MatrixElement::setMaxSpeed(qreal maxSpeed)
{
    if (maxSpeed != this->m_maxSpeed) {
        this->m_maxSpeed = maxSpeed;
        emit this->maxSpeedChanged();
    }
}

void MatrixElement::setShowCursor(bool showCursor)
{
    if (showCursor != this->m_showCursor) {
        this->m_showCursor = showCursor;
        emit this->showCursorChanged();
    }
}

void MatrixElement::resetNDrops()
{
    this->setNDrops(25);
}

void MatrixElement::resetCharTable()
{
    QString charTable;

    for (int i = 32; i < 127; i++)
        charTable.append(QChar(i));

    this->setCharTable(charTable);
}

void MatrixElement::resetFont()
{
    this->setFont(QApplication::font());
}

void MatrixElement::resetCursorColor()
{
    this->setCursorColor(qRgb(255, 255, 255));
}

void MatrixElement::resetForegroundColor()
{
    this->setForegroundColor(qRgb(0, 255, 0));
}

void MatrixElement::resetBackgroundColor()
{
    this->setBackgroundColor(qRgb(0, 0, 0));
}

void MatrixElement::resetMinDropLength()
{
    this->setMinDropLength(3);
}

void MatrixElement::resetMaxDropLength()
{
    this->setMaxDropLength(20);
}

void MatrixElement::resetMinSpeed()
{
    this->setMinSpeed(0.5);
}

void MatrixElement::resetMaxSpeed()
{
    this->setMaxSpeed(5.0);
}

void MatrixElement::resetShowCursor()
{
    this->setShowCursor(false);
}

QbPacket MatrixElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    static QString charTable;
    static QFont font;
    static QRgb cursorColor = -1;
    static QRgb foregroundColor = -1;
    static QRgb backgroundColor = -1;

    if (this->m_font!= font)
        this->m_rain.clear();

    if (packet.caps() != this->m_caps
        || this->m_charTable != charTable
        || this->m_font!= font
        || this->m_cursorColor != cursorColor
        || this->m_foregroundColor!= foregroundColor
        || this->m_backgroundColor!= backgroundColor) {
        this->createCharTable(this->m_charTable,
                              this->m_font,
                              this->m_cursorColor,
                              this->m_foregroundColor,
                              this->m_backgroundColor);

        this->m_caps = packet.caps();
        charTable = this->m_charTable;
        font = this->m_font;
        cursorColor = this->m_cursorColor;
        foregroundColor = this->m_foregroundColor;
        backgroundColor = this->m_backgroundColor;
    }

    int textWidth = src.width() / this->m_fontSize.width();
    int textHeight = src.height() / this->m_fontSize.height();

    int outWidth = textWidth * this->m_fontSize.width();
    int outHeight = textHeight * this->m_fontSize.height();

    QImage oFrame(outWidth, outHeight, QImage::Format_RGB32);

    if (this->m_characters.isEmpty()) {
        oFrame.fill(this->m_backgroundColor);
        QbPacket oPacket = QbUtils::imageToPacket(oFrame.scaled(src.size()), iPacket);
        qbSend(oPacket)
    }

    QImage textImage = src.scaled(textWidth, textHeight).convertToFormat(QImage::Format_RGB32);
    QRgb *textImageBits = (QRgb *) textImage.bits();
    int textArea = textImage.width() * textImage.height();
    QPainter painter;

    painter.begin(&oFrame);

    for (int i = 0; i < textArea; i++) {
        int x = this->m_fontSize.width() * (i % textWidth);
        int y = this->m_fontSize.height() * (i / textWidth);

        Character chr = this->m_characters[qGray(textImageBits[i])];
        painter.drawImage(x, y, chr.image);
        textImageBits[i] = chr.foreground;
    }

    painter.drawImage(0, 0, this->renderRain(oFrame.size(), textImage));

    painter.end();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
