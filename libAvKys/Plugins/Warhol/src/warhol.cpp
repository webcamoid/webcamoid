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

#include <QColor>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "warhol.h"

class WarholPrivate
{
    public:
        Warhol *self {nullptr};
        QString m_description {QObject::tr("Warhol")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        int m_frameLen {2};
        int m_levels {3};
        int m_saturation {127};
        int m_luminance {127};
        int m_paletteOffset {0};
        int m_shadowThLow {0};
        int m_shadowThHi {31};
        QRgb m_shadowColor {qRgb(0, 0, 0)};
        QRgb *m_palette {nullptr};
        IAkVideoFilterPtr m_otsuFilter {akPluginManager->create<IAkVideoFilter>("VideoFilter/Otsu")};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;

        explicit WarholPrivate(Warhol *self);
        void createPalette(int frameLen,
                           int levels,
                           int saturation,
                           int luminance,
                           int paletteOffset);
        AkVideoPacket colorize(const AkVideoPacket &otsu,
                               int frame,
                               int levels) const;
        AkVideoPacket blackLevel(const AkVideoPacket &src,
                                 QRgb color,
                                 int thresholdLow,
                                 int thresholdHi) const;
};

Warhol::Warhol(QObject *parent):
      QObject(parent)
{
    this->d = new WarholPrivate(this);
}

Warhol::~Warhol()
{
    if (this->d->m_palette)
        delete [] this->d->m_palette;

    delete this->d;
}

QString Warhol::description() const
{
    return this->d->m_description;
}

AkElementType Warhol::type() const
{
    return this->d->m_type;
}

AkElementCategory Warhol::category() const
{
    return this->d->m_category;
}

void *Warhol::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Warhol::create(const QString &id)
{
    Q_UNUSED(id)

    return new Warhol;
}

int Warhol::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Warhol",
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

int Warhol::frameLen() const
{
    return this->d->m_frameLen;
}

int Warhol::levels() const
{
    return this->d->m_levels;
}

int Warhol::saturation() const
{
    return this->d->m_saturation;
}

int Warhol::luminance() const
{
    return this->d->m_luminance;
}

int Warhol::paletteOffset() const
{
    return this->d->m_paletteOffset;
}

int Warhol::shadowThLow() const
{
    return this->d->m_shadowThLow;
}

int Warhol::shadowThHi() const
{
    return this->d->m_shadowThHi;
}

QRgb Warhol::shadowColor() const
{
    return this->d->m_shadowColor;
}

void Warhol::deleteThis(void *userData) const
{
    delete reinterpret_cast<Warhol *>(userData);
}

QString Warhol::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Warhol/share/qml/main.qml");
}

void Warhol::controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Warhol", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Warhol::iVideoStream(const AkVideoPacket &packet)
{
    int frameLen = qMax(this->d->m_frameLen, 1);
    int subWidth = packet.caps().width() / frameLen;
    int subHeight = packet.caps().height() / frameLen;

    this->d->m_videoConverter.begin();
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_ya88pack,
                                             subWidth,
                                             subHeight,
                                             {}});
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    int levels = qBound(2, this->d->m_levels, 4);
    this->d->createPalette(frameLen,
                           levels,
                           qBound(0, this->d->m_saturation, 255),
                           qBound(0, this->d->m_luminance, 255),
                           qBound(0, this->d->m_paletteOffset, 360));
    this->d->m_otsuFilter->property<IAkPropertyInt>("levels")->setValue(levels);
    AkVideoPacket otsu = this->d->m_otsuFilter->iStream(src);

    auto shadowThLow = qBound(0, this->d->m_shadowThLow, 255);
    auto shadowThHi = qBound(0, this->d->m_shadowThHi, 255);

    if (shadowThLow > shadowThHi)
        std::swap(shadowThLow, shadowThHi);

    bool drawBack = shadowThHi > 0;
    AkVideoPacket black;

    if (drawBack)
        black = this->d->blackLevel(src,
                                    this->d->m_shadowColor,
                                    shadowThLow,
                                    shadowThHi);

    AkVideoPacket dst({AkVideoCaps::Format_argbpack,
                       subWidth * frameLen,
                       subHeight * frameLen,
                       src.caps().fps()});
    dst.copyMetadata(src);

    for (int j = 0; j < frameLen; j++) {
        int jOffset = j * frameLen;

        for (int i = 0; i < frameLen; i++) {
            auto frame = this->d->colorize(otsu, i + jOffset, levels);

            this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache
                                           | AkVideoMixer::MixerFlagForceBlit);
            this->d->m_videoMixer.begin(&dst);
            this->d->m_videoMixer.draw(i * subWidth, j * subHeight, frame);
            this->d->m_videoMixer.end();

            if (drawBack) {
                this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache);
                this->d->m_videoMixer.begin(&dst);
                this->d->m_videoMixer.draw(i * subWidth, j * subHeight, black);
                this->d->m_videoMixer.end();
            }
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void Warhol::setFrameLen(int frameLen)
{
    if (this->d->m_frameLen == frameLen)
        return;

    this->d->m_frameLen = frameLen;
    emit this->frameLenChanged(frameLen);
}

