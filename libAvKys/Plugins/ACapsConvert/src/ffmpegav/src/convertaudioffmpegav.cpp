/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "convertaudioffmpegav.h"

typedef QMap<AkAudioCaps::ChannelLayout, uint64_t> ChannelLayoutsMap;

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

ConvertAudioFFmpegAV::ConvertAudioFFmpegAV(QObject *parent):
    ConvertAudio(parent)
{
    this->m_resampleContext = NULL;
    this->m_contextIsOpen = false;

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif
}

ConvertAudioFFmpegAV::~ConvertAudioFFmpegAV()
{
    this->uninit();
}

bool ConvertAudioFFmpegAV::init(const AkAudioCaps &caps)
{
    QMutexLocker mutexLocker(&this->m_mutex);
    this->m_caps = caps;
    this->m_resampleContext = avresample_alloc_context();

    return true;
}

AkPacket ConvertAudioFFmpegAV::convert(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_caps)
        return AkPacket();

    uint64_t iSampleLayout = channelLayouts->value(packet.caps().layout(), 0);

    AVSampleFormat iSampleFormat =
            av_get_sample_fmt(AkAudioCaps::sampleFormatToString(packet.caps().format())
                              .toStdString().c_str());

    int iSampleRate = packet.caps().rate();
    int iNChannels = packet.caps().channels();
    int iNSamples = packet.caps().samples();

    uint64_t oSampleLayout = channelLayouts->value(this->m_caps.layout(),
                                                   AV_CH_LAYOUT_STEREO);

    AVSampleFormat oSampleFormat =
            av_get_sample_fmt(AkAudioCaps::sampleFormatToString(this->m_caps.format())
                              .toStdString().c_str());

    int oSampleRate = this->m_caps.rate();
    int oNChannels = this->m_caps.channels();

    // Create input audio frame.
    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = iSampleFormat;
    iFrame.channel_layout = uint64_t(iSampleLayout);
    iFrame.sample_rate = iSampleRate;
    iFrame.nb_samples = iNSamples;
    iFrame.pts = packet.pts();

    int iFrameSize = av_samples_get_buffer_size(iFrame.linesize,
                                                iNChannels,
                                                iFrame.nb_samples,
                                                iSampleFormat,
                                                1);

    if (avcodec_fill_audio_frame(&iFrame,
                                 iNChannels,
                                 iSampleFormat,
                                 reinterpret_cast<const uint8_t *>(packet.buffer().constData()),
                                 packet.buffer().size(),
                                 1) < 0) {
        return AkPacket();
    }

    // Fill output audio frame.
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));
    oFrame.format = oSampleFormat;
    oFrame.channel_layout = uint64_t(oSampleLayout);
    oFrame.sample_rate = oSampleRate;
    oFrame.nb_samples = int(avresample_get_delay(this->m_resampleContext))
                        + iFrame.nb_samples
                        * oSampleRate
                        / iSampleRate
                        + 3;
    oFrame.pts = iFrame.pts * oSampleRate / iSampleRate;

    // Calculate the size of the audio buffer.
    int oFrameSize = av_samples_get_buffer_size(oFrame.linesize,
                                                oNChannels,
                                                oFrame.nb_samples,
                                                oSampleFormat,
                                                1);

    QByteArray oBuffer(oFrameSize, Qt::Uninitialized);

    if (avcodec_fill_audio_frame(&oFrame,
                                 oNChannels,
                                 oSampleFormat,
                                 reinterpret_cast<const uint8_t *>(oBuffer.constData()),
                                 oBuffer.size(),
                                 1) < 0) {
        return AkPacket();
    }

    // convert to destination format
    if (!this->m_contextIsOpen) {
        // Configure output context
        av_opt_set_int(this->m_resampleContext,
                       "out_sample_fmt",
                       AVSampleFormat(oFrame.format),
                       0);
        av_opt_set_int(this->m_resampleContext,
                       "out_channel_layout",
                       int64_t(oFrame.channel_layout),
                       0);
        av_opt_set_int(this->m_resampleContext,
                       "out_sample_rate",
                       oFrame.sample_rate,
                       0);

        // Configure input context
        av_opt_set_int(this->m_resampleContext,
                       "in_sample_fmt",
                       AVSampleFormat(iFrame.format),
                       0);
        av_opt_set_int(this->m_resampleContext,
                       "in_channel_layout",
                       int64_t(iFrame.channel_layout),
                       0);
        av_opt_set_int(this->m_resampleContext,
                       "in_sample_rate",
                       iFrame.sample_rate,
                       0);

        if (avresample_open(this->m_resampleContext) < 0)
            return AkPacket();

        this->m_contextIsOpen = true;
    }

    int oSamples = avresample_convert(this->m_resampleContext,
                                      oFrame.data,
                                      oFrameSize,
                                      oFrame.nb_samples,
                                      iFrame.data,
                                      iFrameSize,
                                      iFrame.nb_samples);

    if (oSamples < 1)
        return AkPacket();

    oFrame.nb_samples = oSamples;

    oFrameSize = av_samples_get_buffer_size(oFrame.linesize,
                                           oNChannels,
                                           oFrame.nb_samples,
                                           oSampleFormat,
                                           1);

    oBuffer.resize(oFrameSize);

    AkAudioPacket oAudioPacket;
    oAudioPacket.caps() = this->m_caps;
    oAudioPacket.caps().samples() = oFrame.nb_samples;
    oAudioPacket.buffer() = oBuffer;
    oAudioPacket.pts() = oFrame.pts;
    oAudioPacket.timeBase() = AkFrac(1, this->m_caps.rate());
    oAudioPacket.index() = packet.index();
    oAudioPacket.id() = packet.id();

    return oAudioPacket.toPacket();
}

void ConvertAudioFFmpegAV::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);
    this->m_caps = AkAudioCaps();

    if (this->m_resampleContext)
        avresample_free(&this->m_resampleContext);

    this->m_contextIsOpen = false;
}
