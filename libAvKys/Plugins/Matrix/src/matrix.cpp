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
#include <QFontMetrics>
#include <QMutex>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "matrix.h"
#include "character.h"
#include "raindrop.h"

using  HintingPreferenceToStr = QMap<QFont::HintingPreference, QString>;

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

Q_GLOBAL_STATIC_WITH_ARGS(HintingPreferenceToStr,
                          hintingPreferenceToStr,
                          (initHintingPreferenceToStr()))

using StyleStrategyToStr = QMap<QFont::StyleStrategy, QString>;

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
        {QFont::NoSubpixelAntialias, "NoSubpixelAntialias"},
        {QFont::PreferNoShaping    , "PreferNoShaping "   },
        {QFont::NoFontMerging      , "NoFontMerging"      }
    };

    return styleStrategyToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(StyleStrategyToStr,
                          styleStrategyToStr,
                          (initStyleStrategyToStr()))

class MatrixPrivate
{
    public:
        Matrix *self {nullptr};
        QString m_description {QObject::tr("Matrix")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;
        int m_nDrops {25};
        QString m_charTable;
        QFont m_font {QApplication::font()};
        QRgb m_cursorColor {qRgb(255, 255, 255)};
        QRgb m_foregroundColor {qRgb(0, 255, 0)};
        QRgb m_backgroundColor {qRgb(0, 0, 0)};
        int m_minDropLength {3};
        int m_maxDropLength {20};
        qreal m_minSpeed {0.5};
        qreal m_maxSpeed {5.0};
        bool m_smooth {true};
        bool m_showCursor {true};
        bool m_showRain {true};
        Character *m_characters {nullptr};
        QRgb m_palette[256];
        int m_colorTable[256];
        QSize m_fontSize;
        QList<RainDrop> m_rain;
        QMutex m_mutex;

        explicit MatrixPrivate(Matrix *self);
        void updateCharTable();
        void updatePalette();
        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QSize fontSize(const QChar &chr, const QFont &font) const;
        AkVideoPacket createMask(const AkVideoPacket &src,
                                 const QSize &fontSize,
                                 const Character *characters);
        AkVideoPacket applyMask(const AkVideoPacket &src,
                                const AkVideoPacket &mask);
        AkVideoPacket renderdrop(const RainDrop &drop,
                                 const QSize &fontSize,
                                 const Character *characters,
                                 bool showCursor);
        void renderRain(AkVideoPacket &src,
                        const QSize &fontSize,
                        const Character *characters);
};

Matrix::Matrix(QObject *parent):
    QObject(parent)
{
    this->d = new MatrixPrivate(this);
    this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache);

    for (int i = 32; i < 127; i++)
        this->d->m_charTable.append(QChar(i));

    this->d->m_font.setHintingPreference(QFont::PreferFullHinting);
    this->d->m_font.setStyleStrategy(QFont::NoAntialias);
    this->d->updateCharTable();
    this->d->updatePalette();
}

Matrix::~Matrix()
{
    if (this->d->m_characters)
        delete [] this->d->m_characters;

    delete this->d;
}

QString Matrix::description() const
{
    return this->d->m_description;
}

AkElementType Matrix::type() const
{
    return this->d->m_type;
}

AkElementCategory Matrix::category() const
{
    return this->d->m_category;
}

void *Matrix::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Matrix::create(const QString &id)
{
    Q_UNUSED(id)

    return new Matrix;
}

int Matrix::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Matrix",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

int Matrix::nDrops() const
{
    return this->d->m_nDrops;
}

QString Matrix::charTable() const
{
    return this->d->m_charTable;
}

QFont Matrix::font() const
{
    return this->d->m_font;
}

QString Matrix::hintingPreference() const
{
    return hintingPreferenceToStr->value(this->d->m_font.hintingPreference(),
                                         "PreferFullHinting");
}

