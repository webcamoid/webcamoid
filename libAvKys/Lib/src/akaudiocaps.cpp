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

        static inline const QVector<SampleFormats> &formats()
        {
            static const QVector<SampleFormats> sampleFormats = {
                {AkAudioCaps::SampleFormat_none , AkAudioCaps::SampleType_unknown,  0, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_s8   , AkAudioCaps::SampleType_int    ,  8, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_u8   , AkAudioCaps::SampleType_uint   ,  8, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_s16le, AkAudioCaps::SampleType_int    , 16, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_s16be, AkAudioCaps::SampleType_int    , 16, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_u16le, AkAudioCaps::SampleType_uint   , 16, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_u16be, AkAudioCaps::SampleType_uint   , 16, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_s32le, AkAudioCaps::SampleType_int    , 32, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_s32be, AkAudioCaps::SampleType_int    , 32, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_u32le, AkAudioCaps::SampleType_uint   , 32, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_u32be, AkAudioCaps::SampleType_uint   , 32, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_s64le, AkAudioCaps::SampleType_int    , 64, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_s64be, AkAudioCaps::SampleType_int    , 64, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_u64le, AkAudioCaps::SampleType_uint   , 64, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_u64be, AkAudioCaps::SampleType_uint   , 64, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_fltle, AkAudioCaps::SampleType_float  , 32, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_fltbe, AkAudioCaps::SampleType_float  , 32, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_dblle, AkAudioCaps::SampleType_float  , 64, Q_LITTLE_ENDIAN},
                {AkAudioCaps::SampleFormat_dblbe, AkAudioCaps::SampleType_float  , 64, Q_BIG_ENDIAN   },
                {AkAudioCaps::SampleFormat_s16  , AkAudioCaps::SampleType_int    , 16, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_u16  , AkAudioCaps::SampleType_uint   , 16, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_s32  , AkAudioCaps::SampleType_int    , 32, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_u32  , AkAudioCaps::SampleType_uint   , 32, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_s64  , AkAudioCaps::SampleType_int    , 64, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_u64  , AkAudioCaps::SampleType_uint   , 64, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_flt  , AkAudioCaps::SampleType_float  , 32, Q_BYTE_ORDER   },
                {AkAudioCaps::SampleFormat_dbl  , AkAudioCaps::SampleType_float  , 64, Q_BYTE_ORDER   },
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

        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
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
                {AkAudioCaps::Layout_6p1_back     ,  7, "6.1(back)"     },
                {AkAudioCaps::Layout_6p1_front    ,  7, "6.1(front)"    },
                {AkAudioCaps::Layout_7p0          ,  7, "7.0"           },
                {AkAudioCaps::Layout_7p0_front    ,  7, "7.0(front)"    },
                {AkAudioCaps::Layout_7p1          ,  8, "7.1"           },
                {AkAudioCaps::Layout_7p1_wide     ,  8, "7.1(wide)"     },
                {AkAudioCaps::Layout_7p1_wide_back,  8, "7.1(wide-back)"},
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
        AkAudioCaps::ChannelLayout m_layout {AkAudioCaps::Layout_none};
        size_t m_planeSize {0};
        QVector<size_t> m_offset;
        int m_rate {0};
        int m_samples {false};
        int m_align {1};
        bool m_planar {false};
        bool m_isValid {false};

        void updateParams();
};

AkAudioCaps::AkAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioCapsPrivate();
}

AkAudioCaps::AkAudioCaps(AkAudioCaps::SampleFormat format,
                         ChannelLayout layout,
                         int rate,
                         int samples,
                         bool planar,
                         int align)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = format != SampleFormat_none;
    this->d->m_format = format;
    this->d->m_layout = layout;
    this->d->m_rate = rate;
    this->d->m_samples = samples;
    this->d->m_planar = planar;
    this->d->m_align = align;
    this->d->updateParams();
}

AkAudioCaps::AkAudioCaps(const QVariantMap &caps)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = true;
    this->fromMap(caps);
}

AkAudioCaps::AkAudioCaps(const QString &caps)
{
    this->d = new AkAudioCapsPrivate();
    this->fromString(caps);
}

AkAudioCaps::AkAudioCaps(const AkCaps &caps)
{
    this->d = new AkAudioCapsPrivate();

    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    }
}

