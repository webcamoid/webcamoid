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

#include "outputparams.h"

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
        {AkAudioCaps::Layout_hexadecagonal, AV_CH_LAYOUT_HEXADECAGONAL    },
        {AkAudioCaps::Layout_downmix      , AV_CH_LAYOUT_STEREO_DOWNMIX   },
    };

    return channelLayouts;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutsMap, channelLayouts, (initChannelFormatsMap()))

OutputParams::OutputParams(int inputIndex,
                           AVCodecContext *codecContext,
                           QObject *parent):
    QObject(parent),
    m_inputIndex(inputIndex),
    m_audioFormat(AV_SAMPLE_FMT_NONE),
    m_audioChannels(0),
    m_id(-1),
    m_pts(0),
    m_ptsDiff(0),
    m_ptsDrift(0),
    m_scaleContext(NULL)
{
    this->m_codecContext = CodecContextPtr(codecContext,
                                           this->deleteCodecContext);
    this->m_audioConvert = AkElement::create("ACapsConvert");
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent()),
    m_inputIndex(other.m_inputIndex),
    m_audioBuffer(other.m_audioBuffer),
    m_audioFormat(other.m_audioFormat),
    m_audioChannels(other.m_audioChannels),
    m_id(other.m_id),
    m_pts(other.m_pts),
    m_ptsDiff(other.m_ptsDiff),
    m_ptsDrift(other.m_ptsDrift),
    m_codecContext(other.m_codecContext),
    m_scaleContext(NULL)
{
    this->m_audioConvert = AkElement::create("ACapsConvert");
}

OutputParams::~OutputParams()
{
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->m_inputIndex = other.m_inputIndex;
        this->m_audioBuffer = other.m_audioBuffer;
        this->m_audioFormat = other.m_audioFormat;
        this->m_audioChannels = other.m_audioChannels;
        this->m_id = other.m_id;
        this->m_pts = other.m_pts;
        this->m_ptsDiff = other.m_ptsDiff;
        this->m_ptsDrift = other.m_ptsDrift;
        this->m_codecContext = other.m_codecContext;
    }

    return *this;
}

int OutputParams::inputIndex() const
{
    return this->m_inputIndex;
}

int &OutputParams::inputIndex()
{
    return this->m_inputIndex;
}

qint64 OutputParams::nextPts(qint64 pts, qint64 id)
{
    if (this->m_pts < 0 || this->m_id < 0) {
        this->m_ptsDrift = -pts;
        this->m_pts = pts;
        this->m_id = id;

        return 0;
    }

    if (pts <= this->m_pts || id != this->m_id) {
        this->m_ptsDrift += this->m_pts - pts + this->m_ptsDiff;
        this->m_pts = pts;
        this->m_id = id;

        return pts + this->m_ptsDrift;
    }

    this->m_ptsDiff = pts - this->m_pts;
    this->m_pts = pts;

    return pts + this->m_ptsDrift;
}

