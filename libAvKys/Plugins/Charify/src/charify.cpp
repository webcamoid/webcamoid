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
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "charify.h"
#include "character.h"

enum ColorMode
{
    ColorModeNatural,
    ColorModeFixed
};

class CharifyPrivate;

class ModeChangedCallbacks:
    public IAkNumericPropertyCallbacks<qint32>
{
    public:
        ModeChangedCallbacks(CharifyPrivate *self);
        void valueChanged(qint32 value) override;

    private:
        CharifyPrivate *self;
};

class CharTableChangedCallbacks:
    public IAkObjectPropertyCallbacks<QString>
{
    public:
        CharTableChangedCallbacks(CharifyPrivate *self);
        void valueChanged(const QString &value) override;

    private:
        CharifyPrivate *self;
};

class FontChangedCallbacks:
    public IAkObjectPropertyCallbacks<QFont>
{
    public:
        FontChangedCallbacks(CharifyPrivate *self);
        void valueChanged(const QFont &value) override;

    private:
       CharifyPrivate *self;
};

class HintingPreferenceChangedCallbacks:
    public IAkNumericPropertyCallbacks<qint32>
{
    public:
       HintingPreferenceChangedCallbacks(CharifyPrivate *self);
       void valueChanged(qint32 value) override;

    private:
       CharifyPrivate *self;
};

class StyleStrategyChangedCallbacks:
    public IAkNumericPropertyCallbacks<qint32>
{
    public:
       StyleStrategyChangedCallbacks(CharifyPrivate *self);
       void valueChanged(qint32 value) override;

    private:
       CharifyPrivate *self;
};

class ForegroundColorChangedCallbacks:
    public IAkNumericPropertyCallbacks<QRgb>
{
    public:
       ForegroundColorChangedCallbacks(CharifyPrivate *self);
       void valueChanged(QRgb value) override;

    private:
       CharifyPrivate *self;
};

class BackgroundColorChangedCallbacks:
    public IAkNumericPropertyCallbacks<QRgb>
{
    public:
       BackgroundColorChangedCallbacks(CharifyPrivate *self);
       void valueChanged(QRgb value) override;

    private:
       CharifyPrivate *self;
};

class ReversedChangedCallbacks:
    public IAkNumericPropertyCallbacks<bool>
{
    public:
       ReversedChangedCallbacks(CharifyPrivate *self);
       void valueChanged(bool value) override;

    private:
       CharifyPrivate *self;
};

