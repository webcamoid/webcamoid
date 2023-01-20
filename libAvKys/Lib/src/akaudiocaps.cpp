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
#include <QtMath>
#include <QQmlEngine>

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
            static const QVector<SampleFormats> sampleFormats {
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

class SpeakerPositions
{
    public:
        AkAudioCaps::Position position;
        qreal azimuth;
        qreal elevation;

        static inline const QVector<SpeakerPositions> &positions()
        {
            /* Speakers positions based on:
             *
             * https://mediaarea.net/AudioChannelLayout
             */
            static const QVector<SpeakerPositions> positions {
                {AkAudioCaps::Position_unknown            ,    0.0,   0.0},
                {AkAudioCaps::Position_FrontCenter        ,    0.0,   0.0},
                {AkAudioCaps::Position_FrontLeft          ,   45.0,   0.0},
                {AkAudioCaps::Position_FrontRight         ,  -45.0,   0.0},
                {AkAudioCaps::Position_BackCenter         ,  180.0,   0.0},
                {AkAudioCaps::Position_BackLeft           ,  150.0,   0.0},
                {AkAudioCaps::Position_BackRight          , -150.0,   0.0},
                {AkAudioCaps::Position_FrontLeftOfCenter  ,   15.0,   0.0},
                {AkAudioCaps::Position_FrontRightOfCenter ,  -15.0,   0.0},
                {AkAudioCaps::Position_WideLeft           ,   60.0,   0.0},
                {AkAudioCaps::Position_WideRight          ,  -60.0,   0.0},
                {AkAudioCaps::Position_SideLeft           ,   90.0,   0.0},
                {AkAudioCaps::Position_SideRight          ,  -90.0,   0.0},
                {AkAudioCaps::Position_LowFrequency1      ,   60.0, -30.0},
                {AkAudioCaps::Position_LowFrequency2      ,  -60.0, -30.0},
                {AkAudioCaps::Position_TopCenter          ,    0.0,  90.0},
                {AkAudioCaps::Position_TopFrontCenter     ,    0.0,  45.0},
                {AkAudioCaps::Position_TopFrontLeft       ,   45.0,  45.0},
                {AkAudioCaps::Position_TopFrontRight      ,  -45.0,  45.0},
                {AkAudioCaps::Position_TopBackCenter      ,  180.0,  45.0},
                {AkAudioCaps::Position_TopBackLeft        ,  135.0,  45.0},
                {AkAudioCaps::Position_TopBackRight       , -135.0,  45.0},
                {AkAudioCaps::Position_TopSideLeft        ,   90.0,  45.0},
                {AkAudioCaps::Position_TopSideRight       ,  -90.0,  45.0},
                {AkAudioCaps::Position_BottomFrontCenter  ,    0.0, -30.0},
                {AkAudioCaps::Position_BottomFrontLeft    ,   45.0, -30.0},
                {AkAudioCaps::Position_BottomFrontRight   ,  -45.0, -30.0},
                {AkAudioCaps::Position_StereoLeft         ,  110.0,   0.0},
                {AkAudioCaps::Position_StereoRight        , -110.0,   0.0},
                {AkAudioCaps::Position_SurroundDirectLeft ,   90.0,   0.0},
                {AkAudioCaps::Position_SurroundDirectRight,  -90.0,   0.0},
            };

            return positions;
        }

        static inline const SpeakerPositions *byPosition(AkAudioCaps::Position position)
        {
            for (auto &position_: positions())
                if (position_.position == position)
                    return &position_;

            return &positions().front();
        }
};

#define LAYOUT_MONO          AkAudioCaps::Position_FrontCenter
#define LAYOUT_STEREO        AkAudioCaps::Position_FrontLeft, \
                             AkAudioCaps::Position_FrontRight
#define LAYOUT_DOWNMIX       AkAudioCaps::Position_StereoLeft, \
                             AkAudioCaps::Position_StereoRight
#define LAYOUT_2P1           LAYOUT_STEREO, AkAudioCaps::Position_LowFrequency1
#define LAYOUT_3P0           LAYOUT_STEREO, AkAudioCaps::Position_FrontCenter
#define LAYOUT_3P0_BACK      LAYOUT_STEREO, AkAudioCaps::Position_BackCenter
#define LAYOUT_3P1           LAYOUT_3P0, AkAudioCaps::Position_LowFrequency1
#define LAYOUT_4P0           LAYOUT_3P0, AkAudioCaps::Position_BackCenter
#define LAYOUT_QUAD          LAYOUT_STEREO, \
                             AkAudioCaps::Position_BackLeft, \
                             AkAudioCaps::Position_BackRight
#define LAYOUT_QUAD_SIDE     LAYOUT_STEREO, \
                             AkAudioCaps::Position_SideLeft, \
                             AkAudioCaps::Position_SideRight
#define LAYOUT_4P1           LAYOUT_4P0, AkAudioCaps::Position_LowFrequency1
#define LAYOUT_5P0           LAYOUT_3P0, AkAudioCaps::Position_BackLeft, \
                             AkAudioCaps::Position_BackRight
#define LAYOUT_5P0_SIDE      LAYOUT_3P0, AkAudioCaps::Position_SideLeft, \
                             AkAudioCaps::AkAudioCaps::Position_SideRight
#define LAYOUT_5P1           LAYOUT_5P0, AkAudioCaps::Position_LowFrequency1
#define LAYOUT_5P1_SIDE      LAYOUT_5P0_SIDE, \
                             AkAudioCaps::Position_LowFrequency1
#define LAYOUT_6P0           LAYOUT_5P0_SIDE, AkAudioCaps::Position_BackCenter
#define LAYOUT_6P0_FRONT     LAYOUT_QUAD_SIDE, \
                             AkAudioCaps::Position_FrontLeftOfCenter, \
                             AkAudioCaps::Position_FrontRightOfCenter
#define LAYOUT_HEXAGONAL     LAYOUT_5P0, AkAudioCaps::Position_BackCenter
#define LAYOUT_6P1           LAYOUT_5P1_SIDE, AkAudioCaps::Position_BackCenter
#define LAYOUT_6P1_BACK      LAYOUT_5P1, AkAudioCaps::Position_BackCenter
#define LAYOUT_6P1_FRONT     LAYOUT_6P0_FRONT, \
                             AkAudioCaps::Position_LowFrequency1
#define LAYOUT_7P0           LAYOUT_5P0_SIDE, \
                             AkAudioCaps::Position_BackLeft, \
                             AkAudioCaps::Position_BackRight
#define LAYOUT_7P0_FRONT     LAYOUT_5P0_SIDE, \
                             AkAudioCaps::Position_FrontLeftOfCenter, \
                             AkAudioCaps::Position_FrontRightOfCenter
#define LAYOUT_7P1           LAYOUT_5P1_SIDE, \
                             AkAudioCaps::Position_BackLeft, \
                             AkAudioCaps::Position_BackRight
#define LAYOUT_7P1_WIDE      LAYOUT_5P1_SIDE, \
                             AkAudioCaps::Position_FrontLeftOfCenter, \
                             AkAudioCaps::Position_FrontRightOfCenter
#define LAYOUT_7P1_WIDE_BACK LAYOUT_5P1, \
                             AkAudioCaps::Position_FrontLeftOfCenter, \
                             AkAudioCaps::Position_FrontRightOfCenter
#define LAYOUT_OCTAGONAL     LAYOUT_7P0, AkAudioCaps::Position_BackCenter
#define LAYOUT_HEXADECAGONAL LAYOUT_OCTAGONAL, \
                             AkAudioCaps::Position_WideLeft, \
                             AkAudioCaps::Position_WideRight, \
                             AkAudioCaps::Position_TopBackLeft, \
                             AkAudioCaps::Position_TopBackRight, \
                             AkAudioCaps::Position_TopBackCenter, \
                             AkAudioCaps::Position_TopFrontCenter, \
                             AkAudioCaps::Position_TopFrontLeft, \
                             AkAudioCaps::Position_TopFrontRight

class ChannelLayouts
{
    public:
        AkAudioCaps::ChannelLayout layout;
        QVector<AkAudioCaps::Position> positions;
        QString description;

        static inline const QVector<ChannelLayouts> &layouts()
        {
            static const QVector<ChannelLayouts> channelLayouts {
                {AkAudioCaps::Layout_none         , {}                    , "none"          },
                {AkAudioCaps::Layout_mono         , {LAYOUT_MONO}         , "mono"          },
                {AkAudioCaps::Layout_stereo       , {LAYOUT_STEREO}       , "stereo"        },
                {AkAudioCaps::Layout_downmix      , {LAYOUT_DOWNMIX}      , "downmix"       },
                {AkAudioCaps::Layout_2p1          , {LAYOUT_2P1}          , "2.1"           },
                {AkAudioCaps::Layout_3p0          , {LAYOUT_3P0}          , "3.0"           },
                {AkAudioCaps::Layout_3p0_back     , {LAYOUT_3P0_BACK}     , "3.0(back)"     },
                {AkAudioCaps::Layout_3p1          , {LAYOUT_3P1}          , "3.1"           },
                {AkAudioCaps::Layout_4p0          , {LAYOUT_4P0}          , "4.0"           },
                {AkAudioCaps::Layout_quad         , {LAYOUT_QUAD}         , "quad"          },
                {AkAudioCaps::Layout_quad_side    , {LAYOUT_QUAD_SIDE}    , "quad(side)"    },
                {AkAudioCaps::Layout_4p1          , {LAYOUT_4P1}          , "4.1"           },
                {AkAudioCaps::Layout_5p0          , {LAYOUT_5P0}          , "5.0"           },
                {AkAudioCaps::Layout_5p0_side     , {LAYOUT_5P0_SIDE}     , "5.0(side)"     },
                {AkAudioCaps::Layout_5p1          , {LAYOUT_5P1}          , "5.1"           },
                {AkAudioCaps::Layout_5p1_side     , {LAYOUT_5P1_SIDE}     , "5.1(side)"     },
                {AkAudioCaps::Layout_6p0          , {LAYOUT_6P0}          , "6.0"           },
                {AkAudioCaps::Layout_6p0_front    , {LAYOUT_6P0_FRONT}    , "6.0(front)"    },
                {AkAudioCaps::Layout_hexagonal    , {LAYOUT_HEXAGONAL}    , "hexagonal"     },
                {AkAudioCaps::Layout_6p1          , {LAYOUT_6P1}          , "6.1"           },
                {AkAudioCaps::Layout_6p1_back     , {LAYOUT_6P1_BACK}     , "6.1(back)"     },
                {AkAudioCaps::Layout_6p1_front    , {LAYOUT_6P1_FRONT}    , "6.1(front)"    },
                {AkAudioCaps::Layout_7p0          , {LAYOUT_7P0}          , "7.0"           },
                {AkAudioCaps::Layout_7p0_front    , {LAYOUT_7P0_FRONT}    , "7.0(front)"    },
                {AkAudioCaps::Layout_7p1          , {LAYOUT_7P1}          , "7.1"           },
                {AkAudioCaps::Layout_7p1_wide     , {LAYOUT_7P1_WIDE}     , "7.1(wide)"     },
                {AkAudioCaps::Layout_7p1_wide_back, {LAYOUT_7P1_WIDE_BACK}, "7.1(wide-back)"},
                {AkAudioCaps::Layout_octagonal    , {LAYOUT_OCTAGONAL}    , "octagonal"     },
                {AkAudioCaps::Layout_hexadecagonal, {LAYOUT_HEXADECAGONAL}, "hexadecagonal" },
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

        static inline const ChannelLayouts *byChannelCount(int channels)
        {
            for (auto &layout: layouts())
                if (layout.positions.size() == channels)
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
        int m_planar {false};
        int m_rate {0};
};

AkAudioCaps::AkAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioCapsPrivate();
}

AkAudioCaps::AkAudioCaps(SampleFormat format,
                         ChannelLayout layout,
                         bool planar,
                         int rate):
    QObject()
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_format = format;
    this->d->m_layout = layout;
    this->d->m_planar = planar;
    this->d->m_rate = rate;
}

AkAudioCaps::AkAudioCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkAudioCapsPrivate();

    if (other.type() == AkCaps::CapsAudio) {
        auto data = reinterpret_cast<AkAudioCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_layout = data->d->m_layout;
        this->d->m_planar = data->d->m_planar;
        this->d->m_rate = data->d->m_rate;
    }
}

AkAudioCaps::AkAudioCaps(const AkAudioCaps &other):
    QObject()
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_layout = other.d->m_layout;
    this->d->m_planar = other.d->m_planar;
    this->d->m_rate = other.d->m_rate;
}

