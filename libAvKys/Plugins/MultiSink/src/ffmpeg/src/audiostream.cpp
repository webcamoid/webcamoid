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

#include "mediawriterffmpeg.h"
#include "audiostream.h"

typedef QMap<AkAudioCaps::ChannelLayout, uint64_t> AkFFChannelLayoutsMap;

inline AkFFChannelLayoutsMap initAkFFChannelFormatsMap()
{
    AkFFChannelLayoutsMap channelLayouts = {
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

Q_GLOBAL_STATIC_WITH_ARGS(AkFFChannelLayoutsMap, akFFChannelLayouts, (initAkFFChannelFormatsMap()))

AudioStream::AudioStream(const AVFormatContext *formatContext,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         const QMap<QString, QVariantMap> &codecOptions,
                         MediaWriterFFmpeg *mediaWriter,
                         QObject *parent):
    AbstractStream(formatContext,
                   index, streamIndex,
                   configs,
                   codecOptions,
                   mediaWriter,
                   parent)
{
    this->m_frame = NULL;
    auto codecContext = this->codecContext();
    auto codec = codecContext->codec;
    auto defaultCodecParams = mediaWriter->defaultCodecParams(codec->name);
    codecContext->bit_rate = configs["bitrate"].toInt();

    if (codecContext->bit_rate < 1)
        codecContext->bit_rate = defaultCodecParams["defaultBitRate"].toInt();

    switch (codec->id) {
    case AV_CODEC_ID_G723_1:
        codecContext->bit_rate = 6300;
        break;
    case AV_CODEC_ID_GSM_MS:
        codecContext->bit_rate = 13000;
        break;
    default:
        break;
    }

    AkAudioCaps audioCaps(configs["caps"].value<AkCaps>());

    QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
    QStringList supportedSampleFormats = defaultCodecParams["supportedSampleFormats"].toStringList();

    if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
        QString defaultSampleFormat = defaultCodecParams["defaultSampleFormat"].toString();
        audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
        audioCaps.bps() = 8 * av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
    }

    QVariantList supportedSampleRates = defaultCodecParams["supportedSampleRates"].toList();

    if (!supportedSampleRates.isEmpty()) {
        int sampleRate = 0;
        int maxDiff = std::numeric_limits<int>::max();

        for (const QVariant &rate: supportedSampleRates) {
            int diff = qAbs(audioCaps.rate() - rate.toInt());

            if (diff < maxDiff) {
                sampleRate = rate.toInt();

                if (!diff)
                    break;

                maxDiff = diff;
            }
        }

        audioCaps.rate() = sampleRate;
    }

    QString channelLayout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
    QStringList supportedChannelLayouts = defaultCodecParams["supportedChannelLayouts"].toStringList();

    if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
        QString defaultChannelLayout = defaultCodecParams["defaultChannelLayout"].toString();
        audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
        audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
    }

    if (!strcmp(formatContext->oformat->name, "gxf")) {
        audioCaps.rate() = 48000;
        audioCaps.layout() = AkAudioCaps::Layout_mono;
        audioCaps.channels() = 1;
    } else if (!strcmp(formatContext->oformat->name, "mxf")) {
        audioCaps.rate() = 48000;
    } else if (!strcmp(formatContext->oformat->name, "swf")) {
        audioCaps = mediaWriter->nearestSWFCaps(audioCaps);
    }

    QString sampleFormatStr = AkAudioCaps::sampleFormatToString(audioCaps.format());
    codecContext->sample_fmt = av_get_sample_fmt(sampleFormatStr.toStdString().c_str());
    codecContext->sample_rate = audioCaps.rate();
    QString layout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
    codecContext->channel_layout = av_get_channel_layout(layout.toStdString().c_str());
    codecContext->channels = audioCaps.channels();

    auto stream = this->stream();
    stream->time_base.num = 1;
    stream->time_base.den = audioCaps.rate();
    codecContext->time_base = stream->time_base;

    this->m_convert = AkElement::create("ACapsConvert");

    auto fmtName = av_get_sample_fmt_name(codecContext->sample_fmt);
    AkAudioCaps caps(AkAudioCaps::sampleFormatFromString(fmtName),
                     codecContext->channels,
                     codecContext->sample_rate);
    caps.layout() = akFFChannelLayouts->key(codecContext->channel_layout);
    this->m_convert->setProperty("caps", caps.toString());

    this->m_frameSize =
            codecContext->codec->capabilities
                          & CODEC_CAP_VARIABLE_FRAME_SIZE?
                              1024:
                              codecContext->frame_size;

    this->m_maxFrameQueueSize = 16 * this->m_frameSize;
}

AudioStream::~AudioStream()
{
    this->uninit();
}

