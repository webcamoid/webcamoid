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

#include <random>
#include <QtMath>

#include "audiogenelement.h"

#define PAUSE_TIMEOUT 500

// No AV correction is done if too big error.
#define AV_NOSYNC_THRESHOLD 10.0

// Maximum audio speed change to get correct sync
#define SAMPLE_CORRECTION_PERCENT_MAX 10

// We use about AUDIO_DIFF_AVG_NB A-V differences to make the average
#define AUDIO_DIFF_AVG_NB 20

typedef QMap<AudioGenElement::WaveType, QString> WaveTypeMap;

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

AudioGenElement::AudioGenElement(): AkElement()
{
    this->m_caps = "audio/x-raw,format=flt,bps=4,channels=1,rate=44100,layout=mono,align=false";
    this->m_waveType = WaveTypeSilence;
    this->m_frequency = 1000;
    this->m_volume = 1.;
    this->m_sampleDuration = 25.;
    this->m_audioConvert = AkElement::create("ACapsConvert");
    this->m_readFramesLoop = false;
    this->m_pause = false;

    QObject::connect(this->m_audioConvert.data(),
                     SIGNAL(oStream(const AkPacket &)),
                     this,
                     SIGNAL(oStream(const AkPacket &)),
                     Qt::DirectConnection);
}

QString AudioGenElement::caps() const
{
    return this->m_caps.toString();
}

QString AudioGenElement::waveType() const
{
    return waveTypeToStr->value(this->m_waveType);
}

qreal AudioGenElement::frequency() const
{
    return this->m_frequency;
}

qreal AudioGenElement::volume() const
{
    return this->m_volume;
}

qreal AudioGenElement::sampleDuration() const
{
    return this->m_sampleDuration;
}

void AudioGenElement::readFramesLoop(AudioGenElement *self)
{
    qint64 pts = 0;
    int t0 = QTime::currentTime().msecsSinceStartOfDay();
    static const qreal coeff = qExp(qLn(0.01) / AUDIO_DIFF_AVG_NB);
    qreal avgDiff = 0;
    int frameCount = 0;

    while (self->m_readFramesLoop) {
        if (self->m_pause) {
            QThread::msleep(PAUSE_TIMEOUT);

            continue;
        }

        self->m_mutex.lock();
        AkCaps oCaps = self->m_caps;
        qreal sampleDuration = self->m_sampleDuration;
        self->m_mutex.unlock();

        AkAudioCaps oAudioCaps(oCaps);

        qreal clock = 0.;
        qreal diff = 0.;

        for (int i = 0; i < 2; i++) {
            clock = 1.e-3 * (QTime::currentTime().msecsSinceStartOfDay() - t0);
            diff = qreal(pts) / oAudioCaps.rate() - clock;

            // Sleep until the moment of sending the frame.
            if (!i && diff < 0)
                QThread::usleep(ulong(1e6 * qAbs(diff)));
        }

        int nSamples = qRound(oAudioCaps.rate() * sampleDuration / 1.e3);

        if (qAbs(diff) < AV_NOSYNC_THRESHOLD) {
            avgDiff = avgDiff * (1. - coeff) + qAbs(diff) * coeff;

            if (frameCount < AUDIO_DIFF_AVG_NB) {
                frameCount++;
            } else {
                qreal diffThreshold = 2. * nSamples / oAudioCaps.rate();

                if (avgDiff >= diffThreshold) {
                    int wantedSamples = qRound(nSamples + diff * oAudioCaps.rate());
                    int minSamples = nSamples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    int maxSamples = nSamples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    nSamples = qBound(minSamples, wantedSamples, maxSamples);
                }
            }
        } else {
            pts = qRound(clock * oAudioCaps.rate());
            avgDiff = 0.;
            frameCount = 0;
        }

        size_t bufferSize = sizeof(qint32) * size_t(nSamples);

        QByteArray iBuffer(int(bufferSize), Qt::Uninitialized);
        qreal time = QTime::currentTime().msecsSinceStartOfDay() / 1.e3;
        qreal tdiff = 1. / oAudioCaps.rate();

        if (self->m_waveType == WaveTypeSilence) {
            iBuffer.fill(0);
        } else if (self->m_waveType == WaveTypeWhiteNoise) {
            static std::default_random_engine engine;
            static std::uniform_int_distribution<int> distribution(-128, 127);

            for (int i = 0; i < iBuffer.size(); i++)
                iBuffer[i] = char(distribution(engine));
        } else {
            qint32 ampMax = qint32(self->m_volume * std::numeric_limits<qint32>::max());
            qint32 ampMin = qint32(self->m_volume * std::numeric_limits<qint32>::min());
            qreal t = time;
            qint32 *buff = reinterpret_cast<qint32 *>(iBuffer.data());

            if (self->m_waveType == WaveTypeSine) {
                for  (int i = 0; i < nSamples; i++, time += tdiff)
                    buff[i] = qRound(ampMax * qSin(2 * M_PI * self->m_frequency * t));
            } else if (self->m_waveType == WaveTypeSquare) {
                for  (int i = 0; i < nSamples; i++, time += tdiff)
                    buff[i] = qRound(2 * self->m_frequency * t) & 0x1?
                                  ampMin: ampMax;
            } else {
                qint32 mod = qRound(oAudioCaps.rate() / self->m_frequency);

                if (self->m_waveType == WaveTypeSawtooth) {
                    qreal k = (qreal(ampMax) - ampMin) / (mod - 1);

                    for  (int i = 0; i < nSamples; i++, time += tdiff) {
                        qint32 nsample = qRound(t / tdiff);
                        buff[i] = qRound(k * (nsample % mod) + ampMin);
                    }
                } else if (self->m_waveType == WaveTypeTriangle) {
                    mod /= 2;
                    qreal k = (qreal(ampMax) - ampMin) / (mod - 1);

                    for  (int i = 0; i < nSamples; i++, time += tdiff) {
                        qint32 nsample = qRound(t / tdiff);

                        buff[i] = qRound(2 * self->m_frequency * t) & 0x1?
                                      qRound(-k * (nsample % mod) + ampMax):
                                      qRound( k * (nsample % mod) + ampMin);
                    }
                }
            }
        }

        AkAudioCaps iAudioCaps(oAudioCaps);
        iAudioCaps.format() = AkAudioCaps::SampleFormat_s32;
        iAudioCaps.bps() = sizeof(qint32);
        iAudioCaps.channels() = 1;
        iAudioCaps.layout() = AkAudioCaps::Layout_mono;
        iAudioCaps.samples() = nSamples;

        AkAudioPacket iPacket(iAudioCaps, iBuffer);

        iPacket.pts() = pts;
        iPacket.timeBase() = AkFrac(1, iAudioCaps.rate());
        iPacket.index() = 0;
        iPacket.id() = self->m_id;

        (*self->m_audioConvert)(iPacket.toPacket());

        pts += nSamples;
    }
}

