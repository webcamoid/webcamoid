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

    this->m_audioDiffCum = 0;
    this->m_audioDiffAvgCoef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
/*
    this->m_audioDiffThreshold = 2.0 * this->m_audio_hw_buf_size
                                 / av_samples_get_buffer_size(NULL,
                                                              this->m_audio_tgt.channels,
                                                              this->m_audio_tgt.freq,
                                                              this->m_audio_tgt.fmt,
                                                              1);
*/
    this->m_audioDiffAvgCount = 0;
}

void SyncElement::deleteSwrContext(SwrContext *context)
{
    swr_free(&context);
}

// return the wanted number of samples to get better sync if sync_type is video
// or external master clock
int SyncElement::synchronizeAudio(const QbPacket &packet)
{
    int nbSamples = packet.caps().property("samples").toInt();
    int wantedNbSamples = nbSamples;
// ----------------------------------------------------------------------------------
    this->m_audioDiffThreshold = 0; //2.0 * 1024 / packet.caps().property("rate").toInt();
// ----------------------------------------------------------------------------------
    double clock = this->m_extrnClock.clock();
    double pts = this->m_audioClock.clock(packet.pts() * packet.timeBase().value());
    double diff = clock - pts;

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
    {
        this->m_audioDiffCum = diff + this->m_audioDiffAvgCoef * this->m_audioDiffCum;

        if (this->m_audioDiffAvgCount < AUDIO_DIFF_AVG_NB)
            // not enough measures to have a correct estimate
            this->m_audioDiffAvgCount++;
        else
        {
            // estimate the A-V difference
            double avgDiff = this->m_audioDiffCum * (1.0 - this->m_audioDiffAvgCoef);

            if (fabs(avgDiff) >= this->m_audioDiffThreshold)
            {
                int rate = packet.caps().property("rate").toInt();
                wantedNbSamples = nbSamples + (int) (diff * rate);
                int minNbSamples = nbSamples * (1.0 - SAMPLE_CORRECTION_PERCENT_MAX);
                int maxNbSamples = nbSamples * (1.0 + SAMPLE_CORRECTION_PERCENT_MAX);
                wantedNbSamples = qBound(minNbSamples, wantedNbSamples, maxNbSamples);
            }
        }
    }
    else
    {
        // too big difference : may be initial PTS errors, so
        // reset A-V filter
        this->m_audioDiffAvgCount = 0;
        this->m_audioDiffCum = 0;
    }

    return wantedNbSamples;
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

    this->m_globlLock.wait();

    if (this->m_fst)
    {
        this->m_audioClock.init(true);
        this->m_videoClock.init(true);
        this->m_extrnClock.init();

        this->m_globlLock.init(2);

        this->m_fst = false;
    }

    QString streamType = packet.caps().mimeType();

    if (streamType == "audio/x-raw")
        QtConcurrent::run(this, &SyncElement::processAudioFrame, packet);
    else
        QtConcurrent::run(this, &SyncElement::processVideoFrame, packet);
}

void SyncElement::setState(ElementState state)
{
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateReady ||
        this->state() == QbElement::ElementStateNull)
    {
        this->m_ready = false;
        this->m_fst = true;
    }
}