AkAudioCaps::AkAudioCaps(const AkAudioCaps &other):
    QObject()
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_layout = other.d->m_layout;
    this->d->m_rate = other.d->m_rate;
    this->d->m_samples = other.d->m_samples;
    this->d->m_planar = other.d->m_planar;
    this->d->m_align = other.d->m_align;
    this->d->m_planeSize = other.d->m_planeSize;
    this->d->m_offset = other.d->m_offset;

    for (auto &property: other.dynamicPropertyNames())
        this->setProperty(property, other.property(property));
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
        this->d->m_layout = other.d->m_layout;
        this->d->m_rate = other.d->m_rate;
        this->d->m_samples = other.d->m_samples;
        this->d->m_planar = other.d->m_planar;
        this->d->m_align = other.d->m_align;
        this->d->m_planeSize = other.d->m_planeSize;
        this->d->m_offset = other.d->m_offset;

        this->clear();

        for (auto &property: other.dynamicPropertyNames())
            this->setProperty(property, other.property(property));

        this->d->updateParams();
    }

    return *this;
}

AkAudioCaps &AkAudioCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    } else {
        this->d->m_isValid = false;
        this->d->m_format = SampleFormat_none;
        this->d->m_layout = Layout_none;
        this->d->m_rate = 0;
        this->d->m_samples = 0;
        this->d->m_planar = false;
        this->d->m_align = 1;
        this->d->m_planeSize = 1;
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
    if (this->dynamicPropertyNames() != other.dynamicPropertyNames())
        return false;

    for (auto &property: this->dynamicPropertyNames())
        if (this->property(property) != other.property(property))
            return false;

    return this->d->m_format == other.d->m_format
            && this->d->m_layout == other.d->m_layout
            && this->d->m_rate == other.d->m_rate
            && this->d->m_samples == other.d->m_samples
            && this->d->m_planar == other.d->m_planar
            && this->d->m_align == other.d->m_align;
}

bool AkAudioCaps::operator !=(const AkAudioCaps &other) const
{
    return !(*this == other);
}

AkAudioCaps::operator bool() const
{
    return this->d->m_isValid;
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

AkAudioCaps::ChannelLayout AkAudioCaps::layout() const
{
    return this->d->m_layout;
}

int AkAudioCaps::bps() const
{
    return AkAudioCaps::bitsPerSample(this->d->m_format);
}

int AkAudioCaps::channels() const
{
    return AkAudioCaps::channelCount(this->d->m_layout);
}

int AkAudioCaps::rate() const
{
    return this->d->m_rate;
}

int &AkAudioCaps::rate()
{
    return this->d->m_rate;
}

int AkAudioCaps::samples() const
{
    return this->d->m_samples;
}

bool AkAudioCaps::planar() const
{
    return this->d->m_planar;
}

int AkAudioCaps::align() const
{
    return this->d->m_align;
}

size_t AkAudioCaps::frameSize() const
{
    auto af = SampleFormats::byFormat(this->d->m_format);

    if (!af)
        return 0;

    return size_t(this->d->m_planar? this->channels(): 1)
            * this->d->m_planeSize;
}

AkAudioCaps &AkAudioCaps::fromMap(const QVariantMap &caps)
{
    for (auto &property: this->dynamicPropertyNames())
        this->setProperty(property, QVariant());

    if (!caps.contains("mimeType")) {
        this->d->m_isValid = false;

        return *this;
    }

    for (auto it = caps.begin(); it != caps.end(); it++)
        if (it.key() == "mimeType") {
            this->d->m_isValid = it.value().toString() == "audio/x-raw";

            if (!this->d->m_isValid)
                return *this;
        } else {
            this->setProperty(it.key().trimmed().toStdString().c_str(),
                              it.value());
        }

    this->d->updateParams();

    return *this;
}

AkAudioCaps &AkAudioCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap AkAudioCaps::toMap() const
{
    QVariantMap map {
        {"format"  , this->d->m_format },
        {"layout"  , this->d->m_layout },
        {"bps"     , this->bps()       },
        {"channels", this->channels()  },
        {"rate"    , this->d->m_rate   },
        {"samples" , this->d->m_samples},
        {"planar"  , this->d->m_planar },
        {"align"   , this->d->m_align  }
    };

    for (auto &property: this->dynamicPropertyNames()) {
        auto key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

QString AkAudioCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString sampleFormat = AkAudioCaps::sampleFormatToString(this->d->m_format);
    QString layout = ChannelLayouts::byLayout(this->d->m_layout)->description;

    auto caps = QString("audio/x-raw,"
                        "format=%1,"
                        "layout=%2,"
                        "bps=%3,"
                        "channels=%4,"
                        "rate=%5,"
                        "samples=%6,"
                        "planar=%7,"
                        "align=%8").arg(sampleFormat)
                                   .arg(layout)
                                   .arg(this->bps())
                                   .arg(this->channels())
                                   .arg(this->d->m_rate)
                                   .arg(this->d->m_samples)
                                   .arg(this->d->m_planar)
                                   .arg(this->d->m_align);

    QStringList properties;

    for (auto &property: this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (auto &property: properties)
        caps.append(QString(",%1=%2").arg(property,
                                          this->property(property.toStdString().c_str()).toString()));

    return caps;
}

AkAudioCaps &AkAudioCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "audio/x-raw")
        return *this;

    this->clear();

    for (auto &property: caps.dynamicPropertyNames()) {
        int i = this->metaObject()->indexOfProperty(property);

        if (this->metaObject()->property(i).isWritable()) {
            auto value = caps.property(property);

            if (property == "format")
                value = AkAudioCaps::sampleFormatFromString(value.toString());
            else if (property == "layout")
                value = AkAudioCaps::channelLayoutFromString(value.toString());

            this->setProperty(property, value);
        }
    }

    this->d->updateParams();

    return *this;
}

AkCaps AkAudioCaps::toCaps() const
{
    return AkCaps(this->toString());
}

size_t AkAudioCaps::planeOffset(int plane) const
{
    return this->d->m_offset[plane];
}

int AkAudioCaps::planes() const
{
    auto af = SampleFormats::byFormat(this->d->m_format);
    auto layouts = ChannelLayouts::byLayout(this->d->m_layout);

    if (!af || !layouts)
        return 0;

    return this->d->m_planar? layouts->channels: 1;
}

size_t AkAudioCaps::planeSize() const
{
    return this->d->m_planeSize;
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
                                                                  int endianness)
{
    for (auto &sampleFormat: SampleFormats::formats())
        if (sampleFormat.type == type
            && sampleFormat.bps == bps
            && sampleFormat.endianness == endianness) {
            return sampleFormat.format;
        }

    return AkAudioCaps::SampleFormat_none;
}

bool AkAudioCaps::sampleFormatProperties(AkAudioCaps::SampleFormat sampleFormat,
                                         AkAudioCaps::SampleType *type,
                                         int *bps,
                                         int *endianness)
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

    return true;
}

