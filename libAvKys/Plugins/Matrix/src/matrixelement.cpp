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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QApplication>

#include "matrixelement.h"

MatrixElement::MatrixElement(): AkElement()
{
    this->m_nDrops = 25;

    for (int i = 32; i < 127; i++)
        this->m_charTable.append(QChar(i));

    this->m_font = QApplication::font();
    this->m_cursorColor = qRgb(255, 255, 255);
    this->m_foregroundColor = qRgb(0, 255, 0);
    this->m_backgroundColor = qRgb(0, 0, 0);
    this->m_minDropLength = 3;
    this->m_maxDropLength = 20;
    this->m_minSpeed = 0.5;
    this->m_maxSpeed = 5.0;
    this->m_showCursor = false;

    this->updateCharTable();

    QObject::connect(this,
                     &MatrixElement::charTableChanged,
                     this,
                     &MatrixElement::updateCharTable);
    QObject::connect(this,
                     &MatrixElement::fontChanged,
                     this,
                     &MatrixElement::updateCharTable);
    QObject::connect(this,
                     &MatrixElement::cursorColorChanged,
                     this,
                     &MatrixElement::updateCharTable);
    QObject::connect(this,
                     &MatrixElement::foregroundColorChanged,
                     this,
                     &MatrixElement::updateCharTable);
    QObject::connect(this,
                     &MatrixElement::backgroundColorChanged,
                     this,
                     &MatrixElement::updateCharTable);
}

QObject *MatrixElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Matrix/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Matrix", (QObject *) this);
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

QImage MatrixElement::renderRain(const QSize &frameSize,
                                 const QImage &textImage)
{
    this->m_mutex.lock();
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
    this->m_mutex.unlock();

    return rain;
}

void MatrixElement::setNDrops(int nDrops)
{
    if (this->m_nDrops == nDrops)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_nDrops = nDrops;
    emit this->nDropsChanged(nDrops);
}

void MatrixElement::setCharTable(const QString &charTable)
{
    if (this->m_charTable == charTable)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_charTable = charTable;
    emit this->charTableChanged(charTable);
}

void MatrixElement::setFont(const QFont &font)
{
    if (this->m_font == font)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_font = font;
    this->m_rain.clear();
    emit this->fontChanged(font);
}

void MatrixElement::setCursorColor(QRgb cursorColor)
{
    if (this->m_cursorColor == cursorColor)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_cursorColor = cursorColor;
    emit this->cursorColorChanged(cursorColor);
}

void MatrixElement::setForegroundColor(QRgb foregroundColor)
{
    if (this->m_foregroundColor == foregroundColor)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_foregroundColor = foregroundColor;
    emit this->foregroundColorChanged(foregroundColor);
}

void MatrixElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->m_backgroundColor == backgroundColor)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void MatrixElement::setMinDropLength(int minDropLength)
{
    if (this->m_minDropLength == minDropLength)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_minDropLength = minDropLength;
    emit this->minDropLengthChanged(minDropLength);
}

void MatrixElement::setMaxDropLength(int maxDropLength)
{
    if (this->m_maxDropLength == maxDropLength)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_maxDropLength = maxDropLength;
    emit this->maxDropLengthChanged(maxDropLength);
}

void MatrixElement::setMinSpeed(qreal minSpeed)
{
    if (this->m_minSpeed == minSpeed)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_minSpeed = minSpeed;
    emit this->minSpeedChanged(minSpeed);
}

void MatrixElement::setMaxSpeed(qreal maxSpeed)
{
    if (this->m_maxSpeed == maxSpeed)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_maxSpeed = maxSpeed;
    emit this->maxSpeedChanged(maxSpeed);
}

void MatrixElement::setShowCursor(bool showCursor)
{
    if (this->m_showCursor == showCursor)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_showCursor = showCursor;
    emit this->showCursorChanged(showCursor);
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

AkPacket MatrixElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_RGB32);

    this->m_mutex.lock();
    int textWidth = src.width() / this->m_fontSize.width();
    int textHeight = src.height() / this->m_fontSize.height();

    int outWidth = textWidth * this->m_fontSize.width();
    int outHeight = textHeight * this->m_fontSize.height();

    QImage oFrame(outWidth, outHeight, src.format());

    QList<Character> characters(this->m_characters);
    this->m_mutex.unlock();

    if (characters.size() < 256) {
        oFrame.fill(this->m_backgroundColor);
        AkPacket oPacket = AkUtils::imageToPacket(oFrame.scaled(src.size()),
                                                  packet);
        akSend(oPacket)
    }

    QImage textImage = src.scaled(textWidth, textHeight);
    QRgb *textImageBits = (QRgb *) textImage.bits();
    int textArea = textImage.width() * textImage.height();
    QPainter painter;

    painter.begin(&oFrame);

    for (int i = 0; i < textArea; i++) {
        int x = this->m_fontSize.width() * (i % textWidth);
        int y = this->m_fontSize.height() * (i / textWidth);

        Character chr = characters[qGray(textImageBits[i])];
        painter.drawImage(x, y, chr.image);
        textImageBits[i] = chr.foreground;
    }

    painter.drawImage(0, 0, this->renderRain(oFrame.size(), textImage));
    painter.end();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

void MatrixElement::updateCharTable()
{
    QMutexLocker(&this->m_mutex);
    QList<Character> characters;
    this->m_fontSize = this->fontSize(this->m_charTable, this->m_font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    foreach (QChar chr, this->m_charTable) {
        QImage image = drawChar(chr,
                                this->m_font,
                                this->m_fontSize,
                                this->m_foregroundColor, this->m_backgroundColor);
        int weight = this->imageWeight(image);

        characters.append(Character(chr, QImage(), weight));
    }

    qSort(characters.begin(), characters.end(), this->chrLessThan);

    this->m_characters.clear();

    if (characters.isEmpty())
        return;

    QVector<QRgb> pallete;

    int r0 = qRed(this->m_backgroundColor);
    int g0 = qGreen(this->m_backgroundColor);
    int b0 = qBlue(this->m_backgroundColor);

    int rDiff = qRed(this->m_foregroundColor) - r0;
    int gDiff = qGreen(this->m_foregroundColor) - g0;
    int bDiff = qBlue(this->m_foregroundColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    r0 = qRed(this->m_foregroundColor);
    g0 = qGreen(this->m_foregroundColor);
    b0 = qBlue(this->m_foregroundColor);

    rDiff = qRed(this->m_cursorColor) - r0;
    gDiff = qGreen(this->m_cursorColor) - g0;
    bDiff = qBlue(this->m_cursorColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        characters[c].image = drawChar(characters[c].chr,
                                       this->m_font,
                                       this->m_fontSize,
                                       pallete[i],
                                       this->m_backgroundColor);
        characters[c].foreground = pallete[i];
        characters[c].background = this->m_backgroundColor;
        this->m_characters.append(characters[c]);
    }
}
