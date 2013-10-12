/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "sleep.h"
#include "syncelement.h"

SyncElement::SyncElement(): QbElement()
{
    this->m_fst = true;
    this->m_ready = false;
    this->m_isAlive = true;

    this->m_log = true;

    this->m_audioDiffAvgCoef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    this->m_audioDiffCum = 0;
    this->m_audioDiffAvgCount = 0;
    this->resetOutputAudioBufferSize();

    this->m_frameLastPts = AV_NOPTS_VALUE;

    QtConcurrent::run(this, &SyncElement::processAudioFrame);
    QtConcurrent::run(this, &SyncElement::processVideoFrame);
}

SyncElement::~SyncElement()
{
    this->m_isAlive = false;
}

int SyncElement::outputAudioBufferSize() const
{
    return this->m_outputAudioBufferSize;
}

void SyncElement::deleteSwrContext(SwrContext *context)
{
    swr_free(&context);
}

QbPacket SyncElement::compensateAudio(const QbPacket &packet, int wantedSamples)
{
    int iNSamples = packet.caps().property("samples").toInt();

    if (iNSamples == wantedSamples)
        return packet;

    AVSampleFormat iSampleFormat = av_get_sample_fmt(packet.caps().property("format").toString().toStdString().c_str());
    int iNChannels = packet.caps().property("channels").toInt();
    int64_t iChannelLayout = av_get_channel_layout(packet.caps().property("layout").toString().toStdString().c_str());
    int iNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    int iSampleRate = packet.caps().property("rate").toInt();

    QbCaps caps1(packet.caps());
    QbCaps caps2(this->m_curInputCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());

    if (caps1 != caps2)
    {
        // create resampler context
        this->m_resampleContext = SwrContextPtr(swr_alloc(), this->deleteSwrContext);

        if (!this->m_resampleContext)
            return packet;

        // set options
        av_opt_set_int(this->m_resampleContext.data(), "in_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "in_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "in_sample_fmt", iSampleFormat, 0);

        av_opt_set_int(this->m_resampleContext.data(), "out_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "out_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "out_sample_fmt", iSampleFormat, 0);

        // initialize the resampling context
        if (swr_init(this->m_resampleContext.data()) < 0)
            return packet;

        this->m_curInputCaps = packet.caps();
    }

    if (swr_set_compensation(this->m_resampleContext.data(),
                             wantedSamples - iNSamples,
                             wantedSamples) < 0)
        return packet;

    // buffer is going to be directly written to a rawaudio file, no alignment
    int oNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    QVector<uint8_t *> oData(oNPlanes);

    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 iNChannels,
                                                 wantedSamples,
                                                 iSampleFormat,
                                                 0);

    QbBufferPtr oBuffer(new uchar[oBufferSize]);

    if (!oBuffer)
        return packet;

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               iNChannels,
                               wantedSamples,
                               iSampleFormat,
                               0) < 0)
        return packet;

    QVector<uint8_t *> iData(iNPlanes);
    int iLineSize;

    if (av_samples_fill_arrays(&iData.data()[0],
                               &iLineSize,
                               (const uint8_t *) packet.buffer().data(),
                               iNChannels,
                               iNSamples,
                               iSampleFormat,
                               0) < 0)
        return packet;

    int oNSamples = swr_convert(this->m_resampleContext.data(),
                                oData.data(),
                                wantedSamples,
                                (const uint8_t **) iData.data(),
                                iNSamples);

    if (oNSamples < 0)
        return packet;

    QbPacket oPacket(packet);
    QbCaps caps(oPacket.caps());
    caps.setProperty("samples", oNSamples);

    oPacket.setBuffer(oBuffer);
    oPacket.setBufferSize(oBufferSize);
    oPacket.setCaps(caps);

    double duration = (double) oNSamples
                      / (double) iSampleRate
                      / packet.timeBase().value();

    oPacket.setDuration(duration);

    return oPacket;
}

// return the wanted number of samples to get better sync if sync_type is video
// or external master clock
int SyncElement::synchronizeAudio(double diff, QbPacket packet)
{
    int samples = packet.caps().property("samples").toInt();
    int wantedSamples = samples;

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
    {
        this->m_audioDiffCum = diff + this->m_audioDiffAvgCoef * this->m_audioDiffCum;

        if (this->m_audioDiffAvgCount < AUDIO_DIFF_AVG_NB)
            // not enough measures to have a correct estimate
            this->m_audioDiffAvgCount += 1;
        else
        {
            // estimate the A-V difference
            double avgDiff = this->m_audioDiffCum * (1.0 - this->m_audioDiffAvgCoef);
            double rate = packet.caps().property("rate").toDouble();
            double audioDiffThreshold = 2.0 * this->m_outputAudioBufferSize / rate;

            if (fabs(avgDiff) >= audioDiffThreshold)
            {
                wantedSamples = samples + diff * rate;
                int minSamples = samples * (1.0 - SAMPLE_CORRECTION_PERCENT_MAX);
                int maxSamples = samples * (1.0 + SAMPLE_CORRECTION_PERCENT_MAX);
                wantedSamples = qBound(minSamples, wantedSamples, maxSamples);
            }
        }
    }
    else
    {
        // too big difference: may be initial PTS errors, so
        // reset A-V filter
        this->m_audioDiffAvgCount = 0;
        this->m_audioDiffCum = 0;
    }

    return wantedSamples;
}