AkAudioCaps::~AkAudioCaps()
{
    delete this->d;
}

AkAudioCaps &AkAudioCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsAudio) {
        auto data = reinterpret_cast<AkAudioCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_layout = data->d->m_layout;
        this->d->m_planar = data->d->m_planar;
        this->d->m_rate = data->d->m_rate;
    } else {
        this->d->m_format = SampleFormat_none;
        this->d->m_layout = Layout_none;
        this->d->m_planar = false;
        this->d->m_rate = 0;
    }

    return *this;
}

AkAudioCaps &AkAudioCaps::operator =(const AkAudioCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_layout = other.d->m_layout;
        this->d->m_planar = other.d->m_planar;
        this->d->m_rate = other.d->m_rate;;
    }

    return *this;
}

bool AkAudioCaps::operator ==(const AkAudioCaps &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_layout == other.d->m_layout
            && this->d->m_planar == other.d->m_planar
            && this->d->m_rate == other.d->m_rate;
}

bool AkAudioCaps::operator !=(const AkAudioCaps &other) const
{
    return !(*this == other);
}

QObject *AkAudioCaps::create()
{
    return new AkAudioCaps();
}

QObject *AkAudioCaps::create(const AkCaps &caps)
{
    return new AkAudioCaps(caps);
}

QObject *AkAudioCaps::create(const AkAudioCaps &caps)
{
    return new AkAudioCaps(caps);
}