void AudioGenElement::setCaps(const QString &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_mutex.lock();
    this->m_caps = caps;
    this->m_mutex.unlock();
    this->m_audioConvert->setProperty("caps", caps);
    emit this->capsChanged(caps);
}

void AudioGenElement::setWaveType(const QString &waveType)
{
    WaveType waveTypeEnum = waveTypeToStr->key(waveType,
                                                WaveTypeSilence);

    if (this->m_waveType == waveTypeEnum)
        return;

    this->m_waveType = waveTypeEnum;
    emit this->waveTypeChanged(waveType);
}

void AudioGenElement::setFrequency(qreal frequency)
{
    if (qFuzzyCompare(this->m_frequency, frequency))
        return;

    this->m_frequency = frequency;
    emit this->frequencyChanged(frequency);
}

void AudioGenElement::setVolume(qreal volume)
{
    if (qFuzzyCompare(this->m_volume, volume))
        return;

    this->m_volume = volume;
    emit this->volumeChanged(volume);
}

void AudioGenElement::setSampleDuration(qreal sampleDuration)
{
    if (qFuzzyCompare(this->m_sampleDuration, sampleDuration))
        return;

    this->m_mutex.lock();
    this->m_sampleDuration = sampleDuration;
    this->m_mutex.unlock();
    emit this->sampleDurationChanged(sampleDuration);
}

void AudioGenElement::resetCaps()
{
    this->setCaps("audio/x-raw,format=flt,bps=4,channels=1,rate=44100,layout=mono,align=false");
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
            this->m_audioConvert->setState(state);
            this->m_pause = true;
            this->m_readFramesLoop = true;
            this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                             this->readFramesLoop,
                                                             this);

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            this->m_audioConvert->setState(state);
            this->m_id = Ak::id();
            this->m_pause = false;
            this->m_readFramesLoop = true;
            this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                             this->readFramesLoop,
                                                             this);

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
            this->m_pause = false;
            this->m_readFramesLoop = false;
            this->m_readFramesLoopResult.waitForFinished();
            this->m_audioConvert->setState(state);

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->m_audioConvert->setState(state);
            this->m_pause = false;

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->m_pause = false;
            this->m_readFramesLoop = false;
            this->m_readFramesLoopResult.waitForFinished();
            this->m_audioConvert->setState(state);

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->m_pause = true;
            this->m_audioConvert->setState(state);

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}
