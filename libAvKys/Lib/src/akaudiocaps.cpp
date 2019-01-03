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
#include <QVector>

#include "akaudiocaps.h"
#include "akcaps.h"

class SampleFormats
{
    public:
        AkAudioCaps::SampleFormat format;
        AkAudioCaps::SampleType type;
        int bps;
        int endianness;
        bool planar;

        static inline const QVector<SampleFormats> &formats()
        {
            static const QVector<SampleFormats> sampleFormats = {
                {AkAudioCaps::SampleFormat_none , AkAudioCaps::SampleType_unknown,  0, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s8   , AkAudioCaps::SampleType_int    ,  8, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_u8   , AkAudioCaps::SampleType_uint   ,  8, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s16  , AkAudioCaps::SampleType_int    , 16, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s16le, AkAudioCaps::SampleType_int    , 16, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_s16be, AkAudioCaps::SampleType_int    , 16, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_u16  , AkAudioCaps::SampleType_uint   , 16, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_u16le, AkAudioCaps::SampleType_uint   , 16, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_u16be, AkAudioCaps::SampleType_uint   , 16, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_s24  , AkAudioCaps::SampleType_int    , 24, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s24le, AkAudioCaps::SampleType_int    , 24, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_s24be, AkAudioCaps::SampleType_int    , 24, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_u24  , AkAudioCaps::SampleType_uint   , 24, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_u24le, AkAudioCaps::SampleType_uint   , 24, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_u24be, AkAudioCaps::SampleType_uint   , 24, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_s32  , AkAudioCaps::SampleType_int    , 32, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s32le, AkAudioCaps::SampleType_int    , 32, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_s32be, AkAudioCaps::SampleType_int    , 32, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_u32  , AkAudioCaps::SampleType_uint   , 32, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_u32le, AkAudioCaps::SampleType_uint   , 32, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_u32be, AkAudioCaps::SampleType_uint   , 32, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_s64  , AkAudioCaps::SampleType_int    , 64, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_s64le, AkAudioCaps::SampleType_int    , 64, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_s64be, AkAudioCaps::SampleType_int    , 64, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_u64  , AkAudioCaps::SampleType_uint   , 64, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_u64le, AkAudioCaps::SampleType_uint   , 64, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_u64be, AkAudioCaps::SampleType_uint   , 64, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_flt  , AkAudioCaps::SampleType_float  , 32, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_fltle, AkAudioCaps::SampleType_float  , 32, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_fltbe, AkAudioCaps::SampleType_float  , 32, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_dbl  , AkAudioCaps::SampleType_float  , 64, Q_BYTE_ORDER   , false},
                {AkAudioCaps::SampleFormat_dblle, AkAudioCaps::SampleType_float  , 64, Q_LITTLE_ENDIAN, false},
                {AkAudioCaps::SampleFormat_dblbe, AkAudioCaps::SampleType_float  , 64, Q_BIG_ENDIAN   , false},
                {AkAudioCaps::SampleFormat_u8p  , AkAudioCaps::SampleType_uint   ,  8, Q_BYTE_ORDER   ,  true},
                {AkAudioCaps::SampleFormat_s16p , AkAudioCaps::SampleType_int    , 16, Q_BYTE_ORDER   ,  true},
                {AkAudioCaps::SampleFormat_s32p , AkAudioCaps::SampleType_int    , 32, Q_BYTE_ORDER   ,  true},
                {AkAudioCaps::SampleFormat_s64p , AkAudioCaps::SampleType_int    , 64, Q_BYTE_ORDER   ,  true},
                {AkAudioCaps::SampleFormat_fltp , AkAudioCaps::SampleType_float  , 32, Q_BYTE_ORDER   ,  true},
                {AkAudioCaps::SampleFormat_dblp , AkAudioCaps::SampleType_float  , 64, Q_BYTE_ORDER   ,  true},
            };

            return sampleFormats;
        }

        static inline const SampleFormats *byFormat(AkAudioCaps::SampleFormat format)
        {
            for (auto &format_: formats())
                if (format_.format == format)
                    return &format_;

            return &formats().front();
        }

        static inline const SampleFormats *byType(AkAudioCaps::SampleType type)
        {
            for (auto &format: formats())
                if (format.type == type)
                    return &format;

            return &formats().front();
        }

        static inline const SampleFormats *byBps(int bps)
        {
            for (auto &format: formats())
                if (format.bps == bps)
                    return &format;

            return &formats().front();
        }