void OutputParams::addAudioSamples(const AVFrame *frame, qint64 id)
{
    this->m_audioFormat = AVSampleFormat(frame->format);
    this->m_audioChannels = frame->channels;
    int audioBps = av_get_bytes_per_sample(AVSampleFormat(frame->format))
                   * frame->channels;

    int audioBufferSamples = this->m_audioBuffer.size() / audioBps;

    // Calculate silence offset and the number of samples to copy.
    int silence = 0;
    int frameSamples = frame->nb_samples;

    if (this->m_id != id) {
        this->m_ptsDrift = this->m_pts - frame->pts + audioBufferSamples;
        this->m_id = id;
    } else {
        qint64 framePts = frame->pts + this->m_ptsDrift;
        qint64 pts = this->m_pts + audioBufferSamples;
        qint64 ptsDiff = qAbs(framePts - pts);

        if (framePts > pts) {
            silence = int(ptsDiff);
        } else if (framePts < pts) {
            frameSamples -= ptsDiff;

            if (frameSamples < 0)
                frameSamples = 0;
        }
    }

    // Calculate total samples.
    int totalSamples = audioBufferSamples + silence + frameSamples;

    // Fill joined buffer.
    int joinedBufferSize = av_samples_get_buffer_size(NULL,
                                                      frame->channels,
                                                      totalSamples,
                                                      AVSampleFormat(frame->format),
                                                      1);

    QByteArray joinedBuffer(joinedBufferSize, Qt::Uninitialized);

    AVFrame joinedFrame;
    memset(&joinedFrame, 0, sizeof(AVFrame));
    joinedFrame.nb_samples = totalSamples;

    if (avcodec_fill_audio_frame(&joinedFrame,
                                 frame->channels,
                                 AVSampleFormat(frame->format),
                                 reinterpret_cast<const uint8_t *>(joinedBuffer.constData()),
                                 joinedBuffer.size(),
                                 1) < 0) {
        return;
    }

    if (!this->m_audioBuffer.isEmpty()) {
        // Fill current audio buffer.
        AVFrame bufferFrame;
        memset(&bufferFrame, 0, sizeof(AVFrame));
        bufferFrame.nb_samples = audioBufferSamples;

        if (avcodec_fill_audio_frame(&bufferFrame,
                                     frame->channels,
                                     AVSampleFormat(frame->format),
                                     reinterpret_cast<const uint8_t *>(this->m_audioBuffer.constData()),
                                     this->m_audioBuffer.size(),
                                     1) < 0) {
            return;
        }

        // Copy current audio buffer to joined buffer.
        av_samples_copy(joinedFrame.data,
                        bufferFrame.data,
                        0,
                        0,
                        audioBufferSamples,
                        frame->channels,
                        AVSampleFormat(frame->format));
    }

    // Add silence if necessary.
    av_samples_set_silence(joinedFrame.data,
                           audioBufferSamples,
                           silence,
                           frame->channels,
                           AVSampleFormat(frame->format));

    // Append Samples
    av_samples_copy(joinedFrame.data,
                    frame->data,
                    audioBufferSamples + silence,
                    0,
                    frameSamples,
                    frame->channels,
                    AVSampleFormat(frame->format));

    // Replace audio buffer with joined buffer.
    this->m_audioBuffer = joinedBuffer;
}

int OutputParams::readAudioSamples(int samples, uint8_t **buffer)
{
    if (!buffer)
        return 0;

    int frameSize = av_samples_get_buffer_size(NULL,
                                               this->m_audioChannels,
                                               samples,
                                               this->m_audioFormat,
                                               1);

    if (this->m_audioBuffer.size() < frameSize) {
        *buffer = NULL;

        return 0;
    }

    // Fill output buffer.
    *buffer = new uint8_t[frameSize];

    AVFrame outputFrame;
    memset(&outputFrame, 0, sizeof(AVFrame));
    outputFrame.nb_samples = samples;

    if (avcodec_fill_audio_frame(&outputFrame,
                                 this->m_audioChannels,
                                 this->m_audioFormat,
                                 *buffer,
                                 frameSize,
                                 1) < 0) {
        delete [] *buffer;
        *buffer = NULL;

        return 0;
    }

    // Fill audio buffer.
    AVFrame bufferFrame;
    memset(&bufferFrame, 0, sizeof(AVFrame));
    bufferFrame.nb_samples = this->m_audioBuffer.size()
                             / av_get_bytes_per_sample(this->m_audioFormat)
                             / this->m_audioChannels;

    if (avcodec_fill_audio_frame(&bufferFrame,
                                 this->m_audioChannels,
                                 this->m_audioFormat,
                                 reinterpret_cast<const uint8_t *>(this->m_audioBuffer.constData()),
                                 this->m_audioBuffer.size(),
                                 1) < 0) {
        delete [] *buffer;
        *buffer = NULL;

        return 0;
    }

    // Copy current audio buffer to output buffer.
    av_samples_copy(outputFrame.data,
                    reinterpret_cast<uint8_t * const *>(bufferFrame.data),
                    0,
                    0,
                    samples,
                    this->m_audioChannels,
                    this->m_audioFormat);

    // Create a new audio buffer with the remaining samples.
    QByteArray audioBuffer(this->m_audioBuffer.size() - frameSize,
                           Qt::Uninitialized);

    // Fill new audio buffer.
    AVFrame audioFrame;
    memset(&audioFrame, 0, sizeof(AVFrame));
    audioFrame.nb_samples = bufferFrame.nb_samples - samples;

    if (avcodec_fill_audio_frame(&audioFrame,
                                 this->m_audioChannels,
                                 this->m_audioFormat,
                                 reinterpret_cast<const uint8_t *>(audioBuffer.constData()),
                                 audioBuffer.size(),
                                 1) < 0) {
        delete [] *buffer;
        *buffer = NULL;

        return 0;
    }

    // Copy remaining samples to the new buffer-
    av_samples_copy(audioFrame.data,
                    reinterpret_cast<uint8_t * const *>(bufferFrame.data),
                    0,
                    samples,
                    audioFrame.nb_samples,
                    this->m_audioChannels,
                    this->m_audioFormat);

    this->m_audioBuffer = audioBuffer;
    this->m_pts += samples;

    return frameSize;
}

