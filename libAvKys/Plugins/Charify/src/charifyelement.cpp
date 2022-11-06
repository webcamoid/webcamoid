/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
#include <QPainter>
#include <QQmlContext>
#include <QMutex>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "charifyelement.h"
#include "character.h"

using ColorModeToStr = QMap<CharifyElement::ColorMode, QString>;

inline ColorModeToStr initColorModeToStr()
{
    ColorModeToStr colorModeToStr {
        {CharifyElement::ColorModeNatural, "natural"},
        {CharifyElement::ColorModeFixed  , "fixed"  }
    };

    return colorModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(ColorModeToStr,
                          colorModeToStr,
                          (initColorModeToStr()))

using HintingPreferenceToStr = QMap<QFont::HintingPreference, QString>;

inline HintingPreferenceToStr initHintingPreferenceToStr()
{
    HintingPreferenceToStr hintingPreferenceToStr {
        {QFont::PreferDefaultHinting , "PreferDefaultHinting" },
        {QFont::PreferNoHinting      , "PreferNoHinting"      },
        {QFont::PreferVerticalHinting, "PreferVerticalHinting"},
        {QFont::PreferFullHinting    , "PreferFullHinting"    }
    };

    return hintingPreferenceToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(HintingPreferenceToStr,
                          hintingPreferenceToStr,
                          (initHintingPreferenceToStr()))

using StyleStrategyToStr = QMap<QFont::StyleStrategy, QString>;

inline StyleStrategyToStr initStyleStrategyToStr()
{
    StyleStrategyToStr styleStrategyToStr {
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

Q_GLOBAL_STATIC_WITH_ARGS(StyleStrategyToStr,
                          styleStrategyToStr,
                          (initStyleStrategyToStr()))

class CharifyElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        CharifyElement::ColorMode m_mode {CharifyElement::ColorModeNatural};
        QString m_charTable;
        QFont m_font {QApplication::font()};
        QRgb m_foregroundColor {qRgb(255, 255, 255)};
        QRgb m_backgroundColor {qRgb(0, 0, 0)};
        QVector<Character> m_characters;
        QVector<QRgb> m_grayToForeBackTable;
        QSize m_fontSize;
        QMutex m_mutex;
        bool m_reversed {false};

        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QImage drawChar(const QChar &chr,
                        const QFont &font,
                        const QSize &fontSize) const;
        int imageWeight(const QImage &image, bool reversed) const;
        QImage createMask(const AkVideoPacket &packet,
                          const QSize &fontSize,
                          const QVector<Character> &characters) const;
};

CharifyElement::CharifyElement(): AkElement()
{
    this->d = new CharifyElementPrivate;

    for (int i = 32; i < 127; i++)
        this->d->m_charTable.append(QChar(i));

    this->d->m_font.setHintingPreference(QFont::PreferFullHinting);
    this->d->m_font.setStyleStrategy(QFont::NoAntialias);

    this->updateCharTable();
    this->updateGrayToForeBackTable();

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
                     &CharifyElement::foregroundColorChanged,
                     this,
                     &CharifyElement::updateGrayToForeBackTable);
    QObject::connect(this,
                     &CharifyElement::backgroundColorChanged,
                     this,
                     &CharifyElement::updateCharTable);
    QObject::connect(this,
                     &CharifyElement::backgroundColorChanged,
                     this,
                     &CharifyElement::updateGrayToForeBackTable);
    QObject::connect(this,
                     &CharifyElement::reversedChanged,
                     this,
                     &CharifyElement::updateCharTable);
}

CharifyElement::~CharifyElement()
{
    delete this->d;
}

QString CharifyElement::mode() const
{
    return colorModeToStr->value(this->d->m_mode);
}

QString CharifyElement::charTable() const
{
    return this->d->m_charTable;
}

QFont CharifyElement::font() const
{
    return this->d->m_font;
}

QString CharifyElement::hintingPreference() const
{
    return hintingPreferenceToStr->value(this->d->m_font.hintingPreference(),
                                         "PreferFullHinting");
}

QString CharifyElement::styleStrategy() const
{
    return styleStrategyToStr->value(this->d->m_font.styleStrategy(),
                                     "NoAntialias");
}

QRgb CharifyElement::foregroundColor() const
{
    return this->d->m_foregroundColor;
}

QRgb CharifyElement::backgroundColor() const
{
    return this->d->m_backgroundColor;
}

bool CharifyElement::reversed() const
{
    return this->d->m_reversed;
}

QString CharifyElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Charify/share/qml/main.qml");
}

void CharifyElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Charify", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket CharifyElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    this->d->m_mutex.lock();
    auto fontSize = this->d->m_fontSize;

    int textWidth = src.caps().width() / fontSize.width();
    int textHeight = src.caps().height() / fontSize.height();

    int outWidth = textWidth * fontSize.width();
    int outHeight = textHeight * fontSize.height();

    auto ocaps = src.caps();
    ocaps.setWidth(outWidth);
    ocaps.setHeight(outHeight);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    if (this->d->m_characters.isEmpty()) {
        this->d->m_mutex.unlock();

        for (int y = 0; y < dst.caps().height(); y++) {
            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < dst.caps().width(); x++)
                line[x] = this->d->m_backgroundColor;
        }

        if (dst)
            emit this->oStream(dst);

        return dst;
    }

    auto mask = this->d->createMask(src, fontSize, this->d->m_characters);
    this->d->m_mutex.unlock();

    if (this->d->m_mode == ColorModeFixed) {
        this->d->m_mutex.lock();

        for (int y = 0; y < dst.caps().height(); y++) {
            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));
            auto maskLine = reinterpret_cast<const quint8 *>(mask.constScanLine(y));

            for (int x = 0; x < dst.caps().width(); x++)
                line[x] = this->d->m_grayToForeBackTable[maskLine[x]];
        }

        this->d->m_mutex.unlock();
    } else {
        auto br = qRed(this->d->m_backgroundColor);
        auto bg = qGreen(this->d->m_backgroundColor);
        auto bb = qBlue(this->d->m_backgroundColor);

        for (int y = 0; y < dst.caps().height(); y++) {
            int ys = y * (src.caps().height() - 1) / (dst.caps().height() - 1);
            auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));
            auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, ys));
            auto maskLine = reinterpret_cast<const quint8 *>(mask.constScanLine(y));

            for (int x = 0; x < dst.caps().width(); x++) {
                int xs = x * (src.caps().width() - 1);

                if (textWidth > 1)
                    xs /= dst.caps().width() - 1;

                auto pixel = srcLine[xs];
                auto alpha = maskLine[x];
                dstLine[x] = qRgb((alpha * qRed(pixel)   + (255 - alpha) * br) / 255,
                                  (alpha * qGreen(pixel) + (255 - alpha) * bg) / 255,
                                  (alpha * qBlue(pixel)  + (255 - alpha) * bb) / 255);
            }
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void CharifyElement::setMode(const QString &mode)
{
    ColorMode modeEnum = colorModeToStr->key(mode, ColorModeFixed);

    if (this->d->m_mode == modeEnum)
        return;

    this->d->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void CharifyElement::setCharTable(const QString &charTable)
{
    if (this->d->m_charTable == charTable)
        return;

    this->d->m_charTable = charTable;
    emit this->charTableChanged(charTable);
}

void CharifyElement::setFont(const QFont &font)
{
    if (this->d->m_font == font)
        return;

    auto hp = hintingPreferenceToStr->key(this->hintingPreference(),
                                          QFont::PreferFullHinting);
    auto ss = styleStrategyToStr->key(this->styleStrategy(),
                                      QFont::NoAntialias);

    this->d->m_font = font;
    this->d->m_font.setHintingPreference(hp);
    this->d->m_font.setStyleStrategy(ss);
    emit this->fontChanged(font);
}

void CharifyElement::setHintingPreference(const QString &hintingPreference)
{
    auto hp = hintingPreferenceToStr->key(hintingPreference,
                                          QFont::PreferFullHinting);

    if (this->d->m_font.hintingPreference() == hp)
        return;

    this->d->m_font.setHintingPreference(hp);
    emit hintingPreferenceChanged(hintingPreference);
}

void CharifyElement::setStyleStrategy(const QString &styleStrategy)
{
    auto ss = styleStrategyToStr->key(styleStrategy, QFont::NoAntialias);

    if (this->d->m_font.styleStrategy() == ss)
        return;

    this->d->m_font.setStyleStrategy(ss);
    emit styleStrategyChanged(styleStrategy);
}

void CharifyElement::setForegroundColor(QRgb foregroundColor)
{
    if (this->d->m_foregroundColor == foregroundColor)
        return;

    this->d->m_foregroundColor = foregroundColor;
    emit this->foregroundColorChanged(foregroundColor);
}

void CharifyElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    this->d->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void CharifyElement::setReversed(bool reversed)
{
    if (this->d->m_reversed == reversed)
        return;

    this->d->m_reversed = reversed;
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

void CharifyElement::updateCharTable()
{
    QList<Character> characters;
    auto fontSize = this->d->fontSize(this->d->m_charTable, this->d->m_font);

    QVector<QRgb> colorTable(256);

    for (int i = 0; i < 256; i++)
        colorTable[i] = qRgb(i, i, i);

    for (auto &chr: this->d->m_charTable) {
        auto image = this->d->drawChar(chr,
                                       this->d->m_font,
                                       fontSize);
        int weight = this->d->imageWeight(image, this->d->m_reversed);
        characters << Character(chr, image, weight);
    }

    QMutexLocker locker(&this->d->m_mutex);

    this->d->m_fontSize = fontSize;

    if (characters.isEmpty()) {
        this->d->m_characters.clear();

        return;
    }

    this->d->m_characters.resize(256);
    std::sort(characters.begin(),
              characters.end(),
              [] (const Character &chr1, const Character &chr2) {
                  return chr1.weight() < chr2.weight();
              });

    for (int i = 0; i < 256; i++) {
        int c = i * (characters.size() - 1) / 255;
        this->d->m_characters[i] = characters[c];
    }
}

void CharifyElement::updateGrayToForeBackTable()
{
    QMutexLocker locker(&this->d->m_mutex);

    auto fr = qRed(this->d->m_foregroundColor);
    auto fg = qGreen(this->d->m_foregroundColor);
    auto fb = qBlue(this->d->m_foregroundColor);

    auto br = qRed(this->d->m_backgroundColor);
    auto bg = qGreen(this->d->m_backgroundColor);
    auto bb = qBlue(this->d->m_backgroundColor);

    this->d->m_grayToForeBackTable.clear();

    for (int i = 0; i < 256; i++)
        this->d->m_grayToForeBackTable << qRgb((i * fr + (255 - i) * br) / 255,
                                               (i * fg + (255 - i) * bg) / 255,
                                               (i * fb + (255 - i) * bb) / 255);
}

QSize CharifyElementPrivate::fontSize(const QString &chrTable,
                                      const QFont &font) const
{
    QFontMetrics metrics(font);
    int width = -1;
    int height = -1;

    for (auto &chr: chrTable) {
        auto size = metrics.size(Qt::TextSingleLine, chr);

        if (size.width() > width)
            width = size.width();

        if (size.height() > height)
            height = size.height();
    }

    return {width, height};
}

QImage CharifyElementPrivate::drawChar(const QChar &chr,
                                       const QFont &font,
                                       const QSize &fontSize) const
{
    QImage fontImg(fontSize, QImage::Format_Grayscale8);
    fontImg.fill(qRgb(0, 0, 0));

    QPainter painter;
    painter.begin(&fontImg);
    painter.setPen(qRgb(255, 255, 255));
    painter.setFont(font);
    painter.drawText(fontImg.rect(), chr, Qt::AlignHCenter | Qt::AlignVCenter);
    painter.end();

    return fontImg;
}

int CharifyElementPrivate::imageWeight(const QImage &image, bool reversed) const
{
    int weight = 0;

    for (int y = 0; y < image.height(); y++) {
        auto imageLine = reinterpret_cast<const quint8 *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); x++)
            weight += imageLine[x];
    }

    weight /= image.width() * image.height();

    if (reversed)
        weight = 255 - weight;

    return weight;
}

QImage CharifyElementPrivate::createMask(const AkVideoPacket &packet,
                                         const QSize &fontSize,
                                         const QVector<Character> &characters) const
{
    int textWidth = packet.caps().width() / fontSize.width();
    int textHeight = packet.caps().height() / fontSize.height();

    int outWidth = textWidth * fontSize.width();
    int outHeight = textHeight * fontSize.height();

    QImage oFrame(outWidth, outHeight, QImage::Format_Grayscale8);

    QPainter painter;
    painter.begin(&oFrame);

    for (int y = 0; y < textHeight; y++) {
        int ys = y * (packet.caps().height() - 1) / (textHeight - 1);
        auto srcLine = reinterpret_cast<const QRgb *>(packet.constLine(0, ys));

        for (int x = 0; x < textWidth; x++) {
            int xs = x * (packet.caps().width() - 1);

            if (textWidth > 1)
                xs /= textWidth - 1;

            auto gray = qGray(srcLine[xs]);
            painter.drawImage(x * fontSize.width(), y * fontSize.height(), characters[gray].image());
        }
    }

    painter.end();

    return oFrame;
}

#include "moc_charifyelement.cpp"