        static inline const SampleFormats *byEndianness(int endianness)
        {
            for (auto &format: formats())
                if (format.endianness == endianness)
                    return &format;

            return &formats().front();
        }

        static inline const SampleFormats *byPlanar(bool planar)
        {
            for (auto &format: formats())
                if (format.planar == planar)
                    return &format;

            return &formats().front();
        }
};

class ChannelLayouts
{
    public:
        AkAudioCaps::ChannelLayout layout;
        int channels;
        QString description;

        static inline const QVector<ChannelLayouts> &layouts()
        {
            static const QVector<ChannelLayouts> channelLayouts = {
                {AkAudioCaps::Layout_none         ,  0, "none"          },
                {AkAudioCaps::Layout_mono         ,  1, "mono"          },
                {AkAudioCaps::Layout_stereo       ,  2, "stereo"        },
                {AkAudioCaps::Layout_2p1          ,  3, "2.1"           },
                {AkAudioCaps::Layout_3p0          ,  3, "3.0"           },
                {AkAudioCaps::Layout_3p0_back     ,  3, "3.0(back)"     },
                {AkAudioCaps::Layout_3p1          ,  4, "3.1"           },
                {AkAudioCaps::Layout_4p0          ,  4, "4.0"           },
                {AkAudioCaps::Layout_quad         ,  4, "quad"          },
                {AkAudioCaps::Layout_quad_side    ,  4, "quad(side)"    },
                {AkAudioCaps::Layout_4p1          ,  5, "4.1"           },
                {AkAudioCaps::Layout_5p0          ,  5, "5.0"           },
                {AkAudioCaps::Layout_5p0_side     ,  5, "5.0(side)"     },
                {AkAudioCaps::Layout_5p1          ,  6, "5.1"           },
                {AkAudioCaps::Layout_5p1_side     ,  6, "5.1(side)"     },
                {AkAudioCaps::Layout_6p0          ,  6, "6.0"           },
                {AkAudioCaps::Layout_6p0_front    ,  6, "6.0(front)"    },
                {AkAudioCaps::Layout_hexagonal    ,  6, "hexagonal"     },
                {AkAudioCaps::Layout_6p1          ,  7, "6.1"           },
                {AkAudioCaps::Layout_6p1_front    ,  7, "6.1(back)"     },
                {AkAudioCaps::Layout_6p1_front    ,  7, "6.1(front)"    },
                {AkAudioCaps::Layout_7p0          ,  7, "7.0"           },
                {AkAudioCaps::Layout_7p0_front    ,  7, "7.0(front)"    },
                {AkAudioCaps::Layout_7p1          ,  8, "7.1"           },
                {AkAudioCaps::Layout_7p1_wide     ,  8, "7.1(wide)"     },
                {AkAudioCaps::Layout_7p1_wide_side,  8, "7.1(wide-side)"},
                {AkAudioCaps::Layout_octagonal    ,  8, "octagonal"     },
                {AkAudioCaps::Layout_hexadecagonal, 16, "hexadecagonal" },
                {AkAudioCaps::Layout_downmix      ,  2, "downmix"       },
            };

            return channelLayouts;
        }

        static inline const ChannelLayouts *byLayout(AkAudioCaps::ChannelLayout layout)
        {
            for (auto &layout_: layouts())
                if (layout_.layout == layout)
                    return &layout_;

            return &layouts().front();
        }

        static inline const ChannelLayouts *byChannels(int channels)
        {
            for (auto &layout: layouts())
                if (layout.channels == channels)
                    return &layout;

            return &layouts().front();
        }

        static inline const ChannelLayouts *byDescription(const QString &description)
        {
            for (auto &layout: layouts())
                if (layout.description == description)
                    return &layout;

            return &layouts().front();
        }
};

class AkAudioCapsPrivate
{
    public:
        AkAudioCaps::SampleFormat m_format {AkAudioCaps::SampleFormat_none};
        int m_bps {0};
        int m_channels {0};
        int m_rate {0};
        AkAudioCaps::ChannelLayout m_layout {AkAudioCaps::Layout_none};
        int m_samples {false};
        int m_align {0};
        bool m_isValid {false};
};

AkAudioCaps::AkAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioCapsPrivate();
}

AkAudioCaps::AkAudioCaps(const QVariantMap &caps)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format != SampleFormat_none;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toInt();
}

AkAudioCaps::AkAudioCaps(const QString &caps)
{
    this->d = new AkAudioCapsPrivate();
    *this = caps;
}