qint64 OutputParams::audioPts() const
{
    return this->m_pts;
}

CodecContextPtr OutputParams::codecContext() const
{
    return this->m_codecContext;
}

void OutputParams::deleteCodecContext(AVCodecContext *codecContext)
{
    avcodec_free_context(&codecContext);
}

void OutputParams::setInputIndex(int inputIndex)
{
    if (this->m_inputIndex == inputIndex)
        return;

    this->m_inputIndex = inputIndex;
    emit this->inputIndexChanged(inputIndex);
}

void OutputParams::resetInputIndex()
{
    this->setInputIndex(0);
}

bool OutputParams::convert(const AkPacket &packet, AVFrame *frame)
{
    if (packet.caps().mimeType() == "audio/x-raw")
        return this->convert(AkAudioPacket(packet), frame);
    else if (packet.caps().mimeType() == "video/x-raw")
        return this->convert(AkVideoPacket(packet), frame);

    return false;
}

bool OutputParams::convert(const AkAudioPacket &packet, AVFrame *frame)
{
    if (this->m_audioConvert->state() != AkElement::ElementStatePlaying) {
        auto fmtName = av_get_sample_fmt_name(AVSampleFormat(frame->format));
        AkAudioCaps caps(AkAudioCaps::sampleFormatFromString(fmtName),
                         frame->channels,
                         frame->sample_rate);
        caps.layout() = channelLayouts->key(frame->channel_layout);
        this->m_audioConvert->setProperty("caps", caps.toString());
        this->m_audioConvert->setState(AkElement::ElementStatePlaying);
    }

    auto oPacket = AkAudioPacket(this->m_audioConvert->iStream(packet));

    if (!oPacket)
        return false;

    static AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_samples_fill_arrays(oFrame.data,
                               oFrame.linesize,
                               reinterpret_cast<const uint8_t *>(oPacket.buffer().constData()),
                               frame->channels,
                               oPacket.caps().samples(),
                               AVSampleFormat(frame->format),
                               1) < 0) {
        return false;
    }

    if (av_samples_alloc(frame->data,
                         frame->linesize,
                         frame->channels,
                         oPacket.caps().samples(),
                         AVSampleFormat(frame->format),
                         1) < 0) {
        return false;
    }

    frame->nb_samples = oPacket.caps().samples();
    frame->pts = oPacket.pts();

    if (av_samples_copy(frame->data,
                        oFrame.data,
                        0,
                        0,
                        frame->nb_samples,
                        frame->channels,
                        AVSampleFormat(frame->format)) < 0) {
        av_freep(&oFrame.data[0]);
        av_frame_unref(&oFrame);

        return false;
    }

    return true;
}

bool OutputParams::convert(const AkVideoPacket &packet, AVFrame *frame)
{
    QString format = AkVideoCaps::pixelFormatToString(packet.caps().format());
    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());
    int iWidth = packet.caps().width();
    int iHeight = packet.caps().height();

    this->m_scaleContext =
            sws_getCachedContext(this->m_scaleContext,
                                 iWidth,
                                 iHeight,
                                 iFormat,
                                 frame->width,
                                 frame->height,
                                 AVPixelFormat(frame->format),
                                 SWS_FAST_BILINEAR,
                                 NULL,
                                 NULL,
                                 NULL);

    if (!this->m_scaleContext)
        return false;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    av_image_fill_arrays(const_cast<uint8_t **>(iFrame.data),
                         iFrame.linesize,
                         reinterpret_cast<const uint8_t *>(packet.buffer().constData()),
                         iFormat,
                         iWidth,
                         iHeight,
                         1);

    if (av_image_alloc(const_cast<uint8_t **>(frame->data),
                       frame->linesize,
                       frame->width,
                       frame->height,
                       AVPixelFormat(frame->format),
                       4) < 0)
        return false;

    sws_scale(this->m_scaleContext,
              const_cast<uint8_t **>(iFrame.data),
              iFrame.linesize,
              0,
              iHeight,
              frame->data,
              frame->linesize);

    return true;
}