bool AkAudioCaps::sampleFormatProperties(const QString &sampleFormat,
                                         AkAudioCaps::SampleType *type,
                                         int *bps,
                                         int *endianness)
{
    return AkAudioCaps::sampleFormatProperties(AkAudioCaps::sampleFormatFromString(sampleFormat),
                                               type,
                                               bps,
                                               endianness);
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
    this->d->updateParams();
    emit this->formatChanged(format);
}

void AkAudioCaps::setLayout(AkAudioCaps::ChannelLayout layout)
{
    if (this->d->m_layout == layout)
        return;

    this->d->m_layout = layout;
    this->d->updateParams();
    emit this->layoutChanged(layout);
}

void AkAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void AkAudioCaps::setSamples(int samples)
{
    if (this->d->m_samples == samples)
        return;

    this->d->m_samples = samples;
    this->d->updateParams();
    emit this->samplesChanged(samples);
}

void AkAudioCaps::setPlanar(bool planar)
{
    if (this->d->m_planar == planar)
        return;

    this->d->m_planar = planar;
    this->d->updateParams();
    emit this->planarChanged(planar);
}

void AkAudioCaps::setAlign(int align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    this->d->updateParams();
    emit this->alignChanged(align);
}

void AkAudioCaps::resetFormat()
{
    this->setFormat(SampleFormat_none);
}

void AkAudioCaps::resetLayout()
{
    this->setLayout(Layout_none);
}

void AkAudioCaps::resetRate()
{
    this->setRate(0);
}

void AkAudioCaps::resetSamples()
{
    this->setSamples(0);
}

void AkAudioCaps::resetPlanar()
{
    this->setPlanar(false);
}

void AkAudioCaps::resetAlign()
{
    this->setAlign(1);
}

void AkAudioCaps::clear()
{
    for (auto &property: this->dynamicPropertyNames())
        this->setProperty(property.constData(), {});
}

void AkAudioCapsPrivate::updateParams()
{
    auto af = SampleFormats::byFormat(this->m_format);
    auto layouts = ChannelLayouts::byLayout(this->m_layout);

    if (!af || !layouts) {
        this->m_planeSize = 0;
        this->m_offset.clear();

        return;
    }

    if (this->m_planar) {
        this->m_planeSize =
                SampleFormats::alignUp(size_t(af->bps
                                              * this->m_samples
                                              / 8),
                                       size_t(this->m_align));
    } else {
        this->m_planeSize =
                SampleFormats::alignUp(size_t(af->bps
                                              * layouts->channels
                                              * this->m_samples
                                              / 8),
                                       size_t(this->m_align));
    }

    this->m_offset.clear();
    size_t nplanes = this->m_planar? size_t(layouts->channels): 1;

    for (size_t i = 0; i < nplanes; i++)
        this->m_offset << i * this->m_planeSize;
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