QObject *AkAudioCaps::create(SampleFormat format,
                             ChannelLayout layout,
                             bool planar,
                             int rate)
{
    return new AkAudioCaps(format, layout, planar, rate);
}

QVariant AkAudioCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkAudioCaps::operator bool() const
{
    auto af = SampleFormats::byFormat(this->d->m_format);
    auto layouts = ChannelLayouts::byLayout(this->d->m_layout);

    if (!af || !layouts)
        return false;

    if (af->format == SampleFormat_none || layouts->layout == Layout_none)
        return false;

    return this->d->m_rate > 0;
}

AkAudioCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsAudio);
    caps.setPrivateData(new AkAudioCaps(*this),
                        [] (void *data) -> void * {
                            return new AkAudioCaps(*reinterpret_cast<AkAudioCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkAudioCaps *>(data);
                        });

    return caps;
}

AkAudioCaps::SampleFormat AkAudioCaps::format() const
{
    return this->d->m_format;
}

AkAudioCaps::ChannelLayout AkAudioCaps::layout() const
{
    return this->d->m_layout;
}

bool AkAudioCaps::planar() const
{
    return this->d->m_planar;
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

const QVector<AkAudioCaps::Position> AkAudioCaps::positions() const
{
    auto layouts = ChannelLayouts::byLayout(this->d->m_layout);

    if (!layouts)
        return {};

    if (layouts->layout == Layout_none)
        return {};

    return layouts->positions;
}

int AkAudioCaps::bitsPerSample(SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->bps;
}

QString AkAudioCaps::sampleFormatToString(SampleFormat sampleFormat)
{
    AkAudioCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(sampleFormat));
    format.remove("SampleFormat_");

    return format;
}

