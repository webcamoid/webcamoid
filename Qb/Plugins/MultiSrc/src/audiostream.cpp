/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QAudioOutput>

#include "audiostream.h"

typedef QMap<AVSampleFormat, QbAudioCaps::SampleFormat> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;
    sampleFormat[AV_SAMPLE_FMT_U8] = QbAudioCaps::Format_u8;
    sampleFormat[AV_SAMPLE_FMT_S16] = QbAudioCaps::Format_s16;
    sampleFormat[AV_SAMPLE_FMT_S32] = QbAudioCaps::Format_s32;
    sampleFormat[AV_SAMPLE_FMT_FLT] = QbAudioCaps::Format_flt;

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

typedef QMap<int64_t, QbAudioCaps::ChannelLayout> ChannelLayoutsMap;

inline ChannelLayoutsMap initChannelFormatsMap()
{
    ChannelLayoutsMap channelLayouts;
    channelLayouts[AV_CH_LAYOUT_MONO] = QbAudioCaps::Layout_mono;
    channelLayouts[AV_CH_LAYOUT_STEREO] = QbAudioCaps::Layout_stereo;

    return channelLayouts;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutsMap, channelLayouts, (initChannelFormatsMap()))

typedef QMap<AVSampleFormat, int> BpsMap;

inline BpsMap initBpsMap()
{
    BpsMap bps;
    bps[AV_SAMPLE_FMT_U8] = 1;
    bps[AV_SAMPLE_FMT_S16] = 2;
    bps[AV_SAMPLE_FMT_S32] = 4;
    bps[AV_SAMPLE_FMT_FLT] = 4;

    return bps;
}

Q_GLOBAL_STATIC_WITH_ARGS(BpsMap, bytesPerSample, (initBpsMap()))

typedef QMap<int64_t, int> NChannelsMap;

inline NChannelsMap initNChannelsMap()
{
    NChannelsMap nChannels;
    nChannels[AV_CH_LAYOUT_MONO] = 1;
    nChannels[AV_CH_LAYOUT_STEREO] = 2;

    return nChannels;
}

Q_GLOBAL_STATIC_WITH_ARGS(NChannelsMap, NChannels, (initNChannelsMap()))

AudioStream::AudioStream(const AVFormatContext *formatContext,
                         uint index, qint64 id, bool noModify, QObject *parent):
    AbstractStream(formatContext, index, id, noModify, parent)
{
    this->m_fst = true;
    this->m_resampleContext = NULL;
    this->m_frameBuffer.setMaxSize(9);
}

AudioStream::~AudioStream()
{
    if (this->m_resampleContext)
        swr_free(&this->m_resampleContext);
}

QbCaps AudioStream::caps() const
{
    QbAudioCaps::SampleFormat format =
            sampleFormats->contains(this->codecContext()->sample_fmt)?
                sampleFormats->value(this->codecContext()->sample_fmt):
                QbAudioCaps::Format_flt;

    QbAudioCaps::ChannelLayout layout =
            channelLayouts->contains(this->codecContext()->channel_layout)?
                channelLayouts->value(this->codecContext()->channel_layout):
                QbAudioCaps::Layout_stereo;

    QbAudioCaps caps;
    caps.isValid() = true;
    caps.format() = format;
    caps.bps() = bytesPerSample->value(sampleFormats->key(format));
    caps.channels() = NChannels->value(layout);
    caps.rate() = this->codecContext()->sample_rate;
    caps.layout() = layout;
    caps.align() = false;

    return caps.toCaps();
}

void AudioStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return;

    if (this->m_fst) {
        this->m_pts = 0;
        this->m_duration = av_frame_get_pkt_duration(&iFrame);
        this->m_fst = false;
    }
    else
        this->m_pts += this->m_duration;

    qint64 pts = (iFrame.pts != AV_NOPTS_VALUE) ? iFrame.pts :
                  (iFrame.pkt_pts != AV_NOPTS_VALUE) ? iFrame.pkt_pts :
                  this->m_pts;

    iFrame.pts = pts;
    iFrame.pkt_pts = pts;

    QbPacket oPacket = this->convert(&iFrame);

    emit this->oStream(oPacket);
}

QbPacket AudioStream::convert(AVFrame *iFrame)
{
    int64_t oLayout = channelLayouts->contains(iFrame->channel_layout)?
                          iFrame->channel_layout:
                          AV_CH_LAYOUT_STEREO;

    AVSampleFormat oFormat = sampleFormats->contains(AVSampleFormat(iFrame->format))?
                                 AVSampleFormat(iFrame->format):
                                 AV_SAMPLE_FMT_FLT;

    this->m_resampleContext =
            swr_alloc_set_opts(this->m_resampleContext,
                               oLayout,
                               oFormat,
                               iFrame->sample_rate,
                               iFrame->channel_layout,
                               AVSampleFormat(iFrame->format),
                               iFrame->sample_rate,
                               0,
                               NULL);

    if (!this->m_resampleContext)
        return QbPacket();

    AVFrame *oFrame = av_frame_alloc();
    oFrame->channel_layout = oLayout;
    oFrame->format = oFormat;
    oFrame->sample_rate = iFrame->sample_rate;

    if (swr_convert_frame(this->m_resampleContext,
                          oFrame,
                          iFrame) < 0)
        return QbPacket();

    QbAudioPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = sampleFormats->value(oFormat);
    packet.caps().bps() = bytesPerSample->value(oFormat);
    packet.caps().channels() = NChannels->value(oLayout);
    packet.caps().rate() = iFrame->sample_rate;
    packet.caps().layout() = channelLayouts->value(oLayout);
    packet.caps().samples() = oFrame->nb_samples;
    packet.caps().align() = false;

    int oLineSize;
    int frameSize = av_samples_get_buffer_size(&oLineSize,
                                               packet.caps().channels(),
                                               packet.caps().samples(),
                                               oFormat,
                                               1);

    QbBufferPtr oBuffer(new char[frameSize]);
    uint8_t *oData;

    if (av_samples_fill_arrays(&oData,
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               packet.caps().channels(),
                               packet.caps().samples(),
                               oFormat,
                               1) < 0)
        return QbPacket();

    if (av_samples_copy(&oData,
                        oFrame->data,
                        0,
                        0,
                        packet.caps().channels(),
                        packet.caps().samples(),
                        oFormat) < 0)
        return QbPacket();

    packet.buffer() = oBuffer;
    packet.bufferSize() = frameSize;
    packet.pts() = av_frame_get_best_effort_timestamp(iFrame);
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    av_frame_free(&oFrame);

    return packet.toPacket();
}
