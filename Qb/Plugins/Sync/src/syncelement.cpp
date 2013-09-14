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

    this->m_frameLastDuration = 0;
    this->m_frameLastPts = AV_NOPTS_VALUE;
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
    this->m_audioDiffThreshold = 2.0 * 0 / packet.caps().property("rate").toInt();
// ----------------------------------------------------------------------------------
    double diff = this->m_audioClock.clock() - this->m_extrnClock.clock();

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

    if (this->m_fst)
    {
        this->m_audioClock = Clock();
        this->m_videoClock = Clock();
        this->m_extrnClock = Clock();

        this->m_frameTimer = QDateTime::currentMSecsSinceEpoch() / 1.0e3;

        this->m_fst = false;
    }

    this->m_avqueue.enqueue(packet);

    if (!this->m_avqueue.isEmpty("audio/x-raw"))
        QtConcurrent::run(this, &SyncElement::processAudioFrame);

    if (!this->m_avqueue.isEmpty("video/x-raw"))
        QtConcurrent::run(this, &SyncElement::processVideoFrame);
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

void SyncElement::processAudioFrame()
{
    this->m_audioLock.lock();

    if (this->m_avqueue.isEmpty("audio/x-raw"))
    {
        this->m_audioLock.unlock();

        return;
    }

    QbPacket packet = this->m_avqueue.dequeue("audio/x-raw");

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

    if (caps1 != caps2)
    {
        // create resampler context
        this->m_resampleContext = SwrContextPtr(swr_alloc(), this->deleteSwrContext);

        if (!this->m_resampleContext)
        {
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
        this->m_audioLock.unlock();

        return;
    }

    int oNSamples = swr_convert(this->m_resampleContext.data(),
                                oData.data(),
                                wantedNbSamples,
                                (const uint8_t **) iData.data(),
                                iNSamples);

    if (oNSamples < 0)
    {
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

    this->m_audioLock.unlock();
}

void SyncElement::processVideoFrame()
{
    this->m_videoLock.lock();

    if (this->m_avqueue.isEmpty("video/x-raw"))
    {
        this->m_videoLock.unlock();

        return;
    }

    QbPacket packet = this->m_avqueue.dequeue("video/x-raw");

    double pts = packet.pts() * packet.timeBase().value();
    double delay = pts - this->m_frameLastPts;

    double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN, delay, AV_SYNC_THRESHOLD_MAX);

    // if video is slave, we try to correct big delays by
    // duplicating or deleting a frame
    double diff = this->m_videoClock.clock() - this->m_extrnClock.clock();

    qDebug() << "A-V=" << -diff;

    if (!isnan(diff) && fabs(diff) < AV_SYNC_THRESHOLD_MAX)
    {
        // video is backward the external clock.
        if (diff < 0)
        {
            if (fabs(diff) > syncThreshold)
            {
                this->m_videoLock.unlock();
                qDebug() << "discard";

                return;
            }
            else
            {
                qDebug() << "show";
            }
        }
        else if (diff > 0)
        {/*
            if (fabs(diff) > syncThreshold)
            {*/
                Sleep::usleep(1.0e6 * diff);
                qDebug() << "wait";
/*            }
            else
            {
                qDebug() << "show";
            }*/
        }
    }
    else
    {
        qDebug() << "show";
    }

    this->updateVideoPts(pts);

    // display picture
    emit this->oStream(packet);

    this->m_videoLock.unlock();
}

void SyncElement::updateVideoPts(double pts)
{
    // update current video pts
    this->m_videoClock.setClock(pts);
    this->m_extrnClock.syncTo(this->m_videoClock);
    this->m_frameLastPts = pts;
}
