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

    this->m_log = true;

    this->m_audioDiffAvgCoef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    this->m_audioDiffCum = 0;
    this->m_audioDiffAvgCount = 0;
    this->resetOutputAudioBufferSize();

    this->m_frameLastPts = AV_NOPTS_VALUE;
    this->m_frameLastDuration = 0;
    this->m_frameTimer = QDateTime::currentMSecsSinceEpoch() / 1.0e3;
    this->m_frameLastDroppedPts = AV_NOPTS_VALUE;

    this->resetAudioTh();
    this->resetVideoTh();
}

SyncElement::~SyncElement()
{
    this->setState(QbElement::ElementStateNull);
}

int SyncElement::outputAudioBufferSize() const
{
    return this->m_outputAudioBufferSize;
}

QString SyncElement::audioTh() const
{
    return this->m_audioTh;
}

QString SyncElement::videoTh() const
{
    return this->m_videoTh;
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

TimerPtr SyncElement::runThread(QThread *thread, const char *method)
{
    TimerPtr timer = TimerPtr(new QTimer());

    QObject::connect(timer.data(), SIGNAL(timeout()), this, method, Qt::DirectConnection);
    timer->moveToThread(thread);

    return timer;
}

double SyncElement::computeTargetDelay(double delay, double *diff)
{
    double nDiff;

    if (!diff)
        diff = &nDiff;

    // if video is slave, we try to correct big delays by
    // duplicating or deleting a frame
    *diff = this->m_videoClock.clock() - this->m_extrnClock.clock();

    // skip or repeat frame. We take into account the
    // delay to compute the threshold. I still don't know
    // if it is the best guess
    double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN, delay, AV_SYNC_THRESHOLD_MAX);

    if (!isnan(*diff) && fabs(*diff) < 10.0)
    {
        // video is backward the external clock.
        if (*diff <= -syncThreshold)
            delay = qMax(0.0, delay + *diff);
        // video is ahead the external clock, and delay is over AV_SYNC_FRAMEDUP_THRESHOLD.
        else if (*diff >= syncThreshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
            delay = delay + *diff;
        // video is ahead the external clock.
        else if (*diff >= syncThreshold)
            delay = 2 * delay;
    }

    return delay;
}

void SyncElement::setOutputAudioBufferSize(int outputAudioBufferSize)
{
    this->m_outputAudioBufferSize = outputAudioBufferSize;
}

void SyncElement::setAudioTh(const QString &audioTh)
{
    QbElement::ElementState state = this->state();
    this->setState(QbElement::ElementStateNull);

    this->m_audioTh = audioTh;
    this->m_audioThread = Qb::requestThread(this->m_audioTh);

    if (this->m_videoTh == "MAIN")
        this->m_audioTimer = this->runThread(QCoreApplication::instance()->thread(),
                                             SLOT(processAudioFrame()));
    else if (!this->m_audioThread)
        this->m_audioTimer = this->runThread(this->thread(),
                                             SLOT(processAudioFrame()));
    else
        this->m_audioTimer = this->runThread(this->m_audioThread.data(),
                                             SLOT(processAudioFrame()));

    this->setState(state);
}

void SyncElement::setVideoTh(const QString &videoTh)
{
    QbElement::ElementState state = this->state();
    this->setState(QbElement::ElementStateNull);

    this->m_videoTh = videoTh;
    this->m_videoThread = Qb::requestThread(this->m_videoTh);

    if (this->m_videoTh == "MAIN")
        this->m_videoTimer = this->runThread(QCoreApplication::instance()->thread(),
                                             SLOT(processVideoFrame()));
    else if (!this->m_videoThread)
        this->m_videoTimer = this->runThread(this->thread(),
                                             SLOT(processVideoFrame()));
    else
        this->m_videoTimer = this->runThread(this->m_videoThread.data(),
                                             SLOT(processVideoFrame()));

    this->setState(state);
}

void SyncElement::resetOutputAudioBufferSize()
{
    this->setOutputAudioBufferSize(1024);
}

void SyncElement::resetAudioTh()
{
    this->setAudioTh("");
}

void SyncElement::resetVideoTh()
{
    this->setVideoTh("");
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

    if (this->state() == QbElement::ElementStatePlaying)
    {
        if (this->m_audioTimer)
            QMetaObject::invokeMethod(this->m_audioTimer.data(), "start");

        if (this->m_videoTimer)
            QMetaObject::invokeMethod(this->m_videoTimer.data(), "start");
    }
    else
    {
        if (this->m_audioTimer)
            QMetaObject::invokeMethod(this->m_audioTimer.data(), "stop");

        if (this->m_videoTimer)
            QMetaObject::invokeMethod(this->m_videoTimer.data(), "stop");
    }
}

void SyncElement::processAudioFrame()
{
    QbPacket packet = this->m_avqueue.dequeue("audio/x-raw");
/*
    double pts = packet.pts() * packet.timeBase().value();

    // if video is slave, we try to correct big delays by
    // duplicating or deleting a frame
    double diff = this->m_audioClock.clock() - this->m_extrnClock.clock();
    this->m_audioClock.setClock(pts);

    int wantedSamples = this->synchronizeAudio(diff, packet);
    QbPacket oPacket = this->compensateAudio(packet, wantedSamples);

    this->printLog(oPacket, diff);

    emit this->oStream(oPacket);*/
    emit this->oStream(packet);
//    this->m_extrnClock.syncTo(this->m_audioClock);
}

void SyncElement::processVideoFrame()
{/*
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
        return;
    else if (operation == PackageProcessingReSync)
        // update current video pts
        this->m_extrnClock.syncTo(this->m_videoClock);

    this->printLog(packet, diff);

    // display picture
    emit this->oStream(packet);
    */
    double remainingTime = 0.0;

    forever
    {
        QCoreApplication::processEvents();

        if (remainingTime > 0.0)
            Sleep::usleep(1.0e6 * remainingTime);

        remainingTime = 0.01;//REFRESH_RATE;

        this->videoRefresh(&remainingTime);
    }

}

void SyncElement::videoRefresh(double *remainingTime)
{
    forever
    {
        if (this->m_avqueue.size("video/x-raw") < 1)
        {
            // nothing to do, no picture to display in the queue

            if (this->m_frameLastDroppedPts != AV_NOPTS_VALUE
                && this->m_frameLastDroppedPts > this->m_frameLastPts)
            {
                this->updateVideoPts(this->m_frameLastDroppedPts);
                this->m_frameLastDroppedPts = AV_NOPTS_VALUE;
            }
        }
        else
        {
            // dequeue the picture
            QbPacket packet = this->m_avqueue.read("video/x-raw");

            // compute nominal last_duration
            double pts = packet.pts() * packet.timeBase().value();
            double lastDuration = pts - this->m_frameLastPts;

            if (!isnan(lastDuration) && lastDuration > 0
                && lastDuration < 10.0)
                // if duration of the last frame was sane, update last_duration in video state
                this->m_frameLastDuration = lastDuration;

            double diff;
            double delay = this->computeTargetDelay(this->m_frameLastDuration, &diff);
            double time = QDateTime::currentMSecsSinceEpoch() / 1.0e3;

            if (time < this->m_frameTimer + delay)
            {
                *remainingTime = qMin(this->m_frameTimer + delay - time, *remainingTime);

                return;
            }

            this->m_frameTimer += delay;

            if (delay > 0 && time - this->m_frameTimer > AV_SYNC_THRESHOLD_MAX)
                this->m_frameTimer = time;

            this->updateVideoPts(pts);

            this->printLog(packet, diff);

            // display picture
            emit this->oStream(packet);

            this->m_avqueue.dequeue("video/x-raw");
        }

        break;
    }
}

void SyncElement::updateVideoPts(double pts)
{
    this->m_videoClock.setClock(pts);
    this->m_extrnClock.syncTo(this->m_videoClock);
    this->m_frameLastPts = pts;
}