class CharifyPrivate
{
    public:
        Charify *self {nullptr};
        QString m_description {QObject::tr("ASCII art")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;
        IAkPropertyIntMenu m_mode {QObject::tr("Rendering mode"),
                                   ColorModeNatural, {
                                       {"natural", QObject::tr("Natural"), ColorModeNatural},
                                       {"fixed"  , QObject::tr("Fixed")  , ColorModeNatural}
                                   }};
        IAkPropertyString m_charTable {QObject::tr("Characters table"), {defaultCharTable()}};
        IAkPropertyFont m_font {QObject::tr("Font"), QApplication::font()};
        IAkPropertyIntMenu m_hintingPreference {QObject::tr("Hinting preference"),
                                                QApplication::font().hintingPreference(), {
                                                    {"defaultHinting" , QObject::tr("Default hinting") , QFont::PreferDefaultHinting },
                                                    {"noHinting"      , QObject::tr("No hinting")      , QFont::PreferNoHinting      },
                                                    {"verticalHinting", QObject::tr("Vertical hinting"), QFont::PreferVerticalHinting},
                                                    {"fullHinting"    , QObject::tr("Full hinting")    , QFont::PreferFullHinting    }
                                                }};
        IAkPropertyIntMenu m_styleStrategy {QObject::tr("Style strategy"),
                                            QApplication::font().styleStrategy(), {
                                                {"preferDefault"      , QObject::tr("Prefer default")       , QFont::PreferDefault      },
                                                {"preferBitmap"       , QObject::tr("Prefer bitmap")        , QFont::PreferBitmap       },
                                                {"preferDevice"       , QObject::tr("Prefer device")        , QFont::PreferDevice       },
                                                {"preferOutline"      , QObject::tr("Prefer outline")       , QFont::PreferOutline      },
                                                {"forceOutline"       , QObject::tr("Force outline")        , QFont::ForceOutline       },
                                                {"preferMatch"        , QObject::tr("Prefer match")         , QFont::PreferMatch        },
                                                {"preferQuality"      , QObject::tr("Prefer quality")       , QFont::PreferQuality      },
                                                {"preferAntialias"    , QObject::tr("Prefer antialias")     , QFont::PreferAntialias    },
                                                {"noAntialias"        , QObject::tr("No antialias")         , QFont::NoAntialias        },
                                                {"noSubpixelAntialias", QObject::tr("No subpixel antialias"), QFont::NoSubpixelAntialias},
                                                {"preferNoShaping "   , QObject::tr("Prefer no shaping")    , QFont::PreferNoShaping    },
                                                {"noFontMerging"      , QObject::tr("No font merging")      , QFont::NoFontMerging      }
                                            }};
        IAkPropertyColor m_foregroundColor {QObject::tr("Foreground color"), qRgb(255, 255, 255)};
        IAkPropertyColor m_backgroundColor {QObject::tr("Background color"), qRgb(0, 0, 0)};
        IAkPropertyBool m_smooth {QObject::tr("Smooth"), true};
        IAkPropertyBool m_reversed {QObject::tr("Reversed"), false};
        ModeChangedCallbacks *m_modeChangedCallbacks {nullptr};
        CharTableChangedCallbacks *m_charTableChangedCallbacks {nullptr};
        FontChangedCallbacks *m_fontChangedCallbacks {nullptr};
        HintingPreferenceChangedCallbacks *m_hintingPreferenceChangedCallbacks {nullptr};
        StyleStrategyChangedCallbacks *m_styleStrategyChangedCallbacks {nullptr};
        ForegroundColorChangedCallbacks *m_foregroundColorChangedCallbacks {nullptr};
        BackgroundColorChangedCallbacks *m_backgroundColorChangedCallbacks {nullptr};
        ReversedChangedCallbacks *m_reversedChangedCallbacks {nullptr};

        Character *m_characters {nullptr};
        QRgb m_palette[256];
        int m_colorTable[256];
        QSize m_fontSize;
        QMutex m_mutex;

        explicit CharifyPrivate(Charify *self);
        ~CharifyPrivate();
        void updateCharTable();
        void updatePalette();
        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QSize fontSize(const QChar &chr, const QFont &font) const;
        AkVideoPacket createMask(const AkVideoPacket &src,
                                 const QSize &fontSize,
                                 const Character *characters);

        inline static QString defaultCharTable()
        {
            QString table;

            for (int i = 32; i < 127; i++)
                table.append(QChar(i));

            return table;
        }
};

Charify::Charify(QObject *parent):
      QObject(parent)
{
    this->d = new CharifyPrivate(this);
    this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache);

    this->registerProperty("mode", &this->d->m_mode);
    this->registerProperty("charTable", &this->d->m_charTable);
    this->registerProperty("font", &this->d->m_font);
    this->registerProperty("hintingPreference", &this->d->m_hintingPreference);
    this->registerProperty("styleStrategy", &this->d->m_styleStrategy);
    this->registerProperty("foregroundColor", &this->d->m_foregroundColor);
    this->registerProperty("backgroundColor", &this->d->m_backgroundColor);
    this->registerProperty("smooth", &this->d->m_smooth);
    this->registerProperty("reversed", &this->d->m_reversed);

    this->d->m_mode.subscribe(this->d->m_modeChangedCallbacks);
    this->d->m_charTable.subscribe(this->d->m_charTableChangedCallbacks);
    this->d->m_font.subscribe(this->d->m_fontChangedCallbacks);
    this->d->m_hintingPreference.subscribe(this->d->m_hintingPreferenceChangedCallbacks);
    this->d->m_styleStrategy.subscribe(this->d->m_styleStrategyChangedCallbacks);
    this->d->m_foregroundColor.subscribe(this->d->m_foregroundColorChangedCallbacks);
    this->d->m_backgroundColor.subscribe(this->d->m_backgroundColorChangedCallbacks);
    this->d->m_reversed.subscribe(this->d->m_reversedChangedCallbacks);