AkAudioCaps::AkAudioCaps(const AkCaps &caps)
{
    this->d = new AkAudioCapsPrivate();

    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        this->d->m_format =
                AkAudioCaps::sampleFormatFromString(caps.property("format").toString());
        this->d->m_bps = caps.property("bps").toInt();
        this->d->m_channels = caps.property("channels").toInt();
        this->d->m_rate = caps.property("rate").toInt();

        QString layout = caps.property("layout").toString();
        this->d->m_layout = ChannelLayouts::byDescription(layout)->layout;

        this->d->m_samples = caps.property("samples").toInt();
        this->d->m_align = caps.property("align").toInt();
    }
}

AkAudioCaps::AkAudioCaps(const AkAudioCaps &other):
    QObject()
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_bps = other.d->m_bps;
    this->d->m_channels = other.d->m_channels;
    this->d->m_rate = other.d->m_rate;
    this->d->m_layout = other.d->m_layout;
    this->d->m_samples = other.d->m_samples;
    this->d->m_align = other.d->m_align;
}

AkAudioCaps::AkAudioCaps(AkAudioCaps::SampleFormat format,
                         int channels,
                         int rate)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_format = format;
    this->d->m_isValid = this->d->m_format != SampleFormat_none;
    this->d->m_bps = AkAudioCaps::bitsPerSample(format);
    this->d->m_channels = channels;
    this->d->m_rate = rate;
    this->d->m_layout = AkAudioCaps::defaultChannelLayout(channels);
}

AkAudioCaps::~AkAudioCaps()
{
    delete this->d;
}

AkAudioCaps &AkAudioCaps::operator =(const AkAudioCaps &other)
{
    if (this != &other) {
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_format = other.d->m_format;
        this->d->m_bps = other.d->m_bps;
        this->d->m_channels = other.d->m_channels;
        this->d->m_rate = other.d->m_rate;
        this->d->m_layout = other.d->m_layout;
        this->d->m_samples = other.d->m_samples;
        this->d->m_align = other.d->m_align;
    }

    return *this;
}

AkAudioCaps &AkAudioCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        this->d->m_format =
                AkAudioCaps::sampleFormatFromString(caps.property("format").toString());
        this->d->m_bps = caps.property("bps").toInt();
        this->d->m_channels = caps.property("channels").toInt();
        this->d->m_rate = caps.property("rate").toInt();

        QString layout = caps.property("layout").toString();
        this->d->m_layout = ChannelLayouts::byDescription(layout)->layout;

        this->d->m_samples = caps.property("samples").toInt();
        this->d->m_align = caps.property("align").toInt();
    } else {
        this->d->m_isValid = false;
        this->d->m_format = SampleFormat_none;
        this->d->m_bps = 0;
        this->d->m_channels = 0;
        this->d->m_rate = 0;
        this->d->m_layout = Layout_none;
        this->d->m_samples = 0;
        this->d->m_align = 0;
    }

    return *this;
}

AkAudioCaps &AkAudioCaps::operator =(const QString &caps)
{
    this->operator =(AkCaps(caps));

    return *this;
}

bool AkAudioCaps::operator ==(const AkAudioCaps &other) const
{
    return this->d->m_isValid == other.d->m_isValid
        && this->d->m_format == other.d->m_format
        && this->d->m_bps == other.d->m_bps
        && this->d->m_channels == other.d->m_channels
        && this->d->m_rate == other.d->m_rate
        && this->d->m_layout == other.d->m_layout
        && this->d->m_samples == other.d->m_samples
        && this->d->m_align == other.d->m_align;
}

bool AkAudioCaps::operator !=(const AkAudioCaps &other) const
{
    return !(*this == other);
}

AkAudioCaps::operator AkCaps() const
{
    return this->toCaps();
}

bool AkAudioCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &AkAudioCaps::isValid()
{
    return this->d->m_isValid;
}

AkAudioCaps::SampleFormat AkAudioCaps::format() const
{
    return this->d->m_format;
}

AkAudioCaps::SampleFormat &AkAudioCaps::format()
{
    return this->d->m_format;
}

int AkAudioCaps::bps() const
{
    return this->d->m_bps;
}

int &AkAudioCaps::bps()
{
    return this->d->m_bps;
}

int AkAudioCaps::channels() const
{
    return this->d->m_channels;
}

int &AkAudioCaps::channels()
{
    return this->d->m_channels;
}

