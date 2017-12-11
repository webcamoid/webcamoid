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
#include <QQmlContext>
#include <QPainter>
#include <QFontMetrics>
#include <QMutex>
#include <akutils.h>
#include <akpacket.h>

#include "matrixelement.h"
#include "character.h"
#include "raindrop.h"

typedef QMap<QFont::HintingPreference, QString> HintingPreferenceToStr;

inline HintingPreferenceToStr initHintingPreferenceToStr()
{
    static const HintingPreferenceToStr hintingPreferenceToStr = {
        {QFont::PreferDefaultHinting , "PreferDefaultHinting" },
        {QFont::PreferNoHinting      , "PreferNoHinting"      },
        {QFont::PreferVerticalHinting, "PreferVerticalHinting"},
        {QFont::PreferFullHinting    , "PreferFullHinting"    }
    };

    return hintingPreferenceToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(HintingPreferenceToStr, hintingPreferenceToStr, (initHintingPreferenceToStr()))

typedef QMap<QFont::StyleStrategy, QString> StyleStrategyToStr;

inline StyleStrategyToStr initStyleStrategyToStr()
{
    static const StyleStrategyToStr styleStrategyToStr = {
        {QFont::PreferDefault      , "PreferDefault"      },
        {QFont::PreferBitmap       , "PreferBitmap"       },
        {QFont::PreferDevice       , "PreferDevice"       },
        {QFont::PreferOutline      , "PreferOutline"      },
        {QFont::ForceOutline       , "ForceOutline"       },
        {QFont::PreferMatch        , "PreferMatch"        },
        {QFont::PreferQuality      , "PreferQuality"      },
        {QFont::PreferAntialias    , "PreferAntialias"    },
        {QFont::NoAntialias        , "NoAntialias"        },
        {QFont::OpenGLCompatible   , "OpenGLCompatible"   },
        {QFont::ForceIntegerMetrics, "ForceIntegerMetrics"},
        {QFont::NoSubpixelAntialias, "NoSubpixelAntialias"},
        {QFont::NoFontMerging      , "NoFontMerging"      }
    };

    return styleStrategyToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(StyleStrategyToStr, styleStrategyToStr, (initStyleStrategyToStr()))

class MatrixElementPrivate
{
    public:
        int m_nDrops;
        QString m_charTable;
        QFont m_font;
        QRgb m_cursorColor;
        QRgb m_foregroundColor;
        QRgb m_backgroundColor;
        int m_minDropLength;
        int m_maxDropLength;
        qreal m_minSpeed;
        qreal m_maxSpeed;
        bool m_showCursor;

        QList<Character> m_characters;
        QSize m_fontSize;
        QList<RainDrop> m_rain;
        QMutex m_mutex;

        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QImage drawChar(const QChar &chr, const QFont &font,
                        const QSize &fontSize,
                        QRgb foreground, QRgb background) const;
        int imageWeight(const QImage &image) const;
        static bool chrLessThan(const Character &chr1, const Character &chr2);
        QImage renderRain(const QSize &frameSize, const QImage &textImage);
};

MatrixElement::MatrixElement(): AkElement()
{
    this->d = new MatrixElementPrivate;
    this->d->m_nDrops = 25;

    for (int i = 32; i < 127; i++)
        this->d->m_charTable.append(QChar(i));

    this->d->m_font = QApplication::font();
    this->d->m_font.setHintingPreference(QFont::PreferFullHinting);
    this->d->m_font.setStyleStrategy(QFont::NoAntialias);
    this->d->m_cursorColor = qRgb(255, 255, 255);
    this->d->m_foregroundColor = qRgb(0, 255, 0);
    this->d->m_backgroundColor = qRgb(0, 0, 0);
    this->d->m_minDropLength = 3;
    this->d->m_maxDropLength = 20;
    this->d->m_minSpeed = 0.5;
    this->d->m_maxSpeed = 5.0;
    this->d->m_showCursor = false;

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
                     &MatrixElement::hintingPreferenceChanged,
                     this,
                     &MatrixElement::updateCharTable);
    QObject::connect(this,
                     &MatrixElement::styleStrategyChanged,
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

MatrixElement::~MatrixElement()
{
    delete this->d;
}

int MatrixElement::nDrops() const
{
    return this->d->m_nDrops;
}

QString MatrixElement::charTable() const
{
    return this->d->m_charTable;
}

QFont MatrixElement::font() const
{
    return this->d->m_font;
}

QString MatrixElement::hintingPreference() const
{
    return hintingPreferenceToStr->value(this->d->m_font.hintingPreference(),
                                         "PreferFullHinting");
}

QString MatrixElement::styleStrategy() const
{
    return styleStrategyToStr->value(this->d->m_font.styleStrategy(),
                                     "NoAntialias");
}

QRgb MatrixElement::cursorColor() const
{
    return this->d->m_cursorColor;
}

QRgb MatrixElement::foregroundColor() const
{
    return this->d->m_foregroundColor;
}

QRgb MatrixElement::backgroundColor() const
{
    return this->d->m_backgroundColor;
}

int MatrixElement::minDropLength() const
{
    return this->d->m_minDropLength;
}

int MatrixElement::maxDropLength() const
{
    return this->d->m_maxDropLength;
}

qreal MatrixElement::minSpeed() const
{
    return this->d->m_minSpeed;
}

qreal MatrixElement::maxSpeed() const
{
    return this->d->m_maxSpeed;
}

bool MatrixElement::showCursor() const
{
    return this->d->m_showCursor;
}

QSize MatrixElementPrivate::fontSize(const QString &chrTable,
                                     const QFont &font) const
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

QImage MatrixElementPrivate::drawChar(const QChar &chr, const QFont &font,
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

int MatrixElementPrivate::imageWeight(const QImage &image) const
{
    int weight = 0;

    for (int y = 0; y < image.height(); y++) {
        const QRgb *imageLine = reinterpret_cast<const QRgb *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); x++)
            weight += qGray(imageLine[x]);
    }

    weight /= image.width() * image.height();

    return weight;
}

bool MatrixElementPrivate::chrLessThan(const Character &chr1,
                                       const Character &chr2)
{
    return chr1.weight < chr2.weight;
}

QImage MatrixElementPrivate::renderRain(const QSize &frameSize,
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

QString MatrixElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Matrix/share/qml/main.qml");
}

void MatrixElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Matrix", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void MatrixElement::setNDrops(int nDrops)
{
    if (this->d->m_nDrops == nDrops)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_nDrops = nDrops;
    emit this->nDropsChanged(nDrops);
}

void MatrixElement::setCharTable(const QString &charTable)
{
    if (this->d->m_charTable == charTable)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_charTable = charTable;
    emit this->charTableChanged(charTable);
}

void MatrixElement::setFont(const QFont &font)
{
    if (this->d->m_font == font)
        return;

    QMutexLocker(&this->d->m_mutex);

    QFont::HintingPreference hp =
            hintingPreferenceToStr->key(this->hintingPreference(),
                                        QFont::PreferFullHinting);
    QFont::StyleStrategy ss =
            styleStrategyToStr->key(this->styleStrategy(),
                                    QFont::NoAntialias);

    this->d->m_font = font;
    this->d->m_font.setHintingPreference(hp);
    this->d->m_font.setStyleStrategy(ss);
    this->d->m_rain.clear();
    emit this->fontChanged(font);
}

void MatrixElement::setHintingPreference(const QString &hintingPreference)
{
    QFont::HintingPreference hp =
            hintingPreferenceToStr->key(hintingPreference,
                                        QFont::PreferFullHinting);

    if (this->d->m_font.hintingPreference() == hp)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_font.setHintingPreference(hp);
    this->d->m_rain.clear();
    emit hintingPreferenceChanged(hintingPreference);
}

void MatrixElement::setStyleStrategy(const QString &styleStrategy)
{
    QFont::StyleStrategy ss =
            styleStrategyToStr->key(styleStrategy,
                                    QFont::NoAntialias);

    if (this->d->m_font.styleStrategy() == ss)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_font.setStyleStrategy(ss);
    this->d->m_rain.clear();
    emit styleStrategyChanged(styleStrategy);
}

void MatrixElement::setCursorColor(QRgb cursorColor)
{
    if (this->d->m_cursorColor == cursorColor)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_cursorColor = cursorColor;
    emit this->cursorColorChanged(cursorColor);
}

void MatrixElement::setForegroundColor(QRgb foregroundColor)
{
    if (this->d->m_foregroundColor == foregroundColor)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_foregroundColor = foregroundColor;
    emit this->foregroundColorChanged(foregroundColor);
}

void MatrixElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void MatrixElement::setMinDropLength(int minDropLength)
{
    if (this->d->m_minDropLength == minDropLength)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_minDropLength = minDropLength;
    emit this->minDropLengthChanged(minDropLength);
}

void MatrixElement::setMaxDropLength(int maxDropLength)
{
    if (this->d->m_maxDropLength == maxDropLength)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_maxDropLength = maxDropLength;
    emit this->maxDropLengthChanged(maxDropLength);
}

void MatrixElement::setMinSpeed(qreal minSpeed)
{
    if (qFuzzyCompare(this->d->m_minSpeed, minSpeed))
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_minSpeed = minSpeed;
    emit this->minSpeedChanged(minSpeed);
}

void MatrixElement::setMaxSpeed(qreal maxSpeed)
{
    if (qFuzzyCompare(this->d->m_maxSpeed, maxSpeed))
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_maxSpeed = maxSpeed;
    emit this->maxSpeedChanged(maxSpeed);
}

void MatrixElement::setShowCursor(bool showCursor)
{
    if (this->d->m_showCursor == showCursor)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_showCursor = showCursor;
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

void MatrixElement::resetHintingPreference()
{
    this->setHintingPreference("PreferFullHinting");
}

void MatrixElement::resetStyleStrategy()
{
    this->setStyleStrategy("NoAntialias");
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

    this->d->m_mutex.lock();
    int textWidth = src.width() / this->d->m_fontSize.width();
    int textHeight = src.height() / this->d->m_fontSize.height();

    int outWidth = textWidth * this->d->m_fontSize.width();
    int outHeight = textHeight * this->d->m_fontSize.height();

    QImage oFrame(outWidth, outHeight, src.format());

    QList<Character> characters(this->d->m_characters);
    this->d->m_mutex.unlock();

    if (characters.size() < 256) {
        oFrame.fill(this->d->m_backgroundColor);
        AkPacket oPacket = AkUtils::imageToPacket(oFrame.scaled(src.size()),
                                                  packet);
        akSend(oPacket)
    }

    QImage textImage = src.scaled(textWidth, textHeight);
    QRgb *textImageBits = reinterpret_cast<QRgb *>(textImage.bits());
    int textArea = textImage.width() * textImage.height();
    QPainter painter;

    painter.begin(&oFrame);

    for (int i = 0; i < textArea; i++) {
        int x = this->d->m_fontSize.width() * (i % textWidth);
        int y = this->d->m_fontSize.height() * (i / textWidth);

        Character chr = characters[qGray(textImageBits[i])];
        painter.drawImage(x, y, chr.image);
        textImageBits[i] = chr.foreground;
    }

    painter.drawImage(0, 0, this->d->renderRain(oFrame.size(), textImage));
    painter.end();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

void MatrixElement::updateCharTable()
{
    QMutexLocker(&this->d->m_mutex);
    QList<Character> characters;
    this->d->m_fontSize =
            this->d->fontSize(this->d->m_charTable, this->d->m_font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    for (const QChar &chr: this->d->m_charTable) {
        QImage image =
                this->d->drawChar(chr,
                                  this->d->m_font,
                                  this->d->m_fontSize,
                                  this->d->m_foregroundColor,
                                  this->d->m_backgroundColor);
        int weight = this->d->imageWeight(image);

        characters.append(Character(chr, QImage(), weight));
    }

    std::sort(characters.begin(), characters.end(), this->d->chrLessThan);

    this->d->m_characters.clear();

    if (characters.isEmpty())
        return;

    QVector<QRgb> pallete;

    int r0 = qRed(this->d->m_backgroundColor);
    int g0 = qGreen(this->d->m_backgroundColor);
    int b0 = qBlue(this->d->m_backgroundColor);

    int rDiff = qRed(this->d->m_foregroundColor) - r0;
    int gDiff = qGreen(this->d->m_foregroundColor) - g0;
    int bDiff = qBlue(this->d->m_foregroundColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    r0 = qRed(this->d->m_foregroundColor);
    g0 = qGreen(this->d->m_foregroundColor);
    b0 = qBlue(this->d->m_foregroundColor);

    rDiff = qRed(this->d->m_cursorColor) - r0;
    gDiff = qGreen(this->d->m_cursorColor) - g0;
    bDiff = qBlue(this->d->m_cursorColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff) / 127 + r0;
        int g = (i * gDiff) / 127 + g0;
        int b = (i * bDiff) / 127 + b0;

        pallete << qRgb(r, g, b);
    }

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        characters[c].image =
                this->d->drawChar(characters[c].chr,
                                  this->d->m_font,
                                  this->d->m_fontSize,
                                  pallete[i],
                                  this->d->m_backgroundColor);
        characters[c].foreground = pallete[i];
        characters[c].background = this->d->m_backgroundColor;
        this->d->m_characters.append(characters[c]);
    }
}

#include "moc_matrixelement.cpp"