SyncElement::PackageProcessing SyncElement::synchronizeVideo(double diff, double delay)
{
    // (resync)
    // -AV_SYNC_THRESHOLD_MAX
    // (discard)
    // -syncThreshold
    // (release)
    // AV_SYNC_THRESHOLD_MIN
    // (wait)
    // AV_NOSYNC_THRESHOLD
    // (resync)

    double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN,
                                  delay,
                                  AV_SYNC_THRESHOLD_MAX);

    if (!isnan(diff) && abs(diff) < AV_NOSYNC_THRESHOLD)
    {
        if (diff > -syncThreshold)
        {
            // stream is ahead the external clock.
            if (diff > syncThreshold)
                Sleep::usleep(1.0e6 * diff);

            return PackageProcessingRelease;
        }
        // stream is backward the external clock.
        else
            return PackageProcessingDiscard;
    }

    // Update clocks.

    return PackageProcessingReSync;
}

void SyncElement::printLog(const QbPacket &packet, double diff)
{
    if (this->m_log)
    {
        QString logFmt("%1 %2 A-V: %3 aq=%4 vq=%5");

        QString log = logFmt.arg(packet.caps().mimeType()[0])
                            .arg(this->m_extrnClock.clock(), 7, 'f', 2)
                            .arg(-diff, 7, 'f', 3)
                            .arg(this->m_avqueue.size("audio/x-raw"), 5)
                            .arg(this->m_avqueue.size("video/x-raw"), 5);

        qDebug() << log.toStdString().c_str();
    }
}

void SyncElement::setOutputAudioBufferSize(int outputAudioBufferSize)
{
    this->m_outputAudioBufferSize = outputAudioBufferSize;
}

void SyncElement::resetOutputAudioBufferSize()
{
    this->setOutputAudioBufferSize(1024);
}

void SyncElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    if (!this->m_ready)
    {
        this->m_ready = true;
        emit this->ready(packet.index());
    }

    if (this->m_fst)
    {
        this->m_audioClock = Clock();
        this->m_videoClock = Clock();
        this->m_extrnClock = Clock();

        this->m_fst = false;
    }

    this->m_avqueue.enqueue(packet);
}

void SyncElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateReady ||
        this->state() == QbElement::ElementStateNull)
    {
        this->m_ready = false;
        this->m_fst = true;
    }
}

void SyncElement::processAudioFrame()
{
    while (this->m_isAlive)
    {
        QbPacket packet = this->m_avqueue.dequeue("audio/x-raw");

        double pts = packet.pts() * packet.timeBase().value();

        // if video is slave, we try to correct big delays by
        // duplicating or deleting a frame
        double diff = this->m_audioClock.clock() - this->m_extrnClock.clock();
        this->m_audioClock.setClock(pts);

        int wantedSamples = this->synchronizeAudio(diff, packet);
        QbPacket oPacket = this->compensateAudio(packet, wantedSamples);

        this->printLog(oPacket, diff);

        emit this->oStream(oPacket);
        this->m_extrnClock.syncTo(this->m_audioClock);
    }
}

void SyncElement::processVideoFrame()
{
    while (this->m_isAlive)
    {
        QbPacket packet = this->m_avqueue.dequeue("video/x-raw");

        // if video is slave, we try to correct big delays by
        // duplicating or deleting a frame
        double diff = this->m_videoClock.clock() - this->m_extrnClock.clock();
        double pts = packet.pts() * packet.timeBase().value();
        double delay = pts - this->m_frameLastPts;

        this->m_videoClock.setClock(pts);
        this->m_frameLastPts = pts;

        PackageProcessing operation = this->synchronizeVideo(diff, delay);

        if (operation == PackageProcessingDiscard)
            continue;
        else if (operation == PackageProcessingReSync)
            // update current video pts
            this->m_extrnClock.syncTo(this->m_videoClock);

        this->printLog(packet, diff);

        // display picture
        emit this->oStream(packet);
    }
}
