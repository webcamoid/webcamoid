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
#include <QQmlEngine>

#include "akcompressedaudiocaps.h"
#include "akcaps.h"
#include "akcompressedcaps.h"
#include "akaudiocaps.h"

class AkCompressedAudioCapsPrivate
{
    public:
        AkCompressedAudioCaps::AudioCodecID m_codec {AkCompressedAudioCaps::AudioCodecID_unknown};
        AkAudioCaps m_rawCaps;
        int m_bitrate {0};
};

AkCompressedAudioCaps::AkCompressedAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCompressedAudioCapsPrivate();
}

AkCompressedAudioCaps::AkCompressedAudioCaps(AudioCodecID codec,
                                             const AkAudioCaps &rawCaps,
                                             int bitrate):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();
    this->d->m_codec = codec;
    this->d->m_rawCaps = rawCaps;
    this->d->m_bitrate = bitrate;
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();

    if (other.type() == AkCaps::CapsAudioCompressed) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    }
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCompressedCaps &other)
{
    this->d = new AkCompressedAudioCapsPrivate();

    if (other.type() == AkCompressedCaps::CapsType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    }
}

AkCompressedAudioCaps::AkCompressedAudioCaps(const AkCompressedAudioCaps &other):
    QObject()
{
    this->d = new AkCompressedAudioCapsPrivate();
    this->d->m_codec = other.d->m_codec;
    this->d->m_rawCaps = other.d->m_rawCaps;
    this->d->m_bitrate = other.d->m_bitrate;
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
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    } else {
        this->d->m_codec = AudioCodecID_unknown;
        this->d->m_rawCaps = AkAudioCaps();
        this->d->m_bitrate = AkAudioCaps::SampleFormat_unknown;
    }

    return *this;
}

AkCompressedAudioCaps &AkCompressedAudioCaps::operator =(const AkCompressedCaps &other)
{
    if (other.type() == AkCompressedCaps::CapsType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    } else {
        this->d->m_codec = AudioCodecID_unknown;
        this->d->m_rawCaps = AkAudioCaps();
        this->d->m_bitrate = AkAudioCaps::SampleFormat_unknown;
    }

    return *this;
}

AkCompressedAudioCaps &AkCompressedAudioCaps::operator =(const AkCompressedAudioCaps &other)
{
    if (this != &other) {
        this->d->m_codec = other.d->m_codec;
        this->d->m_rawCaps = other.d->m_rawCaps;
        this->d->m_bitrate = other.d->m_bitrate;
    }

    return *this;
}

bool AkCompressedAudioCaps::operator ==(const AkCompressedAudioCaps &other) const
{
    return this->d->m_codec == other.d->m_codec
            && this->d->m_rawCaps == other.d->m_rawCaps
            && this->d->m_bitrate == other.d->m_bitrate;
}

bool AkCompressedAudioCaps::operator !=(const AkCompressedAudioCaps &other) const
{
    return !(*this == other);
}

AkCompressedAudioCaps::operator bool() const
{
    return this->d->m_codec != AudioCodecID_unknown
           && this->d->m_rawCaps.channels() > 0
           && this->d->m_rawCaps.rate() > 0;
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
                                       const AkAudioCaps &rawCaps,
                                       int bitrate)
{
    return new AkCompressedAudioCaps(codec, rawCaps, bitrate);
}

QVariant AkCompressedAudioCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCompressedAudioCaps::AudioCodecID AkCompressedAudioCaps::codec() const
{
    return this->d->m_codec;
}

AkAudioCaps AkCompressedAudioCaps::rawCaps() const
{
    return this->d->m_rawCaps;
}

int AkCompressedAudioCaps::bitrate() const
{
    return this->d->m_bitrate;
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

void AkCompressedAudioCaps::setRawCaps(const AkAudioCaps &rawCaps)
{
    if (this->d->m_rawCaps == rawCaps)
        return;

    this->d->m_rawCaps = rawCaps;
    emit this->rawCapsChanged(rawCaps);
}

void AkCompressedAudioCaps::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkCompressedAudioCaps::resetCodec()
{
    this->setCodec(AudioCodecID_unknown);
}

void AkCompressedAudioCaps::resetRawCaps()
{
    this->setRawCaps(AkAudioCaps());
}

void AkCompressedAudioCaps::resetBitrate()
{
    this->setBitrate(0);
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
                    << ",rawCaps="
                    << caps.rawCaps()
                    << ",bitrate="
                    << caps.bitrate()
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
    AkAudioCaps rawCaps;
    istream >> rawCaps;
    caps.setRawCaps(rawCaps);
    int bitrate = 0;
    istream >> bitrate;
    caps.setBitrate(bitrate);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCompressedAudioCaps &caps)
{
    ostream << caps.codec();
    ostream << caps.rawCaps();
    ostream << caps.bitrate();

    return ostream;
}

#include "moc_akcompressedaudiocaps.cpp"