QString Matrix::styleStrategy() const
{
    return styleStrategyToStr->value(this->d->m_font.styleStrategy(),
                                     "NoAntialias");
}

QRgb Matrix::cursorColor() const
{
    return this->d->m_cursorColor;
}

QRgb Matrix::foregroundColor() const
{
    return this->d->m_foregroundColor;
}

QRgb Matrix::backgroundColor() const
{
    return this->d->m_backgroundColor;
}

int Matrix::minDropLength() const
{
    return this->d->m_minDropLength;
}

int Matrix::maxDropLength() const
{
    return this->d->m_maxDropLength;
}

qreal Matrix::minSpeed() const
{
    return this->d->m_minSpeed;
}

qreal Matrix::maxSpeed() const
{
    return this->d->m_maxSpeed;
}

bool Matrix::smooth() const
{
    return this->d->m_smooth;
}

bool Matrix::showCursor() const
{
    return this->d->m_showCursor;
}

bool Matrix::showRain() const
{
    return this->d->m_showRain;
}

void Matrix::deleteThis(void *userData) const
{
    delete reinterpret_cast<Matrix *>(userData);
}

QString Matrix::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Matrix/share/qml/main.qml");
}

void Matrix::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Matrix", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Matrix::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();
    auto fontSize = this->d->m_fontSize;

    int textWidth = packet.caps().width() / fontSize.width();
    int textHeight = packet.caps().height() / fontSize.height();

    if (this->d->m_charTable.isEmpty()) {
        this->d->m_mutex.unlock();

        AkVideoPacket dst({AkVideoCaps::Format_xrgbpack,
                           textWidth * fontSize.width(),
                           textHeight * fontSize.height(),
                           packet.caps().fps()});
        dst.copyMetadata(packet);
        dst.fill(this->d->m_backgroundColor);

        if (dst)
            this->oStream(dst);

        return dst;
    }

    this->d->m_videoConverter.setScalingMode(this->d->m_smooth?
                                                 AkVideoConverter::ScalingMode_Linear:
                                                 AkVideoConverter::ScalingMode_Fast);

    this->d->m_videoConverter.begin();
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_y8,
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

    src = this->d->applyMask(src, mask);

    if (this->d->m_showRain)
        this->d->renderRain(src, fontSize, this->d->m_characters);

    this->d->m_mutex.unlock();

    AkVideoPacket dst({AkVideoCaps::Format_xrgbpack,
                       outWidth,
                       outHeight,
                       src.caps().fps()});
    dst.copyMetadata(src);

    for (int y = 0; y < dst.caps().height(); y++) {
        auto chrLine = src.constLine(0, y);
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < dst.caps().width(); x++)
            dstLine[x] = this->d->m_palette[chrLine[x]];
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

void Matrix::setNDrops(int nDrops)
{
    if (this->d->m_nDrops == nDrops)
        return;

    this->d->m_mutex.lock();
    this->d->m_nDrops = nDrops;
    this->d->m_mutex.unlock();
    emit this->nDropsChanged(nDrops);
}

void Matrix::setCharTable(const QString &charTable)
{
    if (this->d->m_charTable == charTable)
        return;

    this->d->m_mutex.lock();
    this->d->m_charTable = charTable;
    this->d->m_rain.clear();
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->charTableChanged(charTable);
}

void Matrix::setFont(const QFont &font)
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
    this->d->m_rain.clear();
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->fontChanged(font);
}

void Matrix::setHintingPreference(const QString &hintingPreference)
{
    auto hp = hintingPreferenceToStr->key(hintingPreference,
                                          QFont::PreferFullHinting);

    if (this->d->m_font.hintingPreference() == hp)
        return;

    this->d->m_mutex.lock();
    this->d->m_font.setHintingPreference(hp);
    this->d->m_rain.clear();
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->hintingPreferenceChanged(hintingPreference);
}

