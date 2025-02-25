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

#include <QSharedPointer>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
}

#include "audiostream.h"
#include "clock.h"

// No AV correction is done if too big error.
#define AV_NOSYNC_THRESHOLD 10.0

// Maximum audio speed change to get correct sync
#define SAMPLE_CORRECTION_PERCENT_MAX 10

// We use about AUDIO_DIFF_AVG_NB A-V differences to make the average
#define AUDIO_DIFF_AVG_NB 20

struct SampleFormat
{
    AVSampleFormat ffFormat;
    AkAudioCaps::SampleFormat akFormat;

    static inline const SampleFormat *byFF(AVSampleFormat ffFormat);
    static inline const SampleFormat *byAk(AkAudioCaps::SampleFormat akFormat);
};

static const SampleFormat multiSrcSampleFormatTable[] {
    {AV_SAMPLE_FMT_U8  , AkAudioCaps::SampleFormat_u8  },
    {AV_SAMPLE_FMT_S16 , AkAudioCaps::SampleFormat_s16 },
    {AV_SAMPLE_FMT_S32 , AkAudioCaps::SampleFormat_s32 },
    {AV_SAMPLE_FMT_FLT , AkAudioCaps::SampleFormat_flt },
    {AV_SAMPLE_FMT_DBL , AkAudioCaps::SampleFormat_dbl },

    {AV_SAMPLE_FMT_U8P , AkAudioCaps::SampleFormat_u8  },
    {AV_SAMPLE_FMT_S16P, AkAudioCaps::SampleFormat_s16 },
    {AV_SAMPLE_FMT_S32P, AkAudioCaps::SampleFormat_s32 },
    {AV_SAMPLE_FMT_FLTP, AkAudioCaps::SampleFormat_flt },
    {AV_SAMPLE_FMT_DBLP, AkAudioCaps::SampleFormat_dbl },
    {AV_SAMPLE_FMT_S64 , AkAudioCaps::SampleFormat_s64 },
    {AV_SAMPLE_FMT_S64P, AkAudioCaps::SampleFormat_s64 },

    {AV_SAMPLE_FMT_NONE, AkAudioCaps::SampleFormat_none},
};

const SampleFormat *SampleFormat::byFF(AVSampleFormat ffFormat)
{
    auto fmt = multiSrcSampleFormatTable;

    for (; fmt->akFormat != AkAudioCaps::SampleFormat_none; fmt++)
        if (fmt->ffFormat == ffFormat)
            return fmt;

    return fmt;
}

const SampleFormat *SampleFormat::byAk(AkAudioCaps::SampleFormat akFormat)
{
    auto fmt = multiSrcSampleFormatTable;

    for (; fmt->akFormat != AkAudioCaps::SampleFormat_none; fmt++)
        if (fmt->akFormat == akFormat)
            return fmt;

    return fmt;
}

struct ChannelLayout
{
    uint64_t ffLayout;
    AkAudioCaps::ChannelLayout akLayout;

    static inline const ChannelLayout *byFF(uint64_t ffLayout);
    static inline const ChannelLayout *byAk(AkAudioCaps::ChannelLayout akLayout);
};

static const ChannelLayout multiSrcChannelLayoutsTable[] {
    {AV_CH_LAYOUT_MONO             , AkAudioCaps::Layout_mono         },
    {AV_CH_LAYOUT_STEREO           , AkAudioCaps::Layout_stereo       },
    {AV_CH_LAYOUT_2POINT1          , AkAudioCaps::Layout_2p1          },
    {AV_CH_LAYOUT_SURROUND         , AkAudioCaps::Layout_3p0          },
    {AV_CH_LAYOUT_2_1              , AkAudioCaps::Layout_3p0_back     },
    {AV_CH_LAYOUT_3POINT1          , AkAudioCaps::Layout_3p1          },
    {AV_CH_LAYOUT_4POINT0          , AkAudioCaps::Layout_4p0          },
    {AV_CH_LAYOUT_QUAD             , AkAudioCaps::Layout_quad         },
    {AV_CH_LAYOUT_2_2              , AkAudioCaps::Layout_quad_side    },
    {AV_CH_LAYOUT_4POINT1          , AkAudioCaps::Layout_4p1          },
    {AV_CH_LAYOUT_5POINT0_BACK     , AkAudioCaps::Layout_5p0          },
    {AV_CH_LAYOUT_5POINT0          , AkAudioCaps::Layout_5p0_side     },
    {AV_CH_LAYOUT_5POINT1_BACK     , AkAudioCaps::Layout_5p1          },
    {AV_CH_LAYOUT_5POINT1          , AkAudioCaps::Layout_5p1_side     },
    {AV_CH_LAYOUT_6POINT0          , AkAudioCaps::Layout_6p0          },
    {AV_CH_LAYOUT_6POINT0_FRONT    , AkAudioCaps::Layout_6p0_front    },
    {AV_CH_LAYOUT_HEXAGONAL        , AkAudioCaps::Layout_hexagonal    },
    {AV_CH_LAYOUT_6POINT1          , AkAudioCaps::Layout_6p1          },
    {AV_CH_LAYOUT_6POINT1_BACK     , AkAudioCaps::Layout_6p1_back     },
    {AV_CH_LAYOUT_6POINT1_FRONT    , AkAudioCaps::Layout_6p1_front    },
    {AV_CH_LAYOUT_7POINT0          , AkAudioCaps::Layout_7p0          },
    {AV_CH_LAYOUT_7POINT0_FRONT    , AkAudioCaps::Layout_7p0_front    },
    {AV_CH_LAYOUT_7POINT1          , AkAudioCaps::Layout_7p1          },
    {AV_CH_LAYOUT_7POINT1_WIDE     , AkAudioCaps::Layout_7p1_wide     },
    {AV_CH_LAYOUT_7POINT1_WIDE_BACK, AkAudioCaps::Layout_7p1_wide_back},
    {AV_CH_LAYOUT_OCTAGONAL        , AkAudioCaps::Layout_octagonal    },
    {AV_CH_LAYOUT_HEXADECAGONAL    , AkAudioCaps::Layout_hexadecagonal},
    {AV_CH_LAYOUT_STEREO_DOWNMIX   , AkAudioCaps::Layout_downmix      },
    {0                             , AkAudioCaps::Layout_none         },
};

