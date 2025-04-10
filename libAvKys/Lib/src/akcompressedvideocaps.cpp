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

#include <QDataStream>
#include <QDebug>
#include <QMetaEnum>
#include <QQmlEngine>

#include "akcompressedvideocaps.h"
#include "akcaps.h"
#include "akcompressedcaps.h"
#include "akvideocaps.h"

class AkCompressedVideoCapsPrivate
{
    public:
        AkCompressedVideoCaps::VideoCodecID m_codec {AkCompressedVideoCaps::VideoCodecID_unknown};
        AkVideoCaps m_rawCaps;
        int m_bitrate {0};
};

AkCompressedVideoCaps::AkCompressedVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCompressedVideoCapsPrivate();
}

AkCompressedVideoCaps::AkCompressedVideoCaps(VideoCodecID codec,
                                             const AkVideoCaps &rawCaps,
                                             int bitrate):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();
    this->d->m_codec = codec;
    this->d->m_rawCaps = rawCaps;
    this->d->m_bitrate = bitrate;
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();

    if (other.type() == AkCaps::CapsVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    }
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const AkCompressedCaps &other)
{
    this->d = new AkCompressedVideoCapsPrivate();

    if (other.type() == AkCompressedCaps::CapsType_Video) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    }
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const AkCompressedVideoCaps &other):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();
    this->d->m_codec = other.d->m_codec;
    this->d->m_rawCaps = other.d->m_rawCaps;
    this->d->m_bitrate = other.d->m_bitrate;
}

AkCompressedVideoCaps::~AkCompressedVideoCaps()
{
    delete this->d;
}

AkCompressedVideoCaps &AkCompressedVideoCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    } else {
        this->d->m_codec = VideoCodecID_unknown;
        this->d->m_rawCaps = AkVideoCaps();
        this->d->m_bitrate = 0;
    }

    return *this;
}

AkCompressedVideoCaps &AkCompressedVideoCaps::operator =(const AkCompressedCaps &other)
{
    if (other.type() == AkCompressedCaps::CapsType_Video) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_codec = data->d->m_codec;
        this->d->m_rawCaps = data->d->m_rawCaps;
        this->d->m_bitrate = data->d->m_bitrate;
    } else {
        this->d->m_codec = VideoCodecID_unknown;
        this->d->m_rawCaps = AkVideoCaps();
        this->d->m_bitrate = 0;
    }

    return *this;
}

AkCompressedVideoCaps &AkCompressedVideoCaps::operator =(const AkCompressedVideoCaps &other)
{
    if (this != &other) {
        this->d->m_codec = other.d->m_codec;
        this->d->m_rawCaps = other.d->m_rawCaps;
        this->d->m_bitrate = other.d->m_bitrate;
    }

    return *this;
}

bool AkCompressedVideoCaps::operator ==(const AkCompressedVideoCaps &other) const
{
    return this->d->m_codec == other.d->m_codec
            && this->d->m_rawCaps == other.d->m_rawCaps
            && this->d->m_bitrate == other.d->m_bitrate;
}

bool AkCompressedVideoCaps::operator !=(const AkCompressedVideoCaps &other) const
{
    return !(*this == other);
}

AkCompressedVideoCaps::operator bool() const
{
    return this->d->m_codec != VideoCodecID_unknown
           && this->d->m_rawCaps.width() > 0
           && this->d->m_rawCaps.height() > 0;
}

AkCompressedVideoCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsVideoCompressed);
    caps.setPrivateData(new AkCompressedVideoCaps(*this),
                        [] (void *data) -> void * {
                            return new AkCompressedVideoCaps(*reinterpret_cast<AkCompressedVideoCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkCompressedVideoCaps *>(data);
                        });

    return caps;
}