void AudioStream::convertPacket(const AkPacket &packet)
{
    if (!packet)
        return;

    auto codecContext = this->codecContext();
    auto iPacket = AkAudioPacket(this->m_convert->iStream(packet));

    if (!iPacket)
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = codecContext->sample_fmt;
    iFrame.channel_layout = codecContext->channel_layout;
    iFrame.sample_rate = codecContext->sample_rate;
    iFrame.nb_samples = iPacket.caps().samples();
    iFrame.pts = iPacket.pts();
    int channels = av_get_channel_layout_nb_channels(iFrame.channel_layout);

    if (av_samples_fill_arrays(iFrame.data,
                               iFrame.linesize,
                               reinterpret_cast<const uint8_t *>(iPacket.buffer().constData()),
                               channels,
                               iPacket.caps().samples(),
                               AVSampleFormat(iFrame.format),
                               1) < 0) {
        return;
    }

    // Create new buffer.
    auto oFrame = av_frame_alloc();

    if (av_samples_alloc(oFrame->data,
                         oFrame->linesize,
                         channels,
                         iFrame.nb_samples + this->m_frame->nb_samples,
                         AVSampleFormat(iFrame.format),
                         1) < 0) {
        return;
    }

    // Copy converted samples to the new buffer.
    if (av_samples_copy(oFrame->data,
                        this->m_frame->data,
                        0,
                        0,
                        this->m_frame->nb_samples,
                        channels,
                        AVSampleFormat(iFrame.format)) < 0) {
        return;
    }

    // Copy old samples to new buffer.
    if (av_samples_copy(oFrame->data,
                        iFrame.data,
                        this->m_frame->nb_samples,
                        0,
                        iFrame.nb_samples,
                        channels,
                        AVSampleFormat(iFrame.format)) < 0) {
        return;
    }

    oFrame->format = codecContext->sample_fmt;
    oFrame->channel_layout = codecContext->channel_layout;
    oFrame->sample_rate = codecContext->sample_rate;
    oFrame->nb_samples = iFrame.nb_samples + this->m_frame->nb_samples;
    oFrame->pts = this->m_frame->pts;
    av_frame_free(&this->m_frame);
    this->m_frame = oFrame;
    this->m_frameQueueSize += iFrame.nb_samples;
}

AbstractStream::PacketStatus AudioStream::encodeData(AVFrame *frame)
{
    auto codecContext = this->codecContext();

    /*
    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    qint64 pts = qRound64(QDateTime::currentMSecsSinceEpoch()
                          * outTimeBase.value()
                          / 1000);

    if (this->m_refPts == AV_NOPTS_VALUE)
        this->m_lastPts = this->m_refPts = pts;
    else if (this->m_lastPts != pts)
        this->m_lastPts = pts;
    else
        return;

    frame->pts = this->m_lastPts - this->m_refPts;
    */

    // Compress audio packet.
#ifdef HAVE_SENDRECV
    int result = avcodec_send_frame(codecContext, frame);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qDebug() << "Error: " << error;

        return PacketStatusError;
    }

    auto stream = this->stream();
    auto status = PacketStatusSent;

    forever {
        // Initialize audio packet.
        AVPacket pkt;
        memset(&pkt, 0, sizeof(AVPacket));
        av_init_packet(&pkt);

        if (avcodec_receive_packet(codecContext, &pkt) < 0)
            break;

        pkt.stream_index = this->streamIndex();
        this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);

        // Write the compressed frame to the media file.
        emit this->packetReady(&pkt);
        status = PacketStatusWritten;
    }

    return status;
#else
    // Initialize audio packet.
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));
    av_init_packet(&pkt);

    int gotPacket;
    int result = avcodec_encode_audio2(codecContext,
                                       &pkt,
                                       frame,
                                       &gotPacket);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qDebug() << "Error: " << error;

        return PacketStatusError;
    }

    if (!gotPacket)
        return PacketStatusSent;

    pkt.stream_index = this->streamIndex();
    this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);

    // Write the compressed frame to the media file.
    emit this->packetReady(&pkt);

    return PacketStatusWritten;
#endif
}

AVFrame *AudioStream::dequeueFrame()
{
    /* NOTE: Allocating and copying frames when enqueuing/dequeuing is pretty
     * slow, it can improved creating a fixed size frame, and then playing with
     * read write pointers.
     */

    auto codecContext = this->codecContext();

    // Create output buffer.
    auto oFrame = av_frame_alloc();
    oFrame->format = codecContext->sample_fmt;
    oFrame->channel_layout = codecContext->channel_layout;
    oFrame->sample_rate = codecContext->sample_rate;
    oFrame->nb_samples = this->m_frameSize;
    oFrame->pts = this->m_frame->pts;
    int channels = av_get_channel_layout_nb_channels(oFrame->channel_layout);

    if (av_samples_alloc(oFrame->data,
                         oFrame->linesize,
                         channels,
                         this->m_frameSize,
                         AVSampleFormat(oFrame->format),
                         1) < 0) {
        return NULL;
    }

    // Copy samples to the output buffer.
    if (av_samples_copy(oFrame->data,
                        this->m_frame->data,
                        0,
                        0,
                        this->m_frameSize,
                        channels,
                        AVSampleFormat(oFrame->format)) < 0) {
        return NULL;
    }

    // Create new buffer.
    auto frame = av_frame_alloc();
    frame->format = codecContext->sample_fmt;
    frame->channel_layout = codecContext->channel_layout;
    frame->sample_rate = codecContext->sample_rate;
    frame->nb_samples = this->m_frame->nb_samples - this->m_frameSize;
    frame->pts = this->m_frame->pts + this->m_frameSize;

    if (av_samples_alloc(frame->data,
                         frame->linesize,
                         channels,
                         this->m_frameSize,
                         AVSampleFormat(frame->format),
                         1) < 0) {
        return NULL;
    }

    // Copy samples to the output buffer.
    if (av_samples_copy(frame->data,
                        this->m_frame->data,
                        0,
                        this->m_frameSize,
                        frame->nb_samples,
                        channels,
                        AVSampleFormat(frame->format)) < 0) {
        return NULL;
    }

    av_frame_free(&this->m_frame);
    this->m_frame = frame;
    this->m_frameQueueSize -= this->m_frameSize;

    return oFrame;
}

bool AudioStream::init()
{
    this->m_convert->setState(AkElement::ElementStatePlaying);
    auto result = AbstractStream::init();

    if (!result)
        this->m_convert->setState(AkElement::ElementStateNull);

    return result;
}

void AudioStream::uninit()
{
    AbstractStream::uninit();
    this->m_convert->setState(AkElement::ElementStateNull);
    av_frame_free(&this->m_frame);
}