const ChannelLayout *ChannelLayout::byFF(uint64_t ffLayout)
{
    auto lyt = multiSrcChannelLayoutsTable;

    for (; lyt->akLayout != AkAudioCaps::Layout_none; lyt++)
        if (lyt->ffLayout == ffLayout)
            return lyt;

    return lyt;
}

const ChannelLayout *ChannelLayout::byAk(AkAudioCaps::ChannelLayout akLayout)
{
    auto lyt = multiSrcChannelLayoutsTable;

    for (; lyt->akLayout != AkAudioCaps::Layout_none; lyt++)
        if (lyt->akLayout == akLayout)
            return lyt;

    return lyt;
}

class AudioStreamPrivate
{
    public:
        AudioStream *self;
        qint64 m_pts {0};
        AkAudioConverter m_audioConvert;
        qreal m_audioDiffCum {0.0}; // used for AV difference average computation
        qreal m_audioDiffAvgCoef {exp(log(0.01) / AUDIO_DIFF_AVG_NB)};
        int audioDiffAvgCount {0};

        explicit AudioStreamPrivate(AudioStream *self);
        AkAudioPacket frameToPacket(AVFrame *iFrame);
        AkPacket convert(AVFrame *iFrame);
        AVFrame *copyFrame(AVFrame *frame) const;
        static bool isPlanar(AVSampleFormat format);
};

AudioStream::AudioStream(const AVFormatContext *formatContext,
                         uint index,
                         qint64 id,
                         Clock *globalClock,
                         bool sync,
                         bool noModify,
                         QObject *parent):
    AbstractStream(formatContext,
                   index,
                   id,
                   globalClock,
                   sync,
                   noModify,
                   parent)
{
    this->d = new AudioStreamPrivate(this);
    this->m_maxData = 9;
}

AudioStream::~AudioStream()
{
    delete this->d;
}

AkCaps AudioStream::caps() const
{
    auto iFormat = AVSampleFormat(this->codecContext()->sample_fmt);
    auto oFormat = av_get_packed_sample_fmt(iFormat);
    auto sampleFormat = SampleFormat::byFF(oFormat)->akFormat;
    bool isPlanar = AudioStreamPrivate::isPlanar(oFormat);

    if (sampleFormat == AkAudioCaps::SampleFormat_none) {
        sampleFormat = AkAudioCaps::SampleFormat_flt;
        isPlanar = false;
    }

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    auto layout = ChannelLayout::byFF(this->codecContext()->ch_layout.u.mask)->akLayout;
#else
    auto layout = ChannelLayout::byFF(this->codecContext()->channel_layout)->akLayout;
#endif

    if (layout == AkAudioCaps::Layout_none)
        layout = AkAudioCaps::Layout_stereo;

    AkAudioCaps caps(sampleFormat,
                     layout,
                     isPlanar,
                     this->codecContext()->sample_rate);

    return caps;
}

bool AudioStream::decodeData()
{
    if (!this->isValid())
        return false;

    bool result = false;

    forever {
        auto iFrame = av_frame_alloc();
        int r = avcodec_receive_frame(this->codecContext(), iFrame);

        if (r >= 0) {
            this->dataEnqueue(this->d->copyFrame(iFrame));
            result = true;
        }

        av_frame_free(&iFrame);

        if (r < 0)
            break;
    }

    return result;
}

void AudioStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    if (!packet) {
        this->dataEnqueue(nullptr);

        return;
    }

    avcodec_send_packet(this->codecContext(), packet);
}

void AudioStream::processData(AVFrame *frame)
{
    frame->pts = frame->pts != AV_NOPTS_VALUE? frame->pts: this->d->m_pts;
    AkPacket oPacket = this->d->convert(frame);
    emit this->oStream(oPacket);
    this->d->m_pts = frame->pts + frame->nb_samples;
}

AudioStreamPrivate::AudioStreamPrivate(AudioStream *self):
    self(self)
{
}

