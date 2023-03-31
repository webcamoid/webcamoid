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
#include <QDataStream>
#include <QFontMetrics>
#include <QMutex>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "charifyelement.h"
#include "character.h"

using HintingPreferenceToStr = QMap<QFont::HintingPreference, QString>;

inline HintingPreferenceToStr initHintingPreferenceToStr()
{
    static const HintingPreferenceToStr hintingPreferenceToStr {
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
    static const StyleStrategyToStr styleStrategyToStr {
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
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;
        CharifyElement::ColorMode m_mode {CharifyElement::ColorModeNatural};
        QString m_charTable;
        QFont m_font {QApplication::font()};
        QRgb m_foregroundColor {qRgb(255, 255, 255)};
        QRgb m_backgroundColor {qRgb(0, 0, 0)};
        Character *m_characters {nullptr};
        QRgb m_palette[256];
        int m_colorTable[256];
        QSize m_fontSize;
        QMutex m_mutex;
        bool m_smooth {true};
        bool m_reversed {false};

        void updateCharTable();
        void updatePalette();
        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QSize fontSize(const QChar &chr, const QFont &font) const;
        AkVideoPacket createMask(const AkVideoPacket &src,
                                 const QSize &fontSize,
                                 const Character *characters);
};

CharifyElement::CharifyElement(): AkElement()
{
    this->d = new CharifyElementPrivate;
    this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache);

    for (int i = 32; i < 127; i++)
        this->d->m_charTable.append(QChar(i));

    this->d->m_font.setHintingPreference(QFont::PreferFullHinting);
    this->d->m_font.setStyleStrategy(QFont::NoAntialias);
    this->d->updateCharTable();
    this->d->updatePalette();
}

CharifyElement::~CharifyElement()
{
    if (this->d->m_characters)
        delete [] this->d->m_characters;

    delete this->d;
}

CharifyElement::ColorMode CharifyElement::mode() const
{
    return this->d->m_mode;
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

bool CharifyElement::smooth() const
{
    return this->d->m_smooth;
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
    this->d->m_mutex.lock();
    auto fontSize = this->d->m_fontSize;

    int textWidth = packet.caps().width() / fontSize.width();
    int textHeight = packet.caps().height() / fontSize.height();

    if (this->d->m_charTable.isEmpty()) {
        this->d->m_mutex.unlock();

        AkVideoPacket dst({AkVideoCaps::Format_argbpack,
                           textWidth * fontSize.width(),
                           textHeight * fontSize.height(),
                           packet.caps().fps()});
        dst.copyMetadata(packet);
        dst.fill(this->d->m_backgroundColor);

        if (dst)
            emit this->oStream(dst);

        return dst;
    }

    this->d->m_videoConverter.setScalingMode(this->d->m_smooth?
                                                 AkVideoConverter::ScalingMode_Linear:
                                                 AkVideoConverter::ScalingMode_Fast);

    this->d->m_videoConverter.begin();
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_argbpack,
                                             textWidth,
                                             textHeight,
                                             {}});
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src) {
        this->d->m_mutex.unlock();

        return {};
    }

    int outWidth = textWidth * fontSize.width();
    int outHeight = textHeight * fontSize.height();

    auto mask = this->d->createMask(src, fontSize, this->d->m_characters);
    this->d->m_mutex.unlock();

    auto ocaps = src.caps();
    ocaps.setWidth(outWidth);
    ocaps.setHeight(outHeight);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    if (this->d->m_mode == ColorModeFixed) {
        this->d->m_mutex.lock();

        for (int y = 0; y < dst.caps().height(); y++) {
            auto maskLine = mask.constLine(0, y);
            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < dst.caps().width(); x++)
                line[x] = this->d->m_palette[maskLine[x]];
        }

        this->d->m_mutex.unlock();
    } else {
        auto br = qRed(this->d->m_backgroundColor);
        auto bg = qGreen(this->d->m_backgroundColor);
        auto bb = qBlue(this->d->m_backgroundColor);

        for (int y = 0; y < dst.caps().height(); y++) {
            int ys = y / fontSize.height();
            auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, ys));
            auto maskLine = mask.constLine(0, y);
            auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < dst.caps().width(); x++) {
                int xs = x / fontSize.width();

                auto &pixel = srcLine[xs];
                auto &alpha = maskLine[x];
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

void CharifyElement::setMode(ColorMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
    emit this->modeChanged(mode);
    this->d->updateCharTable();
}

void CharifyElement::setCharTable(const QString &charTable)
{
    if (this->d->m_charTable == charTable)
        return;

    this->d->m_mutex.lock();
    this->d->m_charTable = charTable;
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->charTableChanged(charTable);
}

void CharifyElement::setFont(const QFont &font)
{
    if (this->d->m_font == font)
        return;

    this->d->m_mutex.lock();
    auto hp = hintingPreferenceToStr->key(this->hintingPreference(),
                                          QFont::PreferFullHinting);
    auto ss = styleStrategyToStr->key(this->styleStrategy(),
                                      QFont::NoAntialias);

    this->d->m_font = font;
    this->d->m_font.setHintingPreference(hp);
    this->d->m_font.setStyleStrategy(ss);
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->fontChanged(font);
}

void CharifyElement::setHintingPreference(const QString &hintingPreference)
{
    auto hp = hintingPreferenceToStr->key(hintingPreference,
                                          QFont::PreferFullHinting);

    if (this->d->m_font.hintingPreference() == hp)
        return;

    this->d->m_mutex.lock();
    this->d->m_font.setHintingPreference(hp);
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->hintingPreferenceChanged(hintingPreference);
}

void CharifyElement::setStyleStrategy(const QString &styleStrategy)
{
    auto ss = styleStrategyToStr->key(styleStrategy, QFont::NoAntialias);

    if (this->d->m_font.styleStrategy() == ss)
        return;

    this->d->m_mutex.lock();
    this->d->m_font.setStyleStrategy(ss);
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->styleStrategyChanged(styleStrategy);
}

void CharifyElement::setForegroundColor(QRgb foregroundColor)
{
    if (this->d->m_foregroundColor == foregroundColor)
        return;

    this->d->m_mutex.lock();
    this->d->m_foregroundColor = foregroundColor;
    this->d->updatePalette();
    this->d->m_mutex.unlock();
    emit this->foregroundColorChanged(foregroundColor);
}

void CharifyElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    this->d->m_mutex.lock();
    this->d->m_backgroundColor = backgroundColor;
    this->d->updatePalette();
    this->d->m_mutex.unlock();
    emit this->backgroundColorChanged(backgroundColor);
}