    this->d->updateCharTable();
    this->d->updatePalette();
}

Charify::~Charify()
{
    if (this->d->m_characters)
        delete [] this->d->m_characters;

    delete this->d;
}

QString Charify::description() const
{
    return this->d->m_description;
}

AkElementType Charify::type() const
{
    return this->d->m_type;
}

AkElementCategory Charify::category() const
{
    return this->d->m_category;
}

void *Charify::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Charify::create(const QString &id)
{
    Q_UNUSED(id)

    return new Charify;
}

int Charify::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Charify",
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

void Charify::deleteThis(void *userData) const
{
    delete reinterpret_cast<Charify *>(userData);
}

QString Charify::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Charify/share/qml/main.qml");
}

void Charify::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Charify", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Charify::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();
    auto fontSize = this->d->m_fontSize;

    int textWidth = packet.caps().width() / fontSize.width();
    int textHeight = packet.caps().height() / fontSize.height();

    if (this->d->m_charTable.value().isEmpty()) {
        this->d->m_mutex.unlock();

        AkVideoPacket dst({AkVideoCaps::Format_argbpack,
                           textWidth * fontSize.width(),
                           textHeight * fontSize.height(),
                           packet.caps().fps()});
        dst.copyMetadata(packet);
        dst.fill(QRgb(this->d->m_backgroundColor));

        if (dst)
            this->oStream(dst);

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
        auto br = qRed(QRgb(this->d->m_backgroundColor));
        auto bg = qGreen(QRgb(this->d->m_backgroundColor));
        auto bb = qBlue(QRgb(this->d->m_backgroundColor));

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
        this->oStream(dst);

    return dst;
}

ModeChangedCallbacks::ModeChangedCallbacks(CharifyPrivate *self):
    self(self)
{

}

void ModeChangedCallbacks::valueChanged(qint32 value)
{
    Q_UNUSED(value)

    self->updateCharTable();
}

CharTableChangedCallbacks::CharTableChangedCallbacks(CharifyPrivate *self):
    self(self)
{

}

void CharTableChangedCallbacks::valueChanged(const QString &value)
{
    Q_UNUSED(value)

    self->updateCharTable();
}

FontChangedCallbacks::FontChangedCallbacks(CharifyPrivate *self):
    self(self)
{
}

void FontChangedCallbacks::valueChanged(const QFont &value)
{
    auto font = value;
    font.setHintingPreference(QFont::HintingPreference(self->m_hintingPreference.value()));
    font.setStyleStrategy(QFont::StyleStrategy(self->m_styleStrategy.value()));
    self->m_font = font;
    self->updateCharTable();
}

HintingPreferenceChangedCallbacks::HintingPreferenceChangedCallbacks(CharifyPrivate *self):
    self(self)
{

}

void HintingPreferenceChangedCallbacks::valueChanged(qint32 value)
{
    auto font = self->m_font.value();
    font.setHintingPreference(QFont::HintingPreference(value));
    self->m_font.setValue(font);
    self->updateCharTable();
}

StyleStrategyChangedCallbacks::StyleStrategyChangedCallbacks(CharifyPrivate *self):
    self(self)
{

}

void StyleStrategyChangedCallbacks::valueChanged(qint32 value)
{
    auto font = self->m_font.value();
    font.setStyleStrategy(QFont::StyleStrategy(value));
    self->m_font.setValue(font);
    self->updateCharTable();
}

ForegroundColorChangedCallbacks::ForegroundColorChangedCallbacks(CharifyPrivate *self):
      self(self)
{

}

void ForegroundColorChangedCallbacks::valueChanged(QRgb value)
{
    Q_UNUSED(value)

    self->updatePalette();
}

BackgroundColorChangedCallbacks::BackgroundColorChangedCallbacks(CharifyPrivate *self):
      self(self)
{

}

void BackgroundColorChangedCallbacks::valueChanged(QRgb value)
{
    Q_UNUSED(value)

    self->updatePalette();
}