AkAudioPacket AudioStreamPrivate::frameToPacket(AVFrame *iFrame)
{
    auto sampleFormat = SampleFormat::byFF(AVSampleFormat(iFrame->format));

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    auto layout = ChannelLayout::byFF(iFrame->ch_layout.u.mask)->akLayout;
#else
    auto layout = ChannelLayout::byFF(iFrame->channel_layout)->akLayout;
#endif

    AkAudioCaps caps(sampleFormat->akFormat,
                     layout,
                     AudioStreamPrivate::isPlanar(AVSampleFormat(iFrame->format)),
                     iFrame->sample_rate);
    AkAudioPacket packet(caps, iFrame->nb_samples);
    size_t lineSize = iFrame->linesize[0];

    for (int plane = 0; plane < packet.planes(); ++plane) {
        memcpy(packet.plane(plane),
               iFrame->data[plane],
               qMin<size_t>(packet.planeSize(plane), lineSize));
    }

    packet.setPts(iFrame->pts);
    packet.setDuration(iFrame->nb_samples);
    packet.setTimeBase({1, iFrame->sample_rate});
    packet.setIndex(int(self->index()));
    packet.setId(self->id());

    return packet;
}

AkPacket AudioStreamPrivate::convert(AVFrame *iFrame)
{
    auto packet = this->frameToPacket(iFrame);
    auto caps = packet.caps();
    auto layout = caps.layout();

    if (layout != AkAudioCaps::Layout_mono
        && layout != AkAudioCaps::Layout_stereo)
        caps.setLayout(AkAudioCaps::Layout_stereo);

    this->m_audioConvert.setOutputCaps(caps);

    if (!self->sync())
        return this->m_audioConvert.convert(packet);

    // Synchronize audio
    qreal pts = iFrame->pts * self->timeBase().value();
    qreal diff = pts - self->globalClock()->clock();

    if (!qIsNaN(diff) && qAbs(diff) < AV_NOSYNC_THRESHOLD) {
        this->m_audioDiffCum = diff + this->m_audioDiffAvgCoef * this->m_audioDiffCum;

        if (this->audioDiffAvgCount < AUDIO_DIFF_AVG_NB) {
            // not enough measures to have a correct estimate
            this->audioDiffAvgCount++;
        } else {
            // estimate the A-V difference
            qreal avgDiff = this->m_audioDiffCum * (1.0 - this->m_audioDiffAvgCoef);

            // since we do not have a precise anough audio fifo fullness,
            // we correct audio sync only if larger than this threshold
            qreal diffThreshold = 2.0 * iFrame->nb_samples / iFrame->sample_rate;

            if (qAbs(avgDiff) >= diffThreshold) {
                int wantedSamples = iFrame->nb_samples + int(diff * iFrame->sample_rate);
                int minSamples = iFrame->nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                int maxSamples = iFrame->nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                wantedSamples = qBound(minSamples, wantedSamples, maxSamples);
                packet = this->m_audioConvert.scale(packet, wantedSamples);
            }
        }
    } else {
        // Too big difference: may be initial PTS errors, so
        // reset A-V filter
        this->audioDiffAvgCount = 0;
        this->m_audioDiffCum = 0.0;
    }

    if (qAbs(diff) >= AV_NOSYNC_THRESHOLD)
        self->globalClock()->setClock(pts);

    self->clockDiff() = diff;

    return this->m_audioConvert.convert(packet);
}

AVFrame *AudioStreamPrivate::copyFrame(AVFrame *frame) const
{
    auto oFrame = av_frame_alloc();
    oFrame->format = frame->format;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    av_channel_layout_copy(&oFrame->ch_layout, &frame->ch_layout);
#else
    oFrame->channel_layout = frame->channel_layout;
#endif
    oFrame->sample_rate = frame->sample_rate;
    oFrame->nb_samples = frame->nb_samples;
    oFrame->pts = frame->pts;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    int channels = oFrame->ch_layout.nb_channels;
#else
    int channels = av_get_channel_layout_nb_channels(oFrame->channel_layout);
#endif

    av_samples_alloc(oFrame->data,
                     oFrame->linesize,
                     channels,
                     oFrame->nb_samples,
                     AVSampleFormat(oFrame->format),
                     1);
    av_samples_copy(oFrame->data,
                    frame->data,
                    0,
                    0,
                    oFrame->nb_samples,
                    channels,
                    AVSampleFormat(oFrame->format));

    return oFrame;
}

bool AudioStreamPrivate::isPlanar(AVSampleFormat format)
{
    static const AVSampleFormat formats[] {
        AV_SAMPLE_FMT_U8P ,
        AV_SAMPLE_FMT_S16P,
        AV_SAMPLE_FMT_S32P,
        AV_SAMPLE_FMT_FLTP,
        AV_SAMPLE_FMT_DBLP,
        AV_SAMPLE_FMT_S64P,
        AV_SAMPLE_FMT_NONE,
    };

    for (auto fmt = formats; *fmt != AV_SAMPLE_FMT_NONE; fmt++)
        if (*fmt == format)
            return true;

    return false;
}

#include "moc_audiostream.cpp"