void CharifyElement::setSmooth(bool smooth)
{
    if (this->d->m_smooth == smooth)
        return;

    this->d->m_smooth = smooth;
    emit this->smoothChanged(smooth);
}

void CharifyElement::setReversed(bool reversed)
{
    if (this->d->m_reversed == reversed)
        return;

    this->d->m_mutex.lock();
    this->d->m_reversed = reversed;
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->reversedChanged(reversed);
}

void CharifyElement::resetMode()
{
    this->setMode(ColorModeNatural);
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

void CharifyElement::resetSmooth()
{
    this->setSmooth(true);
}

void CharifyElement::resetReversed()
{
    this->setReversed(false);
}

QDataStream &operator >>(QDataStream &istream, CharifyElement::ColorMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<CharifyElement::ColorMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, CharifyElement::ColorMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

void CharifyElementPrivate::updateCharTable()
{
    if (this->m_characters)
        delete [] this->m_characters;

    if (this->m_charTable.isEmpty()) {
        this->m_fontSize = this->fontSize(' ', this->m_font);
        this->m_characters = new Character [1];
        this->m_characters[0] = Character(' ',
                                          this->m_font,
                                          this->m_fontSize,
                                          this->m_reversed);
        memset(this->m_colorTable, 0, 256);
    } else {
        this->m_fontSize = this->fontSize(this->m_charTable, this->m_font);
        this->m_characters = new Character [this->m_charTable.size()];
        int i = 0;

        for (auto &chr: this->m_charTable) {
            this->m_characters[i] = Character(chr,
                                              this->m_font,
                                              this->m_fontSize,
                                              this->m_reversed);
            i++;
        }

        std::sort(this->m_characters,
                  this->m_characters + this->m_charTable.size(),
                  [] (const Character &chr1, const Character &chr2) {
                      return chr1.weight() < chr2.weight();
                  });

        auto charMax = this->m_charTable.size() - 1;

        for (int i = 0; i < 256; i++)
            this->m_colorTable[i] = charMax * i / 255;
    }
}

void CharifyElementPrivate::updatePalette()
{
    auto fr = qRed(this->m_foregroundColor);
    auto fg = qGreen(this->m_foregroundColor);
    auto fb = qBlue(this->m_foregroundColor);

    auto br = qRed(this->m_backgroundColor);
    auto bg = qGreen(this->m_backgroundColor);
    auto bb = qBlue(this->m_backgroundColor);

    for (int i = 0; i < 256; i++)
        this->m_palette[i] = qRgb((i * fr + (255 - i) * br) / 255,
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

QSize CharifyElementPrivate::fontSize(const QChar &chr, const QFont &font) const
{
    return QFontMetrics(font).size(Qt::TextSingleLine, chr);
}

AkVideoPacket CharifyElementPrivate::createMask(const AkVideoPacket &src,
                                                const QSize &fontSize,
                                                const Character *characters)
{
    int outWidth = src.caps().width() * fontSize.width();
    int outHeight = src.caps().height() * fontSize.height();

    AkVideoPacket dst({AkVideoCaps::Format_gray8,
                       outWidth,
                       outHeight,
                       src.caps().fps()});
    dst.copyMetadata(src);

    this->m_videoMixer.begin(&dst);

    for (int y = 0; y < src.caps().height(); y++) {
        auto ys = y * fontSize.height();
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto xs = x * fontSize.width();
            auto &chr = characters[this->m_colorTable[qGray(srcLine[x])]];
            this->m_videoMixer.draw(xs, ys, chr.image());
        }
    }

    this->m_videoMixer.end();

    return dst;
}

#include "moc_charifyelement.cpp"
