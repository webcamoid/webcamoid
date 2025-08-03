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
#include "../akvideocaps.h"

class AkVideoEncoderPrivate
{
    public:
        QString m_codec;
        AkVideoCaps m_inputCaps;
        int m_bitrate {1500000};
        int m_gop {1000};
        bool m_fillGaps {false};
        QVariantMap m_optionValues;
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

void AkVideoEncoder::resetOptions()
{
    this->resetBitrate();
    this->resetGop();

    for (auto &option: this->options())
        this->resetOptionValue(option.name());
}

#include "moc_akvideoencoder.cpp"