void Matrix::setStyleStrategy(const QString &styleStrategy)
{
    auto ss = styleStrategyToStr->key(styleStrategy, QFont::NoAntialias);

    if (this->d->m_font.styleStrategy() == ss)
        return;

    this->d->m_mutex.lock();
    this->d->m_font.setStyleStrategy(ss);
    this->d->m_rain.clear();
    this->d->updateCharTable();
    this->d->m_mutex.unlock();
    emit this->styleStrategyChanged(styleStrategy);
}

void Matrix::setCursorColor(QRgb cursorColor)
{
    if (this->d->m_cursorColor == cursorColor)
        return;

    this->d->m_mutex.lock();
    this->d->m_cursorColor = cursorColor;
    this->d->updatePalette();
    this->d->m_mutex.unlock();
    emit this->cursorColorChanged(cursorColor);
}

void Matrix::setForegroundColor(QRgb foregroundColor)
{
    if (this->d->m_foregroundColor == foregroundColor)
        return;

    this->d->m_mutex.lock();
    this->d->m_foregroundColor = foregroundColor;
    this->d->updatePalette();
    this->d->m_mutex.unlock();
    emit this->foregroundColorChanged(foregroundColor);
}

void Matrix::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    this->d->m_mutex.lock();
    this->d->m_backgroundColor = backgroundColor;
    this->d->updatePalette();
    this->d->m_mutex.unlock();
    emit this->backgroundColorChanged(backgroundColor);
}

void Matrix::setMinDropLength(int minDropLength)
{
    if (this->d->m_minDropLength == minDropLength)
        return;

    this->d->m_mutex.lock();
    this->d->m_minDropLength = minDropLength;
    this->d->m_mutex.unlock();
    emit this->minDropLengthChanged(minDropLength);
}

void Matrix::setMaxDropLength(int maxDropLength)
{
    if (this->d->m_maxDropLength == maxDropLength)
        return;

    this->d->m_mutex.lock();
    this->d->m_maxDropLength = maxDropLength;
    this->d->m_mutex.unlock();
    emit this->maxDropLengthChanged(maxDropLength);
}

void Matrix::setMinSpeed(qreal minSpeed)
{
    if (qFuzzyCompare(this->d->m_minSpeed, minSpeed))
        return;

    this->d->m_mutex.lock();
    this->d->m_minSpeed = minSpeed;
    this->d->m_mutex.unlock();
    emit this->minSpeedChanged(minSpeed);
}

void Matrix::setMaxSpeed(qreal maxSpeed)
{
    if (qFuzzyCompare(this->d->m_maxSpeed, maxSpeed))
        return;

    this->d->m_mutex.lock();
    this->d->m_maxSpeed = maxSpeed;
    this->d->m_mutex.unlock();
    emit this->maxSpeedChanged(maxSpeed);
}

void Matrix::setSmooth(bool smooth)
{
    if (this->d->m_smooth == smooth)
        return;

    this->d->m_smooth = smooth;
    emit this->smoothChanged(smooth);
}

void Matrix::setShowCursor(bool showCursor)
{
    if (this->d->m_showCursor == showCursor)
        return;

    this->d->m_mutex.lock();
    this->d->m_showCursor = showCursor;
    this->d->m_mutex.unlock();
    emit this->showCursorChanged(showCursor);
}

void Matrix::setShowRain(bool showRain)
{
    if (this->d->m_showRain == showRain)
        return;

    this->d->m_showRain = showRain;
    emit this->showRainChanged(showRain);
}

void Matrix::resetNDrops()
{
    this->setNDrops(25);
}

void Matrix::resetCharTable()
{
    QString charTable;

    for (int i = 32; i < 127; i++)
        charTable.append(QChar(i));

    this->setCharTable(charTable);
}

void Matrix::resetFont()
{
    this->setFont(QApplication::font());
}

void Matrix::resetHintingPreference()
{
    this->setHintingPreference("PreferFullHinting");
}