void SyncElement::processAudioFrame(const QbPacket &packet)
{
    this->m_audioLock.lock();
    this->m_globlLock.lock();

    AVSampleFormat iSampleFormat = av_get_sample_fmt(packet.caps().property("format").toString().toStdString().c_str());
    int iNChannels = packet.caps().property("channels").toInt();
    int64_t iChannelLayout = av_get_channel_layout(packet.caps().property("layout").toString().toStdString().c_str());
    int iNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    int iSampleRate = packet.caps().property("rate").toInt();
    int iNSamples = packet.caps().property("samples").toInt();

    int wantedNbSamples = this->synchronizeAudio(packet);

    QbCaps caps1(packet.caps());
    QbCaps caps2(this->m_curInputCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());

    if (caps1 != caps2 ||
        wantedNbSamples != iNSamples)
    {
        // create resampler context
        this->m_resampleContext = SwrContextPtr(swr_alloc(), this->deleteSwrContext);

        if (!this->m_resampleContext)
        {
            this->m_globlLock.unlock();
            this->m_audioLock.unlock();

            return;
        }

        // set options
        av_opt_set_int(this->m_resampleContext.data(), "in_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "in_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "in_sample_fmt", iSampleFormat, 0);

        av_opt_set_int(this->m_resampleContext.data(), "out_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "out_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "out_sample_fmt", iSampleFormat, 0);

        // initialize the resampling context
        if (swr_init(this->m_resampleContext.data()) < 0)
        {
            this->m_globlLock.unlock();
            this->m_audioLock.unlock();

            return;
        }

        this->m_curInputCaps = packet.caps();
    }

    if (wantedNbSamples != iNSamples)
        if (swr_set_compensation(this->m_resampleContext.data(),
                                 wantedNbSamples - iNSamples,
                                 wantedNbSamples) < 0)
        {
            this->m_globlLock.unlock();
            this->m_audioLock.unlock();

            return;
        }

    // buffer is going to be directly written to a rawaudio file, no alignment
    int oNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    QVector<uint8_t *> oData(oNPlanes);

    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 iNChannels,
                                                 wantedNbSamples,
                                                 iSampleFormat,
                                                 1);

    QSharedPointer<uchar> oBuffer(new uchar[oBufferSize]);

    if (!oBuffer)
    {
        this->m_globlLock.unlock();
        this->m_audioLock.unlock();

        return;
    }

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               iNChannels,
                               wantedNbSamples,
                               iSampleFormat,
                               1) < 0)
    {
        this->m_globlLock.unlock();
        this->m_audioLock.unlock();

        return;
    }

    QVector<uint8_t *> iData(iNPlanes);
    int iLineSize;

    if (av_samples_fill_arrays(&iData.data()[0],
                               &iLineSize,
                               (const uint8_t *) packet.buffer().data(),
                               iNChannels,
                               iNSamples,
                               iSampleFormat,
                               1) < 0)
    {
        this->m_globlLock.unlock();
        this->m_audioLock.unlock();

        return;
    }

    int oNSamples = swr_convert(this->m_resampleContext.data(),
                                oData.data(),
                                256 * wantedNbSamples,
                                (const uint8_t **) iData.data(),
                                iNSamples);

    if (oNSamples < 0)
    {
        this->m_globlLock.unlock();
        this->m_audioLock.unlock();

        return;
    }

    QbPacket oPacket(packet);
    QbCaps caps(oPacket.caps());
    caps.setProperty("samples", oNSamples);

    oPacket.setBuffer(oBuffer);
    oPacket.setBufferSize(oBufferSize);
    oPacket.setCaps(caps);

    emit this->oStream(oPacket);

    this->m_globlLock.unlock();
    this->m_audioLock.unlock();
}

void SyncElement::processVideoFrame(const QbPacket &packet)
{
    this->m_videoLock.lock();
    this->m_globlLock.lock();

    double clock = this->m_extrnClock.clock();
    double pts = this->m_videoClock.clock(packet.pts() * packet.timeBase().value());
    double diff = clock - pts;
    bool show = false;

    if (fabs(diff) < AV_SYNC_THRESHOLD_MIN)
    {
        show = true;
        emit this->oStream(packet);
    }
    else if (fabs(diff) < AV_SYNC_THRESHOLD_MAX)
    {
        if (diff < 0.0)
        {
            // Add a delay
            show = true;
            Sleep::usleep(1.0e6 * fabs(diff));

            emit this->oStream(packet);
        }
        else
        {
            // Discard frame
        }
    }
    else
    {
        // Resync to the master clock
        show = true;
        this->m_videoClock.syncTo(clock);

        emit this->oStream(packet);
    }

    qDebug() << packet.caps().mimeType().toStdString().c_str()[0]
             << QString().sprintf("%.2f", clock).toStdString().c_str()
             << QString().sprintf("%.2f", pts).toStdString().c_str()
             << QString().sprintf("%.2f", diff).toStdString().c_str()
             << (show? "true": "false");

    this->m_globlLock.unlock();
    this->m_videoLock.unlock();
}