int AkAudioCaps::rate() const
{
    return this->d->m_rate;
}

int &AkAudioCaps::rate()
{
    return this->d->m_rate;
}

AkAudioCaps::ChannelLayout AkAudioCaps::layout() const
{
    return this->d->m_layout;
}

AkAudioCaps::ChannelLayout &AkAudioCaps::layout()
{
    return this->d->m_layout;
}

int AkAudioCaps::samples() const
{
    return this->d->m_samples;
}

int &AkAudioCaps::samples()
{
    return this->d->m_samples;
}

int AkAudioCaps::align() const
{
    return this->d->m_align;
}

int &AkAudioCaps::align()
{
    return this->d->m_align;
}

AkAudioCaps &AkAudioCaps::fromMap(const QVariantMap &caps)
{
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format != AkAudioCaps::SampleFormat_none;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toInt();

    return *this;
}

AkAudioCaps::operator bool() const
{
    return this->d->m_isValid;
}

AkAudioCaps &AkAudioCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap AkAudioCaps::toMap() const
{
    return QVariantMap {
        {"format"  , this->d->m_format  },
        {"bps"     , this->d->m_bps     },
        {"channels", this->d->m_channels},
        {"rate"    , this->d->m_rate    },
        {"layout"  , this->d->m_layout  },
        {"samples" , this->d->m_samples },
        {"align"   , this->d->m_align   }
    };
}

QString AkAudioCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString sampleFormat = AkAudioCaps::sampleFormatToString(this->d->m_format);
    QString layout = ChannelLayouts::byLayout(this->d->m_layout)->description;

    return QString("audio/x-raw,"
                   "format=%1,"
                   "bps=%2,"
                   "channels=%3,"
                   "rate=%4,"
                   "layout=%5,"
                   "samples=%6,"
                   "align=%7").arg(sampleFormat)
                              .arg(this->d->m_bps)
                              .arg(this->d->m_channels)
                              .arg(this->d->m_rate)
                              .arg(layout)
                              .arg(this->d->m_samples)
            .arg(this->d->m_align);
}

AkAudioCaps &AkAudioCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "audio/x-raw")
        return *this;

    if (caps.contains("format"))
        this->d->m_format =
            AkAudioCaps::sampleFormatFromString(caps.property("format").toString());

    if (caps.contains("bps"))
        this->d->m_bps = caps.property("bps").toInt();

    if (caps.contains("channels"))
        this->d->m_channels = caps.property("channels").toInt();

    if (caps.contains("rate"))
        this->d->m_rate = caps.property("rate").toInt();

    if (caps.contains("layout")) {
        QString layout = caps.property("layout").toString();
        this->d->m_layout = ChannelLayouts::byDescription(layout)->layout;
    }

    if (caps.contains("samples"))
        this->d->m_samples = caps.property("samples").toInt();

    if (caps.contains("align"))
        this->d->m_align = caps.property("align").toInt();

    return *this;
}

AkCaps AkAudioCaps::toCaps() const
{
    return AkCaps(this->toString());
}

int AkAudioCaps::bitsPerSample(AkAudioCaps::SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->bps;
}

int AkAudioCaps::bitsPerSample(const QString &sampleFormat)
{
    return AkAudioCaps::bitsPerSample(AkAudioCaps::sampleFormatFromString(sampleFormat));
}

QString AkAudioCaps::sampleFormatToString(AkAudioCaps::SampleFormat sampleFormat)
{
    AkAudioCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(sampleFormat));
    format.remove("SampleFormat_");

    return format;
}

AkAudioCaps::SampleFormat AkAudioCaps::sampleFormatFromString(const QString &sampleFormat)
{
    AkAudioCaps caps;
    QString format = "SampleFormat_" + sampleFormat;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    int formatInt = formatEnum.keyToValue(format.toStdString().c_str());

    return static_cast<SampleFormat>(formatInt);
}

AkAudioCaps::SampleFormat AkAudioCaps::sampleFormatFromProperties(AkAudioCaps::SampleType type,
                                                                  int bps,
                                                                  int endianness,
                                                                  bool planar)
{
    for (const SampleFormats &sampleFormat: SampleFormats::formats())
        if (sampleFormat.type == type
            && sampleFormat.bps == bps
            && sampleFormat.endianness == endianness
            && sampleFormat.planar == planar) {
            return sampleFormat.format;
        }

    return AkAudioCaps::SampleFormat_none;
}

