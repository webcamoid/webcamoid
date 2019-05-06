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
#include <QMap>
#include <akelement.h>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akpacket.h>
#include <akaudiopacket.h>

extern "C"
{
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

using SampleFormatMap = QMap<AVSampleFormat, AkAudioCaps::SampleFormat>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat = {
        {AV_SAMPLE_FMT_U8  , AkAudioCaps::SampleFormat_u8  },
        {AV_SAMPLE_FMT_S16 , AkAudioCaps::SampleFormat_s16 },
        {AV_SAMPLE_FMT_S32 , AkAudioCaps::SampleFormat_s32 },
        {AV_SAMPLE_FMT_FLT , AkAudioCaps::SampleFormat_flt },
        {AV_SAMPLE_FMT_DBL , AkAudioCaps::SampleFormat_dbl },

#ifdef HAVE_SAMPLEFORMAT64
        {AV_SAMPLE_FMT_S64 , AkAudioCaps::SampleFormat_s64 },
#endif
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

using ChannelLayoutsMap = QMap<AkAudioCaps::ChannelLayout, uint64_t>;

inline ChannelLayoutsMap initChannelFormatsMap()
{
    ChannelLayoutsMap channelLayouts = {
        {AkAudioCaps::Layout_mono         , AV_CH_LAYOUT_MONO             },
        {AkAudioCaps::Layout_stereo       , AV_CH_LAYOUT_STEREO           },
        {AkAudioCaps::Layout_2p1          , AV_CH_LAYOUT_2POINT1          },
        {AkAudioCaps::Layout_3p0          , AV_CH_LAYOUT_SURROUND         },
        {AkAudioCaps::Layout_3p0_back     , AV_CH_LAYOUT_2_1              },
        {AkAudioCaps::Layout_3p1          , AV_CH_LAYOUT_3POINT1          },
        {AkAudioCaps::Layout_4p0          , AV_CH_LAYOUT_4POINT0          },
        {AkAudioCaps::Layout_quad         , AV_CH_LAYOUT_QUAD             },
        {AkAudioCaps::Layout_quad_side    , AV_CH_LAYOUT_2_2              },
        {AkAudioCaps::Layout_4p1          , AV_CH_LAYOUT_4POINT1          },
        {AkAudioCaps::Layout_5p0          , AV_CH_LAYOUT_5POINT0_BACK     },
        {AkAudioCaps::Layout_5p0_side     , AV_CH_LAYOUT_5POINT0          },
        {AkAudioCaps::Layout_5p1          , AV_CH_LAYOUT_5POINT1_BACK     },
        {AkAudioCaps::Layout_5p1_side     , AV_CH_LAYOUT_5POINT1          },
        {AkAudioCaps::Layout_6p0          , AV_CH_LAYOUT_6POINT0          },
        {AkAudioCaps::Layout_6p0_front    , AV_CH_LAYOUT_6POINT0_FRONT    },
        {AkAudioCaps::Layout_hexagonal    , AV_CH_LAYOUT_HEXAGONAL        },
        {AkAudioCaps::Layout_6p1          , AV_CH_LAYOUT_6POINT1          },
        {AkAudioCaps::Layout_6p1_back     , AV_CH_LAYOUT_6POINT1_BACK     },
        {AkAudioCaps::Layout_6p1_front    , AV_CH_LAYOUT_6POINT1_FRONT    },
        {AkAudioCaps::Layout_7p0          , AV_CH_LAYOUT_7POINT0          },
        {AkAudioCaps::Layout_7p0_front    , AV_CH_LAYOUT_7POINT0_FRONT    },
        {AkAudioCaps::Layout_7p1          , AV_CH_LAYOUT_7POINT1          },
        {AkAudioCaps::Layout_7p1_wide     , AV_CH_LAYOUT_7POINT1_WIDE     },
        {AkAudioCaps::Layout_7p1_wide_side, AV_CH_LAYOUT_7POINT1_WIDE_BACK},
        {AkAudioCaps::Layout_octagonal    , AV_CH_LAYOUT_OCTAGONAL        },
#ifdef AV_CH_LAYOUT_HEXADECAGONAL
        {AkAudioCaps::Layout_hexadecagonal, AV_CH_LAYOUT_HEXADECAGONAL    },
#endif
        {AkAudioCaps::Layout_downmix      , AV_CH_LAYOUT_STEREO_DOWNMIX   },
    };

    return channelLayouts;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutsMap, channelLayouts, (initChannelFormatsMap()))

class AudioStreamPrivate
{
    public:
        AudioStream *self;
        qint64 m_pts {0};
        AkElementPtr m_audioConvert;
        qreal audioDiffCum {0.0}; // used for AV difference average computation
        qreal audioDiffAvgCoef {exp(log(0.01) / AUDIO_DIFF_AVG_NB)};
        int audioDiffAvgCount {0};

        explicit AudioStreamPrivate(AudioStream *self);
        bool compensate(AVFrame *oFrame, AVFrame *iFrame, int wantedSamples);
        AkPacket frameToPacket(AVFrame *iFrame);
        AkPacket convert(AVFrame *iFrame);
        AVFrame *copyFrame(AVFrame *frame) const;
};

AudioStream::AudioStream(const AVFormatContext *formatContext,
                         uint index, qint64 id, Clock *globalClock,
                         bool noModify, QObject *parent):
    AbstractStream(formatContext, index, id, globalClock, noModify, parent)
{
    this->d = new AudioStreamPrivate(this);
    this->m_maxData = 9;
    this->d->m_audioConvert = AkElement::create("ACapsConvert");
}

AudioStream::~AudioStream()
{
    delete this->d;
}

AkCaps AudioStream::caps() const
{
    auto iFormat = AVSampleFormat(this->codecContext()->sample_fmt);
    auto oFormat = av_get_packed_sample_fmt(iFormat);
    oFormat = sampleFormats->contains(oFormat)? oFormat: AV_SAMPLE_FMT_FLT;

    AkAudioCaps caps(sampleFormats->value(oFormat),
                     channelLayouts->key(this->codecContext()->channel_layout,
                                         AkAudioCaps::Layout_stereo),
                     this->codecContext()->sample_rate);

    return caps.toCaps();
}

void AudioStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    if (!packet) {
        this->dataEnqueue(nullptr);

        return;
    }

#ifdef HAVE_SENDRECV
    if (avcodec_send_packet(this->codecContext(), packet) >= 0)
        forever {
            auto iFrame = av_frame_alloc();
            int r = avcodec_receive_frame(this->codecContext(), iFrame);

            if (r >= 0)
                this->dataEnqueue(this->d->copyFrame(iFrame));

            av_frame_free(&iFrame);

            if (r < 0)
                break;
        }
#else
        auto iFrame = av_frame_alloc();
        int gotFrame;
        avcodec_decode_audio4(this->codecContext(), iFrame, &gotFrame, packet);

        if (gotFrame)
            this->dataEnqueue(this->d->copyFrame(iFrame));

        av_frame_free(&iFrame);
#endif
}

void AudioStream::processData(AVFrame *frame)
{
    frame->pts = frame->pts != AV_NOPTS_VALUE? frame->pts: this->d->m_pts;
    AkPacket oPacket = this->d->convert(frame);
    emit this->oStream(oPacket);
    emit this->frameSent();
    this->d->m_pts = frame->pts + frame->nb_samples;
}

AudioStreamPrivate::AudioStreamPrivate(AudioStream *self):
    self(self)
{
}

bool AudioStreamPrivate::compensate(AVFrame *oFrame,
                                    AVFrame *iFrame,
                                    int wantedSamples)
{
    if (wantedSamples == iFrame->nb_samples)
        return false;

    int iChannels = av_get_channel_layout_nb_channels(iFrame->channel_layout);

    if (av_samples_alloc(oFrame->data,
                         iFrame->linesize,
                         iChannels,
                         wantedSamples,
                         AVSampleFormat(iFrame->format),
                         1) < 0) {
        return false;
    }

    if (av_samples_copy(oFrame->data,
                        iFrame->data,
                        0,
                        0,
                        qMin(wantedSamples, iFrame->nb_samples),
                        iChannels,
                        AVSampleFormat(iFrame->format)) < 0) {
        av_freep(&oFrame->data[0]);
        av_frame_unref(oFrame);

        return false;
    }

    oFrame->format = iFrame->format;
    oFrame->channel_layout = iFrame->channel_layout;
    oFrame->sample_rate = iFrame->sample_rate;
    oFrame->nb_samples = wantedSamples;
    oFrame->pts = iFrame->pts;

    return true;
}

AkPacket AudioStreamPrivate::frameToPacket(AVFrame *iFrame)
{
    int iChannels = av_get_channel_layout_nb_channels(iFrame->channel_layout);

    AVFrame frame;
    memset(&frame, 0, sizeof(AVFrame));

    int frameSize = av_samples_get_buffer_size(nullptr,
                                               iChannels,
                                               iFrame->nb_samples,
                                               AVSampleFormat(iFrame->format),
                                               1);

    QByteArray iBuffer(frameSize, 0);

    if (av_samples_fill_arrays(frame.data,
                               frame.linesize,
                               reinterpret_cast<const uint8_t *>(iBuffer.constData()),
                               iChannels,
                               iFrame->nb_samples,
                               AVSampleFormat(iFrame->format),
                               1) < 0) {
        return AkPacket();
    }

    if (av_samples_copy(frame.data,
                        iFrame->data,
                        0,
                        0,
                        iFrame->nb_samples,
                        iChannels,
                        AVSampleFormat(iFrame->format)) < 0) {
        return AkPacket();
    }

    AkAudioPacket packet;
    packet.caps() =
            AkAudioCaps(sampleFormats->value(AVSampleFormat(iFrame->format)),
                        channelLayouts->key(iFrame->channel_layout),
                        iFrame->sample_rate,
                        iFrame->nb_samples);

    packet.buffer() = iBuffer;
    packet.pts() = iFrame->pts;
    packet.timeBase() = self->timeBase();
    packet.index() = int(self->index());
    packet.id() = self->id();

    return packet.toPacket();
}

AkPacket AudioStreamPrivate::convert(AVFrame *iFrame)
{
    if (this->m_audioConvert->state() != AkElement::ElementStatePlaying) {
        auto format = sampleFormats->value(AVSampleFormat(iFrame->format),
                                           AkAudioCaps::SampleFormat_flt);

        if (AkAudioCaps::bitsPerSample(format) > 32)
            format = AkAudioCaps::SampleFormat_flt;

        int iChannels =
                av_get_channel_layout_nb_channels(iFrame->channel_layout);

        AkAudioCaps caps(format,
                         iChannels > 1?
                             AkAudioCaps::Layout_stereo:
                             AkAudioCaps::Layout_mono,
                         iFrame->sample_rate);
        this->m_audioConvert->setProperty("caps", caps.toString());
        this->m_audioConvert->setState(AkElement::ElementStatePlaying);
    }

    auto packet = this->frameToPacket(iFrame);

    // Synchronize audio
    qreal pts = iFrame->pts * self->timeBase().value();
    qreal diff = pts - self->globalClock()->clock();
    int wantedSamples = iFrame->nb_samples;

    if (!qIsNaN(diff) && qAbs(diff) < AV_NOSYNC_THRESHOLD) {
        this->audioDiffCum = diff + this->audioDiffAvgCoef * this->audioDiffCum;

        if (this->audioDiffAvgCount < AUDIO_DIFF_AVG_NB) {
            // not enough measures to have a correct estimate
            this->audioDiffAvgCount++;
        } else {
            // estimate the A-V difference
            qreal avgDiff = this->audioDiffCum * (1.0 - this->audioDiffAvgCoef);

            // since we do not have a precise anough audio fifo fullness,
            // we correct audio sync only if larger than this threshold
            qreal diffThreshold = 2.0 * iFrame->nb_samples / iFrame->sample_rate;

            if (qAbs(avgDiff) >= diffThreshold) {
                wantedSamples = iFrame->nb_samples + int(diff * iFrame->sample_rate);
                int minSamples = iFrame->nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                int maxSamples = iFrame->nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                wantedSamples = qBound(minSamples, wantedSamples, maxSamples);

                AVFrame oFrame;
                memset(&oFrame, 0, sizeof(AVFrame));

                if (this->compensate(&oFrame, iFrame, wantedSamples)) {
                    packet = this->frameToPacket(&oFrame);
                    av_freep(&oFrame.data[0]);
                    av_frame_unref(&oFrame);
                }
            }
        }
    } else {
        // Too big difference: may be initial PTS errors, so
        // reset A-V filter
        this->audioDiffAvgCount = 0;
        this->audioDiffCum = 0.0;
    }

    if (qAbs(diff) >= AV_NOSYNC_THRESHOLD)
        self->globalClock()->setClock(pts);

    self->clockDiff() = diff;

    return this->m_audioConvert->iStream(packet);
}

AVFrame *AudioStreamPrivate::copyFrame(AVFrame *frame) const
{
    auto oFrame = av_frame_alloc();
    oFrame->format = frame->format;
    oFrame->channel_layout = frame->channel_layout;
    oFrame->sample_rate = frame->sample_rate;
    oFrame->nb_samples = frame->nb_samples;
    oFrame->pts = frame->pts;
    int channels = av_get_channel_layout_nb_channels(oFrame->channel_layout);

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

#include "moc_audiostream.cpp"
