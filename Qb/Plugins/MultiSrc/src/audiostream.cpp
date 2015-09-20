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

// No AV correction is done if too big error.
#define AV_NOSYNC_THRESHOLD 10.0

typedef QMap<AVSampleFormat, QbAudioCaps::SampleFormat> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;
    sampleFormat[AV_SAMPLE_FMT_U8] = QbAudioCaps::SampleFormat_u8;
    sampleFormat[AV_SAMPLE_FMT_S16] = QbAudioCaps::SampleFormat_s16;
    sampleFormat[AV_SAMPLE_FMT_S32] = QbAudioCaps::SampleFormat_s32;
    sampleFormat[AV_SAMPLE_FMT_FLT] = QbAudioCaps::SampleFormat_flt;

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

AudioStream::AudioStream(const AVFormatContext *formatContext,
                         uint index, qint64 id, Clock *globalClock,
                         bool noModify, QObject *parent):
    AbstractStream(formatContext, index, id, globalClock, noModify, parent)
{
    this->m_pts = 0;
    this->m_resampleContext = NULL;
    this->m_run = false;
    this->m_frameBuffer.setMaxSize(9);
}

AudioStream::~AudioStream()
{
    if (this->m_resampleContext)
        swr_free(&this->m_resampleContext);
}

QbCaps AudioStream::caps() const
{
    AVSampleFormat iFormat = AVSampleFormat(this->codecContext()->sample_fmt);
    AVSampleFormat oFormat = av_get_packed_sample_fmt(iFormat);
    oFormat = sampleFormats->contains(oFormat)? oFormat: AV_SAMPLE_FMT_FLT;

    QbAudioCaps::ChannelLayout layout =
            channelLayouts->contains(this->codecContext()->channel_layout)?
                channelLayouts->value(this->codecContext()->channel_layout):
                QbAudioCaps::Layout_stereo;

    QbAudioCaps caps;
    caps.isValid() = true;
    caps.format() = sampleFormats->value(oFormat);;
    caps.bps() = av_get_bytes_per_sample(oFormat);
    caps.channels() = av_get_channel_layout_nb_channels(layout);
    caps.rate() = this->codecContext()->sample_rate;
    caps.layout() = layout;
    caps.align() = false;

    return caps.toCaps();
}

void AudioStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    AVFrame *iFrame = av_frame_alloc();
    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return;

#if 1
    this->m_frameBuffer.enqueue(iFrame);
#else
    QbPacket oPacket = this->convert(iFrame);
    av_frame_unref(iFrame);
    av_frame_free(&iFrame);

    emit this->oStream(oPacket);
#endif
}

QbPacket AudioStream::convert(AVFrame *iFrame)
{
    int64_t oLayout = channelLayouts->contains(iFrame->channel_layout)?
                          iFrame->channel_layout:
                          AV_CH_LAYOUT_STEREO;

    AVSampleFormat iFormat = AVSampleFormat(iFrame->format);
    AVSampleFormat oFormat = av_get_packed_sample_fmt(iFormat);
    oFormat = sampleFormats->contains(oFormat)? oFormat: AV_SAMPLE_FMT_FLT;

    this->m_resampleContext =
            swr_alloc_set_opts(this->m_resampleContext,
                               oLayout,
                               oFormat,
                               iFrame->sample_rate,
                               iFrame->channel_layout,
                               iFormat,
                               iFrame->sample_rate,
                               0,
                               NULL);

    if (!this->m_resampleContext)
        return QbPacket();

    qint64 pts = (iFrame->pts != AV_NOPTS_VALUE) ? iFrame->pts :
                  (iFrame->pkt_pts != AV_NOPTS_VALUE) ? iFrame->pkt_pts :
                  this->m_pts;
    iFrame->pts = iFrame->pkt_pts = pts;

    int64_t clock = this->globalClock()->clock()
                    / this->timeBase().value();

    int64_t oPts = swr_next_pts(this->m_resampleContext, clock);

    AVFrame *oFrame = av_frame_alloc();
    oFrame->channel_layout = oLayout;
    oFrame->format = oFormat;
    oFrame->sample_rate = iFrame->sample_rate;

    if (swr_convert_frame(this->m_resampleContext,
                          oFrame,
                          iFrame) < 0)
        return QbPacket();

    int oSamples = oFrame->nb_samples;
    int oChannels = av_get_channel_layout_nb_channels(oLayout);

    int oLineSize;
    int frameSize = av_samples_get_buffer_size(&oLineSize,
                                               oChannels,
                                               oSamples,
                                               oFormat,
                                               1);

    QbBufferPtr oBuffer(new char[frameSize]);
    uint8_t *oData;

    if (av_samples_fill_arrays(&oData,
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               oChannels,
                               oSamples,
                               oFormat,
                               1) < 0)
        return QbPacket();

    if (av_samples_copy(&oData,
                        oFrame->data,
                        0,
                        0,
                        oChannels,
                        oSamples,
                        oFormat) < 0)
        return QbPacket();

    QbAudioPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = sampleFormats->value(oFormat);
    packet.caps().bps() = av_get_bytes_per_sample(oFormat);
    packet.caps().channels() = oChannels;
    packet.caps().rate() = iFrame->sample_rate;
    packet.caps().layout() = channelLayouts->value(oLayout);
    packet.caps().samples() = oSamples;
    packet.caps().align() = false;

    packet.buffer() = oBuffer;
    packet.bufferSize() = frameSize;
    packet.pts() = oPts;
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    int64_t duration = av_frame_get_pkt_duration(iFrame);

    if (duration == AV_NOPTS_VALUE)
        duration = oSamples
                   / (iFrame->sample_rate
                      * this->timeBase().value());

    this->m_pts += duration;
    av_frame_free(&oFrame);

    qreal diff = oPts * this->timeBase().value() - this->globalClock()->clock();
    this->m_clockDiff = diff;

    if (qAbs(diff) >= AV_NOSYNC_THRESHOLD)
        this->globalClock()->setClock(oPts * this->timeBase().value());

    return packet.toPacket();
}

void AudioStream::sendPacket(AudioStream *stream)
{
    while (stream->m_run) {
        AVFramePtr frame = stream->m_frameBuffer.dequeue();

        if (!frame)
            continue;

        QbPacket oPacket = stream->convert(frame.data());

        emit stream->oStream(oPacket);
        emit stream->frameSent();
    }
}

void AudioStream::init()
{
    AbstractStream::init();
    this->m_run = true;
    this->m_pts = 0;

    QtConcurrent::run(&this->m_threadPool, this->sendPacket, this);
}

void AudioStream::uninit()
{
    this->m_run = false;
    this->m_frameBuffer.clear();
    this->m_threadPool.waitForDone();
    AbstractStream::uninit();
}