AkCompressedVideoCaps::operator AkCompressedCaps() const
{
    AkCompressedCaps caps;
    caps.setType(AkCompressedCaps::CapsType_Video);
    caps.setCodecID(this->d->m_codec);
    caps.setPrivateData(new AkCompressedVideoCaps(*this),
                        [] (void *data) -> void * {
                            return new AkCompressedVideoCaps(*reinterpret_cast<AkCompressedVideoCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkCompressedVideoCaps *>(data);
                        });

    return caps;
}

QObject *AkCompressedVideoCaps::create()
{
    return new AkCompressedVideoCaps();
}

QObject *AkCompressedVideoCaps::create(const AkCaps &caps)
{
    return new AkCompressedVideoCaps(caps);
}

QObject *AkCompressedVideoCaps::create(const AkCompressedCaps &caps)
{
    return new AkCompressedVideoCaps(caps);
}

QObject *AkCompressedVideoCaps::create(const AkCompressedVideoCaps &caps)
{
    return new AkCompressedVideoCaps(caps);
}

QObject *AkCompressedVideoCaps::create(VideoCodecID codec,
                                       const AkVideoCaps &rawCaps,
                                       int bitrate)
{
    return new AkCompressedVideoCaps(codec, rawCaps, bitrate);
}

QVariant AkCompressedVideoCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCompressedVideoCaps::VideoCodecID AkCompressedVideoCaps::codec() const
{
    return this->d->m_codec;
}

AkVideoCaps AkCompressedVideoCaps::rawCaps() const
{
    return this->d->m_rawCaps;
}

int AkCompressedVideoCaps::bitrate() const
{
    return this->d->m_bitrate;
}

QString AkCompressedVideoCaps::videoCodecIDToString(VideoCodecID codecID)
{
    AkCompressedVideoCaps caps;
    int codecIndex = caps.metaObject()->indexOfEnumerator("VideoCodecID");
    QMetaEnum codecEnum = caps.metaObject()->enumerator(codecIndex);
    QString codec(codecEnum.valueToKey(codecID));

    if (codec.isEmpty()) {
        QString str;
        QTextStream ss(&str, QIODeviceBase::WriteOnly);
        ss << "VideoCodecID(";
        auto codecIDBytes = reinterpret_cast<char *>(&codecID);

        for (qsizetype i = 0; i < sizeof(VideoCodecID); ++i) {
            if (i > 0)
                ss << ' ';

            if (codecIDBytes[i] >= 32 && codecIDBytes[i] < 127)
                ss << "'" << codecIDBytes[i] << "'";
            else
                ss << (quint32(codecIDBytes[i]) & 0xff);
        }

        ss << ")";

        return str;
    }

    codec.remove("VideoCodecID_");

    return codec;
}

void AkCompressedVideoCaps::setCodec(VideoCodecID codec)
{
    if (this->d->m_codec == codec)
        return;

    this->d->m_codec = codec;
    emit this->codecChanged(codec);
}

void AkCompressedVideoCaps::setRawCaps(const AkVideoCaps &rawCaps)
{
    if (this->d->m_rawCaps == rawCaps)
        return;

    this->d->m_rawCaps = rawCaps;
    emit this->rawCapsChanged(rawCaps);
}

void AkCompressedVideoCaps::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkCompressedVideoCaps::resetCodec()
{
    this->setCodec(VideoCodecID_unknown);
}

void AkCompressedVideoCaps::resetRawCaps()
{
    this->setRawCaps(AkVideoCaps());
}

void AkCompressedVideoCaps::resetBitrate()
{
    this->setBitrate(0);
}

void AkCompressedVideoCaps::registerTypes()
{
    qRegisterMetaType<AkCompressedVideoCaps>("AkCompressedVideoCaps");
    qRegisterMetaType<VideoCodecID>("AkVideoCodecID");
    qmlRegisterSingletonType<AkCompressedVideoCaps>("Ak", 1, 0, "AkCompressedVideoCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedVideoCaps();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedVideoCaps &caps)
{
    debug.nospace() << "AkCompressedVideoCaps("
                    << "codec="
                    << caps.codec()
                    << ",rawCaps="
                    << caps.rawCaps()
                    << ",bitrate="
                    << caps.bitrate()
                    << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkCompressedVideoCaps::VideoCodecID codecID)
{
    debug.nospace() << AkCompressedVideoCaps::videoCodecIDToString(codecID).toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCompressedVideoCaps &caps)
{
    AkCompressedVideoCaps::VideoCodecID codec;
    istream >> codec;
    caps.setCodec(codec);
    AkVideoCaps rawCaps;
    istream >> rawCaps;
    caps.setRawCaps(rawCaps);
    int bitrate = 0;
    istream >> bitrate;
    caps.setBitrate(bitrate);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream,
                         const AkCompressedVideoCaps &caps)
{
    ostream << caps.codec();
    ostream << caps.rawCaps();
    ostream << caps.bitrate();

    return ostream;
}

bool operator <(const AkCompressedVideoCaps &caps1,
                const AkCompressedVideoCaps &caps2)
{
    if (caps1.d->m_codec < caps2.d->m_codec)
        return true;
    else if (caps1.d->m_codec > caps2.d->m_codec)
        return false;

    if (caps1.d->m_rawCaps < caps2.d->m_rawCaps)
        return true;
    else if (caps1.d->m_rawCaps > caps2.d->m_rawCaps)
        return false;

    return caps1.d->m_bitrate < caps2.d->m_bitrate;
}

#include "moc_akcompressedvideocaps.cpp"
