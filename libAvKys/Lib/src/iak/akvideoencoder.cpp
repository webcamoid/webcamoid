/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include <QVariant>

#include "akvideoencoder.h"
#include "../akfrac.h"
#include "../akvideocaps.h"
#include "../akvideopacket.h"

class AkVideoEncoderPrivate
{
    public:
        QString m_codec;
        AkVideoCaps m_inputCaps;
        int m_bitrate {1500000};
        int m_gop {1000};
        bool m_fillGaps {false};
        QVariantMap m_optionValues;
        AkVideoEncoder::BitrateMode m_bitrateMode {AkVideoEncoder::BitrateMode_VBR};

        qint64 m_ptsOut {0};
        qint64 m_prevPts {-1};
        qint64 m_prevId {-1};
        AkVideoPacket m_prevPacket;
};

AkVideoEncoder::AkVideoEncoder(QObject *parent):
    AkElement{parent}
{
    this->d = new AkVideoEncoderPrivate();
}

AkVideoEncoder::~AkVideoEncoder()
{
    delete this->d;
}

QString AkVideoEncoder::codec() const
{
    return this->d->m_codec;
}

AkVideoCaps AkVideoEncoder::inputCaps() const
{
    return this->d->m_inputCaps;
}

int AkVideoEncoder::bitrate() const
{
    return this->d->m_bitrate;
}

int AkVideoEncoder::gop() const
{
    return this->d->m_gop;
}

QByteArray AkVideoEncoder::headers() const
{
    return {};
}

bool AkVideoEncoder::fillGaps() const
{
    return this->d->m_fillGaps;
}

AkPropertyOptions AkVideoEncoder::options() const
{
    return {};
}

QVariant AkVideoEncoder::optionValue(const QString &option) const
{
    auto options = this->options();

    if (options.isEmpty())
        return {};

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    if (it == options.constEnd())
        return {};

    return this->d->m_optionValues.value(option, it->defaultValue());
}

bool AkVideoEncoder::isOptionSet(const QString &option) const
{
    auto options = this->options();

    if (options.isEmpty())
        return {};

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    return it != options.constEnd();
}

bool AkVideoEncoder::hasHardwareSupport(const QString &codec) const
{
    Q_UNUSED(codec)

    return false;
}

AkVideoEncoder::BitrateMode AkVideoEncoder::bitrateMode() const
{
    return this->d->m_bitrateMode;
}

void AkVideoEncoder::setCodec(const QString &codec)
{
    if (this->d->m_codec == codec)
        return;

    this->d->m_codec = codec;
    emit this->codecChanged(codec);
}

void AkVideoEncoder::setInputCaps(const AkVideoCaps &inputCaps)
{
    if (this->d->m_inputCaps == inputCaps)
        return;

    this->d->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AkVideoEncoder::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkVideoEncoder::setGop(int gop)
{
    if (this->d->m_gop == gop)
        return;

    this->d->m_gop = gop;
    emit this->gopChanged(gop);
}

void AkVideoEncoder::setFillGaps(bool fillGaps)
{
    if (this->d->m_fillGaps == fillGaps)
        return;

    this->d->m_fillGaps = fillGaps;
    emit this->fillGapsChanged(fillGaps);
}

void AkVideoEncoder::setOptionValue(const QString &option, const QVariant &value)
{
    auto curValue = this->optionValue(option);

    if (curValue == value)
        return;

    auto options = this->options();

    if (options.isEmpty())
        return;

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    QVariant defaultValue;

    if (it != options.constEnd())
        defaultValue = it->defaultValue();

    if (value == defaultValue)
        this->d->m_optionValues.remove(option);
    else
        this->d->m_optionValues[option] = value;

    emit this->optionValueChanged(option, value);
}

void AkVideoEncoder::setBitrateMode(BitrateMode bitrateMode)
{
    if (this->d->m_bitrateMode == bitrateMode)
        return;

    this->d->m_bitrateMode = bitrateMode;
    emit this->bitrateModeChanged(bitrateMode);
}

void AkVideoEncoder::resetCodec()
{
    this->setCodec({});
}

void AkVideoEncoder::resetInputCaps()
{
    this->setInputCaps({});
}

void AkVideoEncoder::resetBitrate()
{
    this->setBitrate(1500000);
}

void AkVideoEncoder::resetGop()
{
    this->setGop(1000);
}

void AkVideoEncoder::resetFillGaps()
{
    this->setFillGaps(false);
}

void AkVideoEncoder::resetOptionValue(const QString &option)
{
    auto options = this->options();

    if (options.isEmpty())
        return;

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    QVariant defaultValue;

    if (it != options.constEnd())
        defaultValue = it->defaultValue();

    this->setOptionValue(option, defaultValue);
}

void AkVideoEncoder::resetBitrateMode()
{
    this->setBitrateMode(BitrateMode_VBR);
}

void AkVideoEncoder::resetOptions()
{
    this->resetBitrate();
    this->resetGop();

    for (auto &option: this->options())
        this->resetOptionValue(option.name());
}

bool AkVideoEncoder::discardFrame(const AkVideoPacket &packet) const
{
    if (!packet)
        return true;

    auto fps = this->outputCaps().rawCaps().fps();

    if (!fps)
        return false;

    auto pts = qint64(packet.pts()
                      * packet.timeBase().value()
                      * fps.value());

    return this->d->m_prevPts >= 0
           && this->d->m_prevId == packet.id()
           && pts == this->d->m_prevPts;
}

void AkVideoEncoder::regulateFps(const AkVideoPacket &packet)
{
    if (!packet)
        return;

    auto fps = this->outputCaps().rawCaps().fps();

    if (!fps) {
        this->encodeFrame(packet);

        return;
    }

    auto pts = qint64(packet.pts()
                      * packet.timeBase().value()
                      * fps.value());

    /* The source frame rate is much higher than the output frame rate,
     * discard the exedent frame.
     */
    if (this->d->m_prevPts >= 0
        && this->d->m_prevId == packet.id()
        && pts == this->d->m_prevPts) {
        return;
    }

    // Calculate the number of frames between the previous and the current one.
    qint64 framesDiff =
            this->d->m_prevPts < 0
            || pts <= this->d->m_prevPts
            || this->d->m_prevId != packet.id()?
                1:
                pts - this->d->m_prevPts;
    quint64 fill = framesDiff - 1;

    /* If the fillGaps option is enabled, repeat the previous frame until
     * complete the missings one.
     */
    if (this->fillGaps() && fill > 0)
        for (quint64 i = 0; i < fill; ++i) {
            this->d->m_prevPacket.setPts(this->d->m_ptsOut);
            this->encodeFrame(this->d->m_prevPacket);
            ++this->d->m_ptsOut;
        }

    if (this->d->m_prevPts >= 0 && !this->fillGaps())
        this->d->m_ptsOut += framesDiff;

    this->d->m_prevPacket = packet;
    this->d->m_prevPacket.setPts(this->d->m_ptsOut);
    this->d->m_prevPacket.setDuration(1);
    this->d->m_prevPacket.setTimeBase(fps.invert());
    this->encodeFrame(this->d->m_prevPacket);

    if (this->d->m_prevPts < 0 || this->fillGaps())
        ++this->d->m_ptsOut;

    this->d->m_prevId = packet.id();
    this->d->m_prevPts = pts;
}

void AkVideoEncoder::restartFpsControl()
{
    this->d->m_ptsOut = 0;
    this->d->m_prevPts = -1;
    this->d->m_prevId = -1;
    this->d->m_prevPacket = AkVideoPacket();
}

#include "moc_akvideoencoder.cpp"