bool AkAudioCaps::sampleFormatProperties(AkAudioCaps::SampleFormat sampleFormat,
                                         AkAudioCaps::SampleType *type,
                                         int *bps,
                                         int *endianness,
                                         bool *planar)
{
    auto format = SampleFormats::byFormat(sampleFormat);

    if (!format)
        return false;

    if (type)
        *type = format->type;

    if (bps)
        *bps = format->bps;

    if (endianness)
        *endianness = format->endianness;

    if (planar)
        *planar = format->planar;

    return true;
}

bool AkAudioCaps::sampleFormatProperties(const QString &sampleFormat,
                                         AkAudioCaps::SampleType *type,
                                         int *bps,
                                         int *endianness,
                                         bool *planar)
{
    return AkAudioCaps::sampleFormatProperties(AkAudioCaps::sampleFormatFromString(sampleFormat),
                                               type,
                                               bps,
                                               endianness,
                                               planar);
}

AkAudioCaps::SampleType AkAudioCaps::sampleType(AkAudioCaps::SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->type;
}

AkAudioCaps::SampleType AkAudioCaps::sampleType(const QString &sampleFormat)
{
    return AkAudioCaps::sampleType(AkAudioCaps::sampleFormatFromString(sampleFormat));
}

QString AkAudioCaps::channelLayoutToString(AkAudioCaps::ChannelLayout channelLayout)
{
    return ChannelLayouts::byLayout(channelLayout)->description;
}

AkAudioCaps::ChannelLayout AkAudioCaps::channelLayoutFromString(const QString &channelLayout)
{
    return ChannelLayouts::byDescription(channelLayout)->layout;
}

int AkAudioCaps::channelCount(AkAudioCaps::ChannelLayout channelLayout)
{
    return ChannelLayouts::byLayout(channelLayout)->channels;
}

int AkAudioCaps::channelCount(const QString &channelLayout)
{
    return ChannelLayouts::byDescription(channelLayout)->channels;
}

int AkAudioCaps::endianness(AkAudioCaps::SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->endianness;
}

int AkAudioCaps::endianness(const QString &sampleFormat)
{
    return AkAudioCaps::endianness(AkAudioCaps::sampleFormatFromString(sampleFormat));
}

bool AkAudioCaps::isPlanar(AkAudioCaps::SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->planar;
}

bool AkAudioCaps::isPlanar(const QString &sampleFormat)
{
    return AkAudioCaps::isPlanar(AkAudioCaps::sampleFormatFromString(sampleFormat));
}

AkAudioCaps::ChannelLayout AkAudioCaps::defaultChannelLayout(int channelCount)
{
    return ChannelLayouts::byChannels(channelCount)->layout;
}

QString AkAudioCaps::defaultChannelLayoutString(int channelCount)
{
    return ChannelLayouts::byChannels(channelCount)->description;
}

void AkAudioCaps::setFormat(AkAudioCaps::SampleFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkAudioCaps::setBps(int bps)
{
    if (this->d->m_bps == bps)
        return;

    this->d->m_bps = bps;
    emit this->bpsChanged(bps);
}

void AkAudioCaps::setChannels(int channels)
{
    if (this->d->m_channels == channels)
        return;

    this->d->m_channels = channels;
    emit this->channelsChanged(channels);
}

void AkAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void AkAudioCaps::setLayout(AkAudioCaps::ChannelLayout layout)
{
    if (this->d->m_layout == layout)
        return;

    this->d->m_layout = layout;
    emit this->layoutChanged(layout);
}

void AkAudioCaps::setSamples(int samples)
{
    if (this->d->m_samples == samples)
        return;

    this->d->m_samples = samples;
    emit this->samplesChanged(samples);
}

void AkAudioCaps::setAlign(int align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    emit this->alignChanged(align);
}

void AkAudioCaps::resetFormat()
{
    this->setFormat(SampleFormat_none);
}

void AkAudioCaps::resetBps()
{
    this->setBps(0);
}

void AkAudioCaps::resetChannels()
{
    this->setChannels(0);
}

void AkAudioCaps::resetRate()
{
    this->setRate(0);
}

void AkAudioCaps::resetLayout()
{
    this->setLayout(Layout_none);
}

void AkAudioCaps::resetSamples()
{
    this->setSamples(0);
}

void AkAudioCaps::resetAlign()
{
    this->setAlign(0);
}

QDebug operator <<(QDebug debug, const AkAudioCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}

#include "moc_akaudiocaps.cpp"
