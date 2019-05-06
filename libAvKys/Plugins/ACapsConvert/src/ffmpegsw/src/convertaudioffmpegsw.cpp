/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QMap>
#include <QMutex>
#include <akaudiocaps.h>
#include <akpacket.h>
#include <akaudiopacket.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
    #include <libswresample/swresample.h>
}

#include "convertaudioffmpegsw.h"

using ChannelLayoutsMap = QMap<AkAudioCaps::ChannelLayout, int64_t>;

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
        {AkAudioCaps::Layout_hexadecagonal, AV_CH_LAYOUT_HEXADECAGONAL    },
        {AkAudioCaps::Layout_downmix      , AV_CH_LAYOUT_STEREO_DOWNMIX   },
    };

    return channelLayouts;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutsMap, channelLayouts, (initChannelFormatsMap()))

class ConvertAudioFFmpegSWPrivate
{
    public:
        AkAudioCaps m_caps;
        SwrContext *m_resampleContext {nullptr};
        QMutex m_mutex;
};

ConvertAudioFFmpegSW::ConvertAudioFFmpegSW(QObject *parent):
    ConvertAudio(parent)
{
    this->d = new ConvertAudioFFmpegSWPrivate;

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif
}

ConvertAudioFFmpegSW::~ConvertAudioFFmpegSW()
{
    this->uninit();
    delete this->d;
}

bool ConvertAudioFFmpegSW::init(const AkAudioCaps &caps)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);
    this->d->m_caps = caps;

    return true;
}

AkPacket ConvertAudioFFmpegSW::convert(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_caps)
        return AkPacket();

    int64_t iSampleLayout = channelLayouts->value(packet.caps().layout(), 0);

    AVSampleFormat iSampleFormat =
            av_get_sample_fmt(AkAudioCaps::sampleFormatToString(packet.caps().format())
                              .toStdString().c_str());

    int iSampleRate = packet.caps().rate();
    int iNChannels = packet.caps().channels();
    int iNSamples = packet.caps().samples();

    int64_t oSampleLayout = channelLayouts->value(this->d->m_caps.layout(),
                                                  AV_CH_LAYOUT_STEREO);

    AVSampleFormat oSampleFormat =
            av_get_sample_fmt(AkAudioCaps::sampleFormatToString(this->d->m_caps.format())
                              .toStdString().c_str());

    int oSampleRate = this->d->m_caps.rate();
    int oNChannels = this->d->m_caps.channels();

    this->d->m_resampleContext =
            swr_alloc_set_opts(this->d->m_resampleContext,
                               oSampleLayout,
                               oSampleFormat,
                               oSampleRate,
                               iSampleLayout,
                               iSampleFormat,
                               iSampleRate,
                               0,
                               nullptr);

    if (!this->d->m_resampleContext)
        return AkPacket();

    // Create input audio frame.
    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = iSampleFormat;
    iFrame.channels = iNChannels;
    iFrame.channel_layout = uint64_t(iSampleLayout);
    iFrame.sample_rate = iSampleRate;
    iFrame.nb_samples = iNSamples;
    iFrame.pts = packet.pts();

    if (avcodec_fill_audio_frame(&iFrame,
                                 iFrame.channels,
                                 iSampleFormat,
                                 reinterpret_cast<const uint8_t *>(packet.buffer().constData()),
                                 packet.buffer().size(),
                                 packet.caps().align()) < 0) {
        return AkPacket();
    }

    // Fill output audio frame.
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));
    oFrame.format = oSampleFormat;
    oFrame.channels = oNChannels;
    oFrame.channel_layout = uint64_t(oSampleLayout);
    oFrame.sample_rate = oSampleRate;
    oFrame.nb_samples = int(swr_get_delay(this->d->m_resampleContext, oSampleRate))
                        + iFrame.nb_samples
                        * oSampleRate
                        / iSampleRate
                        + 3;
    oFrame.pts = iFrame.pts * oSampleRate / iSampleRate;

    // Calculate the size of the audio buffer.
    int frameSize = av_samples_get_buffer_size(oFrame.linesize,
                                               oFrame.channels,
                                               oFrame.nb_samples,
                                               oSampleFormat,
                                               1);

    QByteArray oBuffer(frameSize, 0);

    if (avcodec_fill_audio_frame(&oFrame,
                                 oFrame.channels,
                                 oSampleFormat,
                                 reinterpret_cast<const uint8_t *>(oBuffer.constData()),
                                 oBuffer.size(),
                                 1) < 0) {
        return AkPacket();
    }

    // convert to destination format
    if (swr_convert_frame(this->d->m_resampleContext,
                          &oFrame,
                          &iFrame) < 0)
        return AkPacket();

    frameSize = av_samples_get_buffer_size(oFrame.linesize,
                                           oFrame.channels,
                                           oFrame.nb_samples,
                                           oSampleFormat,
                                           1);

    oBuffer.resize(frameSize);

    AkAudioPacket oAudioPacket;
    oAudioPacket.caps() = this->d->m_caps;
    oAudioPacket.caps().setSamples(oFrame.nb_samples);
    oAudioPacket.buffer() = oBuffer;
    oAudioPacket.pts() = oFrame.pts;
    oAudioPacket.timeBase() = AkFrac(1, this->d->m_caps.rate());
    oAudioPacket.index() = packet.index();
    oAudioPacket.id() = packet.id();

    return oAudioPacket.toPacket();
}

void ConvertAudioFFmpegSW::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);
    this->d->m_caps = AkAudioCaps();

    if (this->d->m_resampleContext)
        swr_free(&this->d->m_resampleContext);
}

#include "moc_convertaudioffmpegsw.cpp"
