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

#include "outputparams.h"

OutputParams::OutputParams(int inputIndex, QObject *parent):
    QObject(parent),
    m_inputIndex(inputIndex),
    m_audioFormat(AV_SAMPLE_FMT_NONE),
    m_audioChannels(0),
    m_id(-1),
    m_pts(0),
    m_ptsDiff(0),
    m_ptsDrift(0),
    m_resampleContext(NULL),
    m_scaleContext(NULL)
{
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
    m_resampleContext(NULL),
    m_scaleContext(NULL)
{
}

OutputParams::~OutputParams()
{
    if (this->m_resampleContext)
        swr_free(&this->m_resampleContext);

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

        if (framePts > pts)
            silence = ptsDiff;
        else if (framePts < pts) {
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
                                 (const uint8_t *) joinedBuffer.constData(),
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
                                     (const uint8_t *) this->m_audioBuffer.constData(),
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

QByteArray OutputParams::readAudioSamples(int samples)
{
    int frameSize = samples
                    * av_get_bytes_per_sample(this->m_audioFormat)
                    * this->m_audioChannels;

    if (this->m_audioBuffer.size() < frameSize)
        return QByteArray();

    // Fill output buffer.
    int outputBufferSize = av_samples_get_buffer_size(NULL,
                                                      this->m_audioChannels,
                                                      samples,
                                                      this->m_audioFormat,
                                                      1);

    QByteArray outputBuffer(outputBufferSize, Qt::Uninitialized);

    AVFrame outputFrame;
    memset(&outputFrame, 0, sizeof(AVFrame));
    outputFrame.nb_samples = samples;

    if (avcodec_fill_audio_frame(&outputFrame,
                                 this->m_audioChannels,
                                 this->m_audioFormat,
                                 (const uint8_t *) outputBuffer.constData(),
                                 outputBuffer.size(),
                                 1) < 0) {
        return QByteArray();
    }

    // Fill Audio buffer.
    AVFrame bufferFrame;
    memset(&bufferFrame, 0, sizeof(AVFrame));
    bufferFrame.nb_samples = this->m_audioBuffer.size()
                             / av_get_bytes_per_sample(this->m_audioFormat)
                             / this->m_audioChannels;

    if (avcodec_fill_audio_frame(&bufferFrame,
                                 this->m_audioChannels,
                                 this->m_audioFormat,
                                 (const uint8_t *) this->m_audioBuffer.constData(),
                                 this->m_audioBuffer.size(),
                                 1) < 0) {
        return QByteArray();
    }

    // Copy current audio buffer to output buffer.
    av_samples_copy(outputFrame.data,
                    bufferFrame.data,
                    0,
                    0,
                    samples,
                    this->m_audioChannels,
                    this->m_audioFormat);

    this->m_audioBuffer.remove(0, frameSize);
    this->m_pts += samples;

    return outputBuffer;
}

qint64 OutputParams::audioPts() const
{
    return this->m_pts;
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

bool OutputParams::convert(const QbPacket &packet, AVFrame *frame)
{
    Q_UNUSED(packet)
    Q_UNUSED(frame)

    return false;
}

bool OutputParams::convert(const QbAudioPacket &packet, AVFrame *frame)
{
    QString layout = QbAudioCaps::channelLayoutToString(packet.caps().layout());
    int64_t iLayout = av_get_channel_layout(layout.toStdString().c_str());;
    QString format = QbAudioCaps::sampleFormatToString(packet.caps().format());
    AVSampleFormat iFormat = av_get_sample_fmt(format.toStdString().c_str());
    int iSampleRate = packet.caps().rate();

    this->m_resampleContext = swr_alloc_set_opts(this->m_resampleContext,
                                                 frame->channel_layout,
                                                 AVSampleFormat(frame->format),
                                                 frame->sample_rate,
                                                 iLayout,
                                                 iFormat,
                                                 iSampleRate,
                                                 0,
                                                 NULL);

    if (!this->m_resampleContext)
        return false;

    if (!swr_is_initialized(this->m_resampleContext))
        if (swr_init(this->m_resampleContext) < 0)
            return false;

    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    int iChannels = packet.caps().channels();
    int iSamples = packet.caps().samples();

    if (av_samples_fill_arrays(iFrame.data,
                               iFrame.linesize,
                               (const uint8_t *) packet.buffer().constData(),
                               iChannels,
                               iSamples,
                               iFormat,
                               1) < 0)
        return false;

    iFrame.channels = iChannels;
    iFrame.channel_layout = iLayout;
    iFrame.format = iFormat;
    iFrame.sample_rate = iSampleRate;
    iFrame.nb_samples = packet.caps().samples();

    int oNSamples = swr_get_delay(this->m_resampleContext, frame->sample_rate)
                    + iFrame.nb_samples
                    * (int64_t) frame->sample_rate / iSampleRate
                    + 3;

    frame->nb_samples = oNSamples;
    av_frame_get_buffer(frame, 0);

    // convert to destination format
    frame->nb_samples = swr_convert(this->m_resampleContext,
                                    frame->data,
                                    frame->nb_samples,
                                    (const uint8_t **) iFrame.data,
                                    iFrame.nb_samples);

    if (frame->nb_samples < 1)
        return false;

    return true;
}

bool OutputParams::convert(const QbVideoPacket &packet, AVFrame *frame)
{
    QString format = QbVideoCaps::pixelFormatToString(packet.caps().format());
    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());
    int iWidth = packet.caps().width();
    int iHeight = packet.caps().height();

    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
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

    AVPicture iPicture;
    memset(&iPicture, 0, sizeof(AVPicture));

    avpicture_fill(&iPicture,
                   (const uint8_t *) packet.buffer().constData(),
                   iFormat,
                   iWidth,
                   iHeight);

    if (avpicture_alloc((AVPicture *) frame,
                        AVPixelFormat(frame->format),
                        frame->width,
                        frame->height) < 0)
        return false;

    sws_scale(this->m_scaleContext,
              (uint8_t **) iPicture.data,
              iPicture.linesize,
              0,
              iHeight,
              frame->data,
              frame->linesize);

    return true;
}
