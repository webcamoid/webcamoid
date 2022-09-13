/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QFuture>
#include <QMap>
#include <QMutex>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QThreadPool>
#include <QTime>
#include <QtConcurrent>
#include <QtMath>
#include <ak.h>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>

#include "audiogenelement.h"

#define PAUSE_TIMEOUT 500

// No AV correction is done if too big error.
#define AV_NOSYNC_THRESHOLD 10.0

// Maximum audio speed change to get correct sync
#define SAMPLE_CORRECTION_PERCENT_MAX 10

// We use about AUDIO_DIFF_AVG_NB A-V differences to make the average
#define AUDIO_DIFF_AVG_NB 20

using WaveTypeMap = QMap<AudioGenElement::WaveType, QString>;

inline WaveTypeMap initWaveTypeMap()
{
    WaveTypeMap waveTypeToStr = {
        {AudioGenElement::WaveTypeSilence   , "silence"   },
        {AudioGenElement::WaveTypeSine      , "sine"      },
        {AudioGenElement::WaveTypeSquare    , "square"    },
        {AudioGenElement::WaveTypeTriangle  , "triangle"  },
        {AudioGenElement::WaveTypeSawtooth  , "sawtooth"  },
        {AudioGenElement::WaveTypeWhiteNoise, "whiteNoise"},
    };

    return waveTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(WaveTypeMap, waveTypeToStr, (initWaveTypeMap()))

class AudioGenElementPrivate
{
    public:
        AudioGenElement *self;
        AkAudioCaps m_caps {
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::Layout_mono,
            true,
            44100
        };
        AkAudioConverter m_audioConvert;
        QThreadPool m_threadPool;
        QFuture<void> m_readFramesLoopResult;
        QMutex m_mutex;
        qreal m_frequency {1000};
        qreal m_volume {1.0};
        qreal m_sampleDuration {25.0};
        qint64 m_id {-1};
        AudioGenElement::WaveType m_waveType {AudioGenElement::WaveTypeSilence};
        bool m_readFramesLoop {false};
        bool m_pause {false};

        explicit AudioGenElementPrivate(AudioGenElement *self);
        void readFramesLoop();
};

AudioGenElement::AudioGenElement():
    AkElement()
{
    this->d = new AudioGenElementPrivate(this);
}

AudioGenElement::~AudioGenElement()
{
    delete this->d;
}

AkAudioCaps AudioGenElement::caps() const
{
    return this->d->m_caps;
}

QString AudioGenElement::waveType() const
{
    return waveTypeToStr->value(this->d->m_waveType);
}

qreal AudioGenElement::frequency() const
{
    return this->d->m_frequency;
}

qreal AudioGenElement::volume() const
{
    return this->d->m_volume;
}

qreal AudioGenElement::sampleDuration() const
{
    return this->d->m_sampleDuration;
}

void AudioGenElement::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_mutex.lock();
    this->d->m_caps = caps;
    this->d->m_mutex.unlock();

    this->d->m_audioConvert.setOutputCaps(caps);
    emit this->capsChanged(caps);
}

void AudioGenElement::setWaveType(const QString &waveType)
{
    WaveType waveTypeEnum = waveTypeToStr->key(waveType,
                                                WaveTypeSilence);

    if (this->d->m_waveType == waveTypeEnum)
        return;

    this->d->m_waveType = waveTypeEnum;
    emit this->waveTypeChanged(waveType);
}

void AudioGenElement::setFrequency(qreal frequency)
{
    if (qFuzzyCompare(this->d->m_frequency, frequency))
        return;

    this->d->m_frequency = frequency;
    emit this->frequencyChanged(frequency);
}

void AudioGenElement::setVolume(qreal volume)
{
    if (qFuzzyCompare(this->d->m_volume, volume))
        return;

    this->d->m_volume = volume;
    emit this->volumeChanged(volume);
}

void AudioGenElement::setSampleDuration(qreal sampleDuration)
{
    if (qFuzzyCompare(this->d->m_sampleDuration, sampleDuration))
        return;

    this->d->m_mutex.lock();
    this->d->m_sampleDuration = sampleDuration;
    this->d->m_mutex.unlock();
    emit this->sampleDurationChanged(sampleDuration);
}

void AudioGenElement::resetCaps()
{
    this->setCaps({AkAudioCaps::SampleFormat_s16,
                   AkAudioCaps::Layout_mono,
                   true,
                   44100});
}

void AudioGenElement::resetWaveType()
{
    this->setWaveType("silence");
}

void AudioGenElement::resetFrequency()
{
    this->setFrequency(1000);
}

void AudioGenElement::resetVolume()
{
    this->setVolume(1.);
}

void AudioGenElement::resetSampleDuration()
{
    this->setSampleDuration(25.);
}

bool AudioGenElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            this->d->m_audioConvert.reset();
            this->d->m_pause = true;
            this->d->m_readFramesLoop = true;
            this->d->m_readFramesLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &AudioGenElementPrivate::readFramesLoop);

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_audioConvert.reset();
            this->d->m_id = Ak::id();
            this->d->m_pause = false;
            this->d->m_readFramesLoop = true;
            this->d->m_readFramesLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &AudioGenElementPrivate::readFramesLoop);

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_pause = false;
            this->d->m_readFramesLoop = false;
            this->d->m_readFramesLoopResult.waitForFinished();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_pause = false;

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_pause = false;
            this->d->m_readFramesLoop = false;
            this->d->m_readFramesLoopResult.waitForFinished();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

