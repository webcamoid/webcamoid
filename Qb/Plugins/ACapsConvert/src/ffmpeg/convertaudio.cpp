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

#include "convertaudio.h"

typedef QMap<QbAudioCaps::SampleFormat, AVSampleFormat> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;
    sampleFormat[QbAudioCaps::SampleFormat_u8] = AV_SAMPLE_FMT_U8;
    sampleFormat[QbAudioCaps::SampleFormat_s16] = AV_SAMPLE_FMT_S16;
    sampleFormat[QbAudioCaps::SampleFormat_s32] = AV_SAMPLE_FMT_S32;
    sampleFormat[QbAudioCaps::SampleFormat_flt] = AV_SAMPLE_FMT_FLT;

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

typedef QMap<QbAudioCaps::ChannelLayout, int64_t> ChannelLayoutsMap;

inline ChannelLayoutsMap initChannelFormatsMap()
{
    ChannelLayoutsMap channelLayouts;
    channelLayouts[QbAudioCaps::Layout_mono] = AV_CH_LAYOUT_MONO;
    channelLayouts[QbAudioCaps::Layout_stereo] = AV_CH_LAYOUT_STEREO;

    return channelLayouts;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutsMap, channelLayouts, (initChannelFormatsMap()))

ConvertAudio::ConvertAudio(QObject *parent):
    QObject(parent)
{
    this->m_resampleContext = NULL;
}

ConvertAudio::~ConvertAudio()
{
    if (this->m_resampleContext)
        swr_free(&this->m_resampleContext);
}

QbPacket ConvertAudio::convert(const QbAudioPacket &packet,
                               const QbCaps &oCaps)
{
    QbAudioCaps oAudioCaps(oCaps);

    int64_t iSampleLayout = channelLayouts->value(packet.caps().layout(), 0);
    AVSampleFormat iSampleFormat = sampleFormats->value(packet.caps().format(),
                                                        AV_SAMPLE_FMT_NONE);
    int iSampleRate = packet.caps().rate();
    int iNChannels = packet.caps().channels();
    int iNSamples = packet.caps().samples();

    int64_t oSampleLayout = channelLayouts->value(oAudioCaps.layout(),
                                                  AV_CH_LAYOUT_STEREO);
    AVSampleFormat oSampleFormat = sampleFormats->value(oAudioCaps.format(),
                                                        AV_SAMPLE_FMT_FLT);
    int oSampleRate = oAudioCaps.rate();
    int oNChannels = oAudioCaps.channels();

    this->m_resampleContext =
            swr_alloc_set_opts(this->m_resampleContext,
                               oSampleLayout,
                               oSampleFormat,
                               oSampleRate,
                               iSampleLayout,
                               iSampleFormat,
                               iSampleRate,
                               0,
                               NULL);

    if (!this->m_resampleContext)
        return QbPacket();

    if (!swr_is_initialized(this->m_resampleContext))
        if (swr_init(this->m_resampleContext) < 0)
            return QbPacket();

    // Create input audio frame.
    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    if (av_samples_fill_arrays(iFrame.data,
                               iFrame.linesize,
                               (const uint8_t *) packet.buffer().data(),
                               iNChannels,
                               iNSamples,
                               iSampleFormat,
                               1) < 0)
        return QbPacket();

    iFrame.channels = iNChannels;
    iFrame.channel_layout = iSampleLayout;
    iFrame.format = iSampleFormat;
    iFrame.sample_rate = iSampleRate;
    iFrame.nb_samples = iNSamples;
    iFrame.pts = iFrame.pkt_pts = packet.pts();

    // Create output audio packet.
    int oNSamples = swr_get_delay(this->m_resampleContext, oSampleRate)
                    + iFrame.nb_samples
                    * (int64_t) oSampleRate
                    / iSampleRate
                    + 3;

    int oLineSize;
    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 oNChannels,
                                                 oNSamples,
                                                 oSampleFormat,
                                                 1);

    QByteArray oBuffer(oBufferSize, Qt::Uninitialized);

    int oNPlanes = av_sample_fmt_is_planar(oSampleFormat)? oNChannels: 1;
    QVector<uint8_t *> oData(oNPlanes);

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               oNChannels,
                               oNSamples,
                               oSampleFormat,
                               1) < 0)
        return QbPacket();

    int64_t oPts = swr_next_pts(this->m_resampleContext, iFrame.pts);

    // convert to destination format
    int outputSamples = swr_convert(this->m_resampleContext,
                                    oData.data(),
                                    oNSamples,
                                    (const uint8_t **) iFrame.data,
                                    iFrame.nb_samples);

    if (outputSamples < 1)
        return QbPacket();

    oBufferSize = oBufferSize * outputSamples / oNSamples;

    QbBufferPtr buffer(new char[oBufferSize]);
    memcpy(buffer.data(), oBuffer.data(), oBufferSize);

    QbAudioPacket oAudioPacket;
    oAudioPacket.caps() = oAudioCaps;
    oAudioPacket.caps().samples() = outputSamples;
    oAudioPacket.buffer() = buffer;
    oAudioPacket.bufferSize() = oBufferSize;
    oAudioPacket.pts() = oPts;
    oAudioPacket.timeBase() = QbFrac(1, oAudioCaps.rate());
    oAudioPacket.index() = packet.index();
    oAudioPacket.id() = packet.id();

    return oAudioPacket.toPacket();
}