void Warhol::setLevels(int levels)
{
    if (this->d->m_levels == levels)
        return;

    this->d->m_levels = levels;
    emit this->levelsChanged(levels);
}

void Warhol::setSaturation(int saturation)
{
    if (this->d->m_saturation == saturation)
        return;

    this->d->m_saturation = saturation;
    emit this->saturationChanged(saturation);
}

void Warhol::setLuminance(int luminance)
{
    if (this->d->m_luminance == luminance)
        return;

    this->d->m_luminance = luminance;
    emit this->luminanceChanged(luminance);
}

void Warhol::setPaletteOffset(int paletteOffset)
{
    if (this->d->m_paletteOffset == paletteOffset)
        return;

    this->d->m_paletteOffset = paletteOffset;
    emit this->paletteOffsetChanged(paletteOffset);
}

void Warhol::setShadowThLow(int shadowThLow)
{
    if (this->d->m_shadowThLow == shadowThLow)
        return;

    this->d->m_shadowThLow = shadowThLow;
    emit this->shadowThLowChanged(shadowThLow);
}

void Warhol::setShadowThHi(int shadowThHi)
{
    if (this->d->m_shadowThHi == shadowThHi)
        return;

    this->d->m_shadowThHi = shadowThHi;
    emit this->shadowThHiChanged(shadowThHi);
}

void Warhol::setShadowColor(QRgb shadowColor)
{
    if (this->d->m_shadowColor == shadowColor)
        return;

    this->d->m_shadowColor = shadowColor;
    emit this->shadowColorChanged(shadowColor);
}

void Warhol::resetFrameLen()
{
    this->setFrameLen(2);
}

void Warhol::resetLevels()
{
    this->setLevels(3);
}

void Warhol::resetSaturation()
{
    this->setSaturation(127);
}

void Warhol::resetLuminance()
{
    this->setLuminance(127);
}

void Warhol::resetPaletteOffset()
{
    this->setPaletteOffset(0);
}

void Warhol::resetShadowThLow()
{
    this->setShadowThLow(0);
}

void Warhol::resetShadowThHi()
{
    this->setShadowThHi(31);
}

void Warhol::resetShadowColor()
{
    this->setShadowColor(qRgb(0, 0, 0));
}

WarholPrivate::WarholPrivate(Warhol *self):
      self(self)
{

}

void WarholPrivate::createPalette(int frameLen,
                                  int levels,
                                  int saturation,
                                  int luminance,
                                  int paletteOffset)
{
    if (this->m_palette) {
        delete [] this->m_palette;
        this->m_palette = nullptr;
    }

    auto frames = size_t(frameLen) * size_t(frameLen);
    auto paletteSize = frames * size_t(levels);

    if (paletteSize < 1)
        return;

    this->m_palette = new QRgb [paletteSize];

    for (size_t j = 0; j < frames; j++) {
        auto jOffset = j * levels;
        auto framePalette = this->m_palette + jOffset;

        for (size_t i = 0; i < levels; i++) {
            int hue = (paletteOffset + 360 * (i * frames + jOffset) / paletteSize) % 360;
            auto color = QColor::fromHsl(hue, saturation, luminance);
            framePalette[i] = color.rgb();
        }
    }
}

AkVideoPacket WarholPrivate::colorize(const AkVideoPacket &otsu,
                                      int frame,
                                      int levels) const
{
    auto ocaps = otsu.caps();
    ocaps.setFormat(AkVideoCaps::Format_argbpack);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(otsu);

    auto framePalette = this->m_palette + frame * levels;
    int lumaToLevel[256];

    for (int luma = 0; luma < 256; luma++)
        lumaToLevel[luma] = (levels * luma) >> 8;

    for (int y = 0; y < otsu.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(otsu.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < otsu.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto &color = framePalette[lumaToLevel[pixel >> 8]];
            dstLine[x] = qRgba(qRed(color),
                               qGreen(color),
                               qBlue(color),
                               pixel & 0xff);
        }
    }

    return dst;
}

AkVideoPacket WarholPrivate::blackLevel(const AkVideoPacket &src,
                                        QRgb color,
                                        int thresholdLow,
                                        int thresholdHi) const
{
    auto ocaps = src.caps();
    ocaps.setFormat(AkVideoCaps::Format_argbpack);
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);

    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int a = qAlpha(color);

    for (int y = 0; y < dst.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < dst.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int y = pixel >> 8;

            if (y < thresholdLow)
                y = 0;

            if (y > thresholdHi)
                y = 255;

            dstLine[x] = qRgba(r,
                               g,
                               b,
                               ((255 - y) * (pixel & 0xff) * a) >> 16);
        }
    }

    return dst;
}

#include "moc_warhol.cpp"
