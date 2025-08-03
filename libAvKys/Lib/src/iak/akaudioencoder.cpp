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

#include "akaudioencoder.h"
#include "../akaudiocaps.h"

class AkAudioEncoderPrivate
{
    public:
        QString m_codec;
        AkAudioCaps m_inputCaps;
        int m_bitrate {128000};
        bool m_fillGaps {false};
        QVariantMap m_optionValues;
};

AkAudioEncoder::AkAudioEncoder(QObject *parent):
    AkElement{parent}
{
    this->d = new AkAudioEncoderPrivate();
}

AkAudioEncoder::~AkAudioEncoder()
{
    delete this->d;
}

QString AkAudioEncoder::codec() const
{
    return this->d->m_codec;
}

AkAudioCaps AkAudioEncoder::inputCaps() const
{
    return this->d->m_inputCaps;
}

int AkAudioEncoder::bitrate() const
{
    return this->d->m_bitrate;
}

QByteArray AkAudioEncoder::headers() const
{
    return {};
}

bool AkAudioEncoder::fillGaps() const
{
    return this->d->m_fillGaps;
}

AkPropertyOptions AkAudioEncoder::options() const
{
    return {};
}

QVariant AkAudioEncoder::optionValue(const QString &option) const
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

bool AkAudioEncoder::isOptionSet(const QString &option) const
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

void AkAudioEncoder::setCodec(const QString &codec)
{
    if (this->d->m_codec == codec)
        return;

    this->d->m_codec = codec;
    emit this->codecChanged(codec);
}

void AkAudioEncoder::setInputCaps(const AkAudioCaps &inputCaps)
{
    if (this->d->m_inputCaps == inputCaps)
        return;

    this->d->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AkAudioEncoder::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkAudioEncoder::setFillGaps(bool fillGaps)
{
    if (this->d->m_fillGaps == fillGaps)
        return;

    this->d->m_fillGaps = fillGaps;
    emit this->fillGapsChanged(fillGaps);
}

void AkAudioEncoder::setOptionValue(const QString &option,
                                    const QVariant &value)
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

void AkAudioEncoder::resetCodec()
{
    this->setCodec({});
}

void AkAudioEncoder::resetInputCaps()
{
    this->setInputCaps({});
}

void AkAudioEncoder::resetBitrate()
{
    this->setBitrate(128000);
}

void AkAudioEncoder::resetFillGaps()
{
    this->setFillGaps(false);
}

void AkAudioEncoder::resetOptionValue(const QString &option)
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

void AkAudioEncoder::resetOptions()
{
    this->resetBitrate();

    for (auto &option: this->options())
        this->resetOptionValue(option.name());
}

#include "moc_akaudioencoder.cpp"