ReversedChangedCallbacks::ReversedChangedCallbacks(CharifyPrivate *self):
      self(self)
{

}

void ReversedChangedCallbacks::valueChanged(bool value)
{
    Q_UNUSED(value)

    self->updateCharTable();
}

CharifyPrivate::CharifyPrivate(Charify *self):
    self(self)
{
    this->m_modeChangedCallbacks = new ModeChangedCallbacks(this);
    this->m_charTableChangedCallbacks = new CharTableChangedCallbacks(this);
    this->m_fontChangedCallbacks = new FontChangedCallbacks(this);
    this->m_hintingPreferenceChangedCallbacks = new HintingPreferenceChangedCallbacks(this);
    this->m_styleStrategyChangedCallbacks = new StyleStrategyChangedCallbacks(this);
    this->m_foregroundColorChangedCallbacks = new ForegroundColorChangedCallbacks(this);
    this->m_backgroundColorChangedCallbacks = new BackgroundColorChangedCallbacks(this);
    this->m_reversedChangedCallbacks = new ReversedChangedCallbacks(this);
}

CharifyPrivate::~CharifyPrivate()
{
    delete this->m_modeChangedCallbacks;
    delete this->m_charTableChangedCallbacks;
    delete this->m_fontChangedCallbacks;
    delete this->m_hintingPreferenceChangedCallbacks;
    delete this->m_styleStrategyChangedCallbacks;
    delete this->m_foregroundColorChangedCallbacks;
    delete this->m_backgroundColorChangedCallbacks;
    delete this->m_reversedChangedCallbacks;
}

void CharifyPrivate::updateCharTable()
{
    if (this->m_characters)
        delete [] this->m_characters;

    auto font = this->m_font.value();

    if (this->m_charTable.value().isEmpty()) {
        this->m_fontSize = this->fontSize(' ', font);
        this->m_characters = new Character [1];
        this->m_characters[0] = Character(' ',
                                          font,
                                          this->m_fontSize,
                                          this->m_reversed);
        memset(this->m_colorTable, 0, 256);
    } else {
        this->m_fontSize = this->fontSize(this->m_charTable.value(),
                                          font);
        this->m_characters = new Character [this->m_charTable.value().size()];
        int i = 0;

        for (auto &chr: this->m_charTable.value()) {
            this->m_characters[i] = Character(chr,
                                              font,
                                              this->m_fontSize,
                                              this->m_reversed);
            i++;
        }

        std::sort(this->m_characters,
                  this->m_characters + this->m_charTable.value().size(),
                  [] (const Character &chr1, const Character &chr2) {
                      return chr1.weight() < chr2.weight();
                  });

        auto charMax = this->m_charTable.value().size() - 1;

        for (int i = 0; i < 256; i++)
            this->m_colorTable[i] = charMax * i / 255;
    }
}

void CharifyPrivate::updatePalette()
{
    auto fr = qRed(QRgb(this->m_foregroundColor));
    auto fg = qGreen(QRgb(this->m_foregroundColor));
    auto fb = qBlue(QRgb(this->m_foregroundColor));

    auto br = qRed(QRgb(this->m_backgroundColor));
    auto bg = qGreen(QRgb(this->m_backgroundColor));
    auto bb = qBlue(QRgb(this->m_backgroundColor));

    for (int i = 0; i < 256; i++)
        this->m_palette[i] = qRgb((i * fr + (255 - i) * br) / 255,
                                  (i * fg + (255 - i) * bg) / 255,
                                  (i * fb + (255 - i) * bb) / 255);
}

QSize CharifyPrivate::fontSize(const QString &chrTable,
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

QSize CharifyPrivate::fontSize(const QChar &chr, const QFont &font) const
{
    return QFontMetrics(font).size(Qt::TextSingleLine, chr);
}

AkVideoPacket CharifyPrivate::createMask(const AkVideoPacket &src,
                                                const QSize &fontSize,
                                                const Character *characters)
{
    int outWidth = src.caps().width() * fontSize.width();
    int outHeight = src.caps().height() * fontSize.height();

    AkVideoPacket dst({AkVideoCaps::Format_y8,
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

#include "moc_charify.cpp"