void Matrix::resetStyleStrategy()
{
    this->setStyleStrategy("NoAntialias");
}

void Matrix::resetCursorColor()
{
    this->setCursorColor(qRgb(255, 255, 255));
}

void Matrix::resetForegroundColor()
{
    this->setForegroundColor(qRgb(0, 255, 0));
}

void Matrix::resetBackgroundColor()
{
    this->setBackgroundColor(qRgb(0, 0, 0));
}

void Matrix::resetMinDropLength()
{
    this->setMinDropLength(3);
}

void Matrix::resetMaxDropLength()
{
    this->setMaxDropLength(20);
}

void Matrix::resetMinSpeed()
{
    this->setMinSpeed(0.5);
}

void Matrix::resetMaxSpeed()
{
    this->setMaxSpeed(5.0);
}

void Matrix::resetSmooth()
{
    this->setSmooth(true);
}

void Matrix::resetShowCursor()
{
    this->setShowCursor(true);
}

void Matrix::resetShowRain()
{
    this->setShowRain(true);
}

MatrixPrivate::MatrixPrivate(Matrix *self):
    self(self)
{

}

void MatrixPrivate::updateCharTable()
{
    if (this->m_characters)
        delete [] this->m_characters;

    if (this->m_charTable.isEmpty()) {
        this->m_fontSize = this->fontSize(' ', this->m_font);
        this->m_characters = new Character [1];
        this->m_characters[0] = Character(' ', this->m_font, this->m_fontSize);
        memset(this->m_colorTable, 0, 256);
    } else {
        this->m_fontSize = this->fontSize(this->m_charTable, this->m_font);
        this->m_characters = new Character [this->m_charTable.size()];
        int i = 0;

        for (auto &chr: this->m_charTable) {
            this->m_characters[i] = Character(chr, this->m_font, this->m_fontSize);
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

void MatrixPrivate::updatePalette()
{
    int r0 = qRed(this->m_backgroundColor);
    int g0 = qGreen(this->m_backgroundColor);
    int b0 = qBlue(this->m_backgroundColor);

    int rDiff = qRed(this->m_foregroundColor) - r0;
    int gDiff = qGreen(this->m_foregroundColor) - g0;
    int bDiff = qBlue(this->m_foregroundColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff + 127 * r0) / 127;
        int g = (i * gDiff + 127 * g0) / 127;
        int b = (i * bDiff + 127 * b0) / 127;

        this->m_palette[i] = qRgb(r, g, b);
    }

    r0 = qRed(this->m_foregroundColor);
    g0 = qGreen(this->m_foregroundColor);
    b0 = qBlue(this->m_foregroundColor);

    rDiff = qRed(this->m_cursorColor) - r0;
    gDiff = qGreen(this->m_cursorColor) - g0;
    bDiff = qBlue(this->m_cursorColor) - b0;

    for (int i = 0; i < 128; i++) {
        int r = (i * rDiff + 127 * r0) / 127;
        int g = (i * gDiff + 127 * g0) / 127;
        int b = (i * bDiff + 127 * b0) / 127;

        this->m_palette[i + 128] = qRgb(r, g, b);
    }
}

QSize MatrixPrivate::fontSize(const QString &chrTable,
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

QSize MatrixPrivate::fontSize(const QChar &chr, const QFont &font) const
{
    return QFontMetrics(font).size(Qt::TextSingleLine, chr);
}

AkVideoPacket MatrixPrivate::createMask(const AkVideoPacket &src,
                                               const QSize &fontSize,
                                               const Character *characters)
{
    int outWidth = src.caps().width() * fontSize.width();
    int outHeight = src.caps().height() * fontSize.height();

    auto ocaps = src.caps();
    ocaps.setWidth(outWidth);
    ocaps.setHeight(outHeight);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    this->m_videoMixer.begin(&dst);

    for (int y = 0; y < src.caps().height(); y++) {
        auto ys = y * fontSize.height();
        auto srcLine = src.constLine(0, y);

        for (int x = 0; x < src.caps().width(); x++) {
            auto xs = x * fontSize.width();
            auto &chr = characters[this->m_colorTable[srcLine[x]]];
            this->m_videoMixer.draw(xs, ys, chr.image());
        }
    }

    this->m_videoMixer.end();

    return dst;
}

AkVideoPacket MatrixPrivate::applyMask(const AkVideoPacket &src,
                                              const AkVideoPacket &mask)
{
    auto ocaps = src.caps();
    ocaps.setWidth(mask.caps().width());
    ocaps.setHeight(mask.caps().height());
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    auto fontWidth = mask.caps().width() / src.caps().width();
    auto fontHeight = mask.caps().height() / src.caps().height();

    for (int y = 0; y < dst.caps().height(); y++) {
        int ys = y / fontHeight;
        auto srcLine = src.constLine(0, ys);
        auto maskLine = mask.constLine(0, y);
        auto dstLine = dst.line(0, y);

        for (int x = 0; x < dst.caps().width(); x++) {
            int xs = x / fontWidth;
            dstLine[x] = maskLine[x] * srcLine[xs] / 255;
        }
    }

    return dst;
}

AkVideoPacket MatrixPrivate::renderdrop(const RainDrop &drop,
                                               const QSize &fontSize,
                                               const Character *characters,
                                               bool showCursor)
{
    AkVideoPacket dropSprite({AkVideoCaps::Format_y8,
                              fontSize.width(),
                              fontSize.height() * drop.length(),
                              {}});
    int len_1 = drop.length() - 1;
    int yd = len_1 * fontSize.height();
    int j = len_1;

    for (int i = 0; i < drop.length(); i++) {
        auto character = characters[drop.chr(i)];
        auto &sprite = character.image();

        if (showCursor && i == 0) {
            for (int y = 0; y < sprite.caps().height(); y++) {
                auto srcLine = sprite.constLine(0, y);
                auto dstLine = dropSprite.line(0, yd + y);

                for (int x = 0; x < sprite.caps().width(); x++)
                    dstLine[x] = 255 - srcLine[x];
            }
        } else {
            for (int y = 0; y < sprite.caps().height(); y++) {
                auto srcLine = sprite.constLine(0, y);
                auto dstLine = dropSprite.line(0, yd + y);

                for (int x = 0; x < sprite.caps().width(); x++)
                    dstLine[x] = j * srcLine[x] / len_1;
            }
        }

        yd -= fontSize.height();
        j--;
    }

    return dropSprite;
}

void MatrixPrivate::renderRain(AkVideoPacket &src,
                                      const QSize &fontSize,
                                      const Character *characters)
{
    int textWidth = src.caps().width() / fontSize.width();
    int textHeight = src.caps().height() / fontSize.height();
    bool randomStart = this->m_rain.isEmpty();

    while (this->m_rain.size() < this->m_nDrops)
        this->m_rain << RainDrop(textWidth,
                                 textHeight,
                                 this->m_charTable.size(),
                                 this->m_minDropLength,
                                 this->m_maxDropLength,
                                 this->m_minSpeed,
                                 this->m_maxSpeed,
                                 randomStart);

    this->m_videoMixer.begin(&src);

    for (int i = 0; i < this->m_rain.size(); i++) {
        auto &drop = this->m_rain[i];

        if (drop.isVisible()) {
            auto sprite = this->renderdrop(drop,
                                           fontSize,
                                           characters,
                                           this->m_showCursor);
            this->m_videoMixer.draw(drop.x() * fontSize.width(),
                                    drop.y() * fontSize.height(),
                                    sprite);
            drop++;
        } else {
            this->m_rain.removeAt(i);
            i--;
        }
    }

    this->m_videoMixer.end();
}

#include "moc_matrix.cpp"