AudioGenElementPrivate::AudioGenElementPrivate(AudioGenElement *self):
    self(self)
{

}

void AudioGenElementPrivate::readFramesLoop()
{
    qint64 pts = 0;
    int t0 = QTime::currentTime().msecsSinceStartOfDay();
    static const qreal coeff = qExp(qLn(0.01) / AUDIO_DIFF_AVG_NB);
    qreal avgDiff = 0;
    int frameCount = 0;

    this->m_mutex.lock();
    int rate = this->m_caps.rate();
    qreal sampleDuration = this->m_sampleDuration;
    AkAudioCaps audioCaps(AkAudioCaps::SampleFormat_s32,
                          AkAudioCaps::Layout_mono,
                          true,
                          rate);
    this->m_mutex.unlock();

    while (this->m_readFramesLoop) {
        if (this->m_pause) {
            QThread::msleep(PAUSE_TIMEOUT);

            continue;
        }

        qreal clock = 0.;
        qreal diff = 0.;

        for (int i = 0; i < 2; i++) {
            clock = 1.e-3 * (QTime::currentTime().msecsSinceStartOfDay() - t0);
            diff = qreal(pts) / audioCaps.rate() - clock;

            // Sleep until the moment of sending the frame.
            if (!i && diff < 0)
                QThread::usleep(ulong(1e6 * qAbs(diff)));
        }

        int nSamples = qRound(audioCaps.rate() * sampleDuration / 1.e3);

        if (qAbs(diff) < AV_NOSYNC_THRESHOLD) {
            avgDiff = avgDiff * (1. - coeff) + qAbs(diff) * coeff;

            if (frameCount < AUDIO_DIFF_AVG_NB) {
                frameCount++;
            } else {
                qreal diffThreshold = 2. * nSamples / audioCaps.rate();

                if (avgDiff >= diffThreshold) {
                    int wantedSamples = qRound(nSamples + diff * audioCaps.rate());
                    int minSamples = nSamples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    int maxSamples = nSamples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    nSamples = qBound(minSamples, wantedSamples, maxSamples);
                }
            }
        } else {
            pts = qRound(clock * audioCaps.rate());
            avgDiff = 0.;
            frameCount = 0;
        }

        AkAudioPacket iPacket(audioCaps, nSamples);
        qreal time = QTime::currentTime().msecsSinceStartOfDay() / 1.e3;
        qreal tdiff = 1. / audioCaps.rate();

        if (this->m_waveType != AudioGenElement::WaveTypeSilence) {
            memset(iPacket.data(), 0, iPacket.size());
        } else if (this->m_waveType == AudioGenElement::WaveTypeWhiteNoise) {
            auto data = iPacket.data();

            for (size_t i = 0; i < iPacket.size(); ++i)
                data[i] = char(QRandomGenerator::global()->bounded(-128, 127));
        } else {
            auto ampMax = qint32(this->m_volume * std::numeric_limits<qint32>::max());
            auto ampMin = qint32(this->m_volume * std::numeric_limits<qint32>::min());
            auto t = time;
            auto buff = reinterpret_cast<qint32 *>(iPacket.data());

            if (this->m_waveType == AudioGenElement::WaveTypeSine) {
                for  (int i = 0; i < nSamples; i++, time += tdiff)
                    buff[i] = qRound(ampMax * qSin(2 * M_PI * this->m_frequency * t));
            } else if (this->m_waveType == AudioGenElement::WaveTypeSquare) {
                for  (int i = 0; i < nSamples; i++, time += tdiff)
                    buff[i] = (qRound(2 * this->m_frequency * t) & 0x1)?
                                  ampMin: ampMax;
            } else {
                qint32 mod = qRound(audioCaps.rate() / this->m_frequency);

                if (this->m_waveType == AudioGenElement::WaveTypeSawtooth) {
                    qreal k = (qreal(ampMax) - ampMin) / (mod - 1);

                    for  (int i = 0; i < nSamples; i++, time += tdiff) {
                        qint32 nsample = qRound(t / tdiff);
                        buff[i] = qRound(k * (nsample % mod) + ampMin);
                    }
                } else if (this->m_waveType == AudioGenElement::WaveTypeTriangle) {
                    mod /= 2;
                    qreal k = (qreal(ampMax) - ampMin) / (mod - 1);

                    for  (int i = 0; i < nSamples; i++, time += tdiff) {
                        qint32 nsample = qRound(t / tdiff);
                        buff[i] = (qRound(2 * this->m_frequency * t) & 0x1)?
                                      qRound(-k * (nsample % mod) + ampMax):
                                      qRound( k * (nsample % mod) + ampMin);
                    }
                }
            }
        }

        iPacket.setPts(pts);
        iPacket.setTimeBase({1, audioCaps.rate()});
        iPacket.setIndex(0);
        iPacket.setId(this->m_id);

        auto outPacket = this->m_audioConvert.convert(iPacket);
        emit self->oStream(outPacket);
        pts += nSamples;
    }
}

#include "moc_audiogenelement.cpp"