AkAudioCaps::SampleFormat AkAudioCaps::sampleFormatFromProperties(SampleType type,
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

bool AkAudioCaps::sampleFormatProperties(SampleFormat sampleFormat,
                                         SampleType *type,
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

AkAudioCaps::SampleType AkAudioCaps::sampleType(SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->type;
}

QString AkAudioCaps::channelLayoutToString(ChannelLayout channelLayout)
{
    return ChannelLayouts::byLayout(channelLayout)->description;
}

AkAudioCaps::ChannelLayout AkAudioCaps::channelLayoutFromPositions(const QVector<Position> &positions)
{
    QSet<Position> positionsSet;

    for (auto &position: positions)
        positionsSet << position;

    for (auto &layout: ChannelLayouts::layouts()) {
        if (layout.positions.size() != positions.size())
            continue;

        QSet<Position> layoutPositionsSet;

        for (auto &position: layout.positions)
            layoutPositionsSet << position;

        if (positionsSet == layoutPositionsSet)
            return layout.layout;
    }

    return Layout_none;
}

int AkAudioCaps::channelCount(ChannelLayout channelLayout)
{
    return ChannelLayouts::byLayout(channelLayout)->positions.size();
}

int AkAudioCaps::endianness(SampleFormat sampleFormat)
{
    return SampleFormats::byFormat(sampleFormat)->endianness;
}

AkAudioCaps::ChannelLayout AkAudioCaps::defaultChannelLayout(int channelCount)
{
    return ChannelLayouts::byChannelCount(channelCount)->layout;
}

const QVector<AkAudioCaps::Position> &AkAudioCaps::positions(ChannelLayout channelLayout)
{
    return ChannelLayouts::byLayout(channelLayout)->positions;
}

AkAudioCaps::SpeakerPosition AkAudioCaps::position(Position position)
{
    auto sp = SpeakerPositions::byPosition(position);

    if (!sp)
        return {};

    return {sp->azimuth, sp->elevation};
}

AkAudioCaps::SpeakerPosition AkAudioCaps::position(int channel) const
{
    auto positions = AkAudioCaps::positions(this->d->m_layout);

    return AkAudioCaps::position(positions.at(channel));
}

qreal AkAudioCaps::distanceFactor(SpeakerPosition position1,
                                  SpeakerPosition position2)
{
    auto d = 1.0 + (position1 - position2);

    return 1.0 / (d * d);
}

qreal AkAudioCaps::distanceFactor(Position position1, Position position2)
{
    return distanceFactor(position(position1), position(position2));
}

void AkAudioCaps::setFormat(SampleFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkAudioCaps::setLayout(ChannelLayout layout)
{
    if (this->d->m_layout == layout)
        return;

    this->d->m_layout = layout;
    emit this->layoutChanged(layout);
}

void AkAudioCaps::setPlanar(bool planar)
{
    if (this->d->m_planar == planar)
        return;

    this->d->m_planar = planar;
    emit this->planarChanged(planar);
}

void AkAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void AkAudioCaps::resetFormat()
{
    this->setFormat(SampleFormat_none);
}

void AkAudioCaps::resetLayout()
{
    this->setLayout(Layout_none);
}

void AkAudioCaps::resetPlanar()
{
    this->setPlanar(false);
}

void AkAudioCaps::resetRate()
{
    this->setRate(0);
}

void AkAudioCaps::registerTypes()
{
    qRegisterMetaType<AkAudioCaps>("AkAudioCaps");
    qRegisterMetaTypeStreamOperators<AkAudioCaps>("AkAudioCaps");
    qRegisterMetaType<SampleFormat>("SampleFormat");
    QMetaType::registerDebugStreamOperator<SampleFormat>();
    qRegisterMetaType<SampleFormatList>("SampleFormatList");
    qRegisterMetaType<SampleType>("SampleType");
    QMetaType::registerDebugStreamOperator<SampleType>();
    qRegisterMetaType<Position>("Position");
    QMetaType::registerDebugStreamOperator<Position>();
    qRegisterMetaType<ChannelLayout>("ChannelLayout");
    QMetaType::registerDebugStreamOperator<ChannelLayout>();
    qRegisterMetaType<ChannelLayoutList>("ChannelLayoutList");
    qmlRegisterSingletonType<AkAudioCaps>("Ak", 1, 0, "AkAudioCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkAudioCaps();
    });
}

qreal operator -(const AkAudioCaps::SpeakerPosition &pos1,
                 const AkAudioCaps::SpeakerPosition &pos2)
{
    auto a1 = qDegreesToRadians(pos1.first);
    auto e1 = qDegreesToRadians(pos1.second);

    auto x1 = qCos(e1) * qCos(a1);
    auto y1 = qCos(e1) * qSin(a1);
    auto z1 = qSin(e1);

    auto a2 = qDegreesToRadians(pos2.first);
    auto e2 = qDegreesToRadians(pos2.second);

    auto x2 = qCos(e2) * qCos(a2);
    auto y2 = qCos(e2) * qSin(a2);
    auto z2 = qSin(e2);

    auto dx = x1 - x2;
    auto dy = y1 - y2;
    auto dz = z1 - z2;

    return qSqrt(dx * dx + dy * dy + dz * dz);
}

QDebug operator <<(QDebug debug, const AkAudioCaps &caps)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AkAudioCaps("
                    << "format="
                    << caps.format()
                    << ",layout="
                    << caps.layout()
                    << ",planar="
                    << caps.planar()
                    << ",rate="
                    << caps.rate()
                    << ")";

    return debug;
}

QDebug operator <<(QDebug debug, AkAudioCaps::SampleFormat format)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << AkAudioCaps::sampleFormatToString(format).toStdString().c_str();

    return debug;
}

QDebug operator <<(QDebug debug, AkAudioCaps::SampleType sampleType)
{
    AkAudioCaps caps;
    int sampleTypeIndex = caps.metaObject()->indexOfEnumerator("SampleType");
    QMetaEnum sampleTypeEnum = caps.metaObject()->enumerator(sampleTypeIndex);
    QString sampleTypeStr(sampleTypeEnum.valueToKey(sampleType));
    sampleTypeStr.remove("SampleType_");
    QDebugStateSaver saver(debug);
    debug.nospace() << sampleTypeStr.toStdString().c_str();

    return debug;
}

QDebug operator <<(QDebug debug, AkAudioCaps::Position position)
{
    AkAudioCaps caps;
    int positionIndex = caps.metaObject()->indexOfEnumerator("Position");
    QMetaEnum positionEnum = caps.metaObject()->enumerator(positionIndex);
    QString positionStr(positionEnum.valueToKey(position));
    positionStr.remove("Position_");
    QDebugStateSaver saver(debug);
    debug.nospace() << positionStr.toStdString().c_str();

    return debug;
}

QDebug operator <<(QDebug debug, AkAudioCaps::ChannelLayout layout)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << AkAudioCaps::channelLayoutToString(layout).toStdString().c_str();

    return debug;
}

QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps)
{
    AkAudioCaps::SampleFormat format = AkAudioCaps::SampleFormat_none;
    istream >> format;
    caps.setFormat(format);
    AkAudioCaps::ChannelLayout layout = AkAudioCaps::Layout_none;
    istream >> layout;
    caps.setLayout(layout);
    bool planar = false;
    istream >> planar;
    caps.setPlanar(planar);
    int rate = 0;
    istream >> rate;
    caps.setRate(rate);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps)
{
    ostream << caps.format();
    ostream << caps.layout();
    ostream << caps.planar();
    ostream << caps.rate();

    return ostream;
}

#include "moc_akaudiocaps.cpp"
