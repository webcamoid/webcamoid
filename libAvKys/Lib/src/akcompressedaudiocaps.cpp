/* Webcamoid, webcam capture application.
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

#include <QDataStream>
#include <QDebug>
#include <QMetaEnum>
#include <QSize>
#include <QQmlEngine>

#include "akcompressedaudiocaps.h"
#include "akfrac.h"
#include "akcaps.h"
#include "akcompressedcaps.h"

class AkCompressedAudioCapsPrivate
{
    public:
        AkCompressedAudioCaps::AudioCodecID m_codec {AkCompressedAudioCaps::AudioCodecID_unknown};
        int m_bps {0};
        int m_channels {0};
        int m_rate {0};
};

AkCompressedAudioCaps::AkCompressedAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCompressedAudioCapsPrivate();
}

AkCompressedAudioCaps::AkCompressedAudioCaps(AudioCodecID codec,
                                             int bps,
                                             int channels,
                                             int rate):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();
    this->d->m_codec = codec;
    this->d->m_bps = bps;
    this->d->m_channels = channels;
    this->d->m_rate = rate;
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();

    if (other.type() == AkCaps::CapsAudioCompressed) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_bps = data->d->m_bps;
        this->d->m_channels = data->d->m_channels;
        this->d->m_rate = data->d->m_rate;
    }
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCompressedCaps &other)
{
    this->d = new AkCompressedAudioCapsPrivate();

    if (other.type() == AkCompressedCaps::CapsType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_bps = data->d->m_bps;
        this->d->m_channels = data->d->m_channels;
        this->d->m_rate = data->d->m_rate;
    }
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCompressedAudioCaps &other):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();
    this->d->m_codec = other.d->m_codec;
    this->d->m_bps = other.d->m_bps;
    this->d->m_channels = other.d->m_channels;
    this->d->m_rate = other.d->m_rate;
}

AkCompressedAudioCaps::~AkCompressedAudioCaps()
{
    delete this->d;
}

AkCompressedAudioCaps &AkCompressedAudioCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsAudioCompressed) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_bps = data->d->m_bps;
        this->d->m_channels = data->d->m_channels;
        this->d->m_rate = data->d->m_rate;
    } else {
        this->d->m_codec = AudioCodecID_unknown;
        this->d->m_bps = 0;
        this->d->m_channels = 0;
        this->d->m_rate = {};
    }

    return *this;
}

AkCompressedAudioCaps &AkCompressedAudioCaps::operator =(const AkCompressedCaps &other)
{
    if (other.type() == AkCompressedCaps::CapsType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_bps = data->d->m_bps;
        this->d->m_channels = data->d->m_channels;
        this->d->m_rate = data->d->m_rate;
    } else {
        this->d->m_codec = AudioCodecID_unknown;
        this->d->m_bps = 0;
        this->d->m_channels = 0;
        this->d->m_rate = {};
    }

    return *this;
}

AkCompressedAudioCaps &AkCompressedAudioCaps::operator =(const AkCompressedAudioCaps &other)
{
    if (this != &other) {
        this->d->m_codec = other.d->m_codec;
        this->d->m_bps = other.d->m_bps;
        this->d->m_channels = other.d->m_channels;
        this->d->m_rate = other.d->m_rate;
    }

    return *this;
}

bool AkCompressedAudioCaps::operator ==(const AkCompressedAudioCaps &other) const
{
    return this->d->m_codec == other.d->m_codec
            && this->d->m_bps == other.d->m_bps
            && this->d->m_channels == other.d->m_channels
            && this->d->m_rate == other.d->m_rate;
}

bool AkCompressedAudioCaps::operator !=(const AkCompressedAudioCaps &other) const
{
    return !(*this == other);
}

AkCompressedAudioCaps::operator bool() const
{
    return this->d->m_codec != AudioCodecID_unknown
           && this->d->m_bps > 0
           && this->d->m_channels > 0
           && this->d->m_rate > 0;
}

AkCompressedAudioCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsAudioCompressed);
    caps.setPrivateData(new AkCompressedAudioCaps(*this),
                        [] (void *data) -> void * {
                            return new AkCompressedAudioCaps(*reinterpret_cast<AkCompressedAudioCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkCompressedAudioCaps *>(data);
                        });

    return caps;
}

AkCompressedAudioCaps::operator AkCompressedCaps() const
{
    AkCompressedCaps caps;
    caps.setType(AkCompressedCaps::CapsType_Audio);
    caps.setCodecID(this->d->m_codec);
    caps.setPrivateData(new AkCompressedAudioCaps(*this),
                        [] (void *data) -> void * {
                            return new AkCompressedAudioCaps(*reinterpret_cast<AkCompressedAudioCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkCompressedAudioCaps *>(data);
                        });

    return caps;
}

QObject *AkCompressedAudioCaps::create()
{
    return new AkCompressedAudioCaps();
}

QObject *AkCompressedAudioCaps::create(const AkCaps &caps)
{
    return new AkCompressedAudioCaps(caps);
}

QObject *AkCompressedAudioCaps::create(const AkCompressedCaps &caps)
{
    return new AkCompressedAudioCaps(caps);
}

QObject *AkCompressedAudioCaps::create(const AkCompressedAudioCaps &caps)
{
    return new AkCompressedAudioCaps(caps);
}

QObject *AkCompressedAudioCaps::create(AudioCodecID codec,
                                       int bps,
                                       int channels,
                                       int rate)
{
    return new AkCompressedAudioCaps(codec, bps, channels, rate);
}

QVariant AkCompressedAudioCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCompressedAudioCaps::AudioCodecID AkCompressedAudioCaps::codec() const
{
    return this->d->m_codec;
}

int AkCompressedAudioCaps::bps() const
{
    return this->d->m_bps;
}

int AkCompressedAudioCaps::channels() const
{
    return this->d->m_channels;
}

int AkCompressedAudioCaps::rate() const
{
    return this->d->m_rate;
}

QString AkCompressedAudioCaps::audioCodecIDToString(AudioCodecID codecID)
{
    AkCompressedAudioCaps caps;
    int codecIndex = caps.metaObject()->indexOfEnumerator("AudioCodecID");
    QMetaEnum codecEnum = caps.metaObject()->enumerator(codecIndex);
    QString codec(codecEnum.valueToKey(codecID));
    codec.remove("AudioCodecID_");

    return codec;
}

void AkCompressedAudioCaps::setCodec(AudioCodecID codec)
{
    if (this->d->m_codec == codec)
        return;

    this->d->m_codec = codec;
    emit this->codecChanged(codec);
}

void AkCompressedAudioCaps::setBps(int bps)
{
    if (this->d->m_bps == bps)
        return;

    this->d->m_bps = bps;
    emit this->bpsChanged(bps);
}

void AkCompressedAudioCaps::setChannels(int channels)
{
    if (this->d->m_channels == channels)
        return;

    this->d->m_channels = channels;
    emit this->channelsChanged(channels);
}

void AkCompressedAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void AkCompressedAudioCaps::resetCodec()
{
    this->setCodec(AudioCodecID_unknown);
}

void AkCompressedAudioCaps::resetBps()
{
    this->setBps(0);
}

void AkCompressedAudioCaps::resetChannels()
{
    this->setChannels(0);
}

void AkCompressedAudioCaps::resetRate()
{
    this->setRate(0);
}

void AkCompressedAudioCaps::registerTypes()
{
    qRegisterMetaType<AkCompressedAudioCaps>("AkCompressedAudioCaps");
    qRegisterMetaType<AudioCodecID>("AkAudioCodecID");
    qmlRegisterSingletonType<AkCompressedAudioCaps>("Ak", 1, 0, "AkCompressedAudioCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedAudioCaps();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedAudioCaps &caps)
{
    debug.nospace() << "AkCompressedAudioCaps("
                    << "codec="
                    << caps.codec()
                    << ",bps="
                    << caps.bps()
                    << ",channels="
                    << caps.channels()
                    << ",rate="
                    << caps.rate()
                    << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkCompressedAudioCaps::AudioCodecID codecID)
{
    debug.nospace() << AkCompressedAudioCaps::audioCodecIDToString(codecID).toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCompressedAudioCaps &caps)
{
    AkCompressedAudioCaps::AudioCodecID codec;
    istream >> codec;
    caps.setCodec(codec);
    int bps = 0;
    istream >> bps;
    caps.setBps(bps);
    int channels = 0;
    istream >> channels;
    caps.setChannels(channels);
    int rate = 0;
    istream >> rate;
    caps.setRate(rate);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCompressedAudioCaps &caps)
{
    ostream << caps.codec();
    ostream << caps.bps();
    ostream << caps.channels();
    ostream << caps.rate();

    return ostream;
}

#include "moc_akcompressedaudiocaps.cpp"
