/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QElapsedTimer>
#include <QFuture>
#include <QMutex>
#include <QQmlEngine>
#include <QThread>
#include <QThreadPool>
#include <QVector>
#include <QWaitCondition>
#include <QtConcurrent>

#include "akaudiomixer.h"
#include "ak.h"
#include "akaudioconverter.h"
#include "akaudiopacket.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akringbuffer.h"

// Ring buffer capacity as a multiple of the computed frame size. 4x gives enough
// headroom to absorb bursts without overrun while keeping latency low.
#define RING_BUFFER_MULTIPLIER 4

struct SourceSlot
{
    AkAudioConverter converter;

    // One ring buffer per channel (planar) or a single one (interleaved).
    QVector<AkRingBuffer<float>> rings;
};

class AkAudioMixerPrivate
{
    public:
        AkAudioMixer *self;
        size_t m_inputs {0};
        AkAudioCaps m_outputCaps;
        AkAudioConverter m_outputConverter;
        int m_latency {25};
        QVector<SourceSlot> m_slots;
        QMutex m_slotsMutex;

        // Configured state (may change while thread is running).
        AkElement::ElementState m_state {AkElement::ElementStateNull};

        // Mix thread synchronisation.
        QThreadPool m_threadPool;
        QFuture<void> m_mixLoopResult;
        QMutex m_mixMutex;
        QWaitCondition m_mixCond;
        bool m_run {false};

        explicit AkAudioMixerPrivate(AkAudioMixer *self);
        ~AkAudioMixerPrivate();

        static AkAudioCaps mixCaps(const AkAudioCaps &outputCaps);
        void mixLoop();
        bool allocateSources();
        void clearSources();
        void startMixLoop();
        void stopMixLoop();
        static void normalizeFrame(AkAudioPacket &packet);
};

AkAudioMixer::AkAudioMixer(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioMixerPrivate(this);
}

AkAudioMixer::~AkAudioMixer()
{
    delete this->d;
}

size_t AkAudioMixer::inputs() const
{
    return this->d->m_inputs;
}

AkAudioCaps AkAudioMixer::outputCaps() const
{
    return this->d->m_outputCaps;
}

int AkAudioMixer::latency() const
{
    return this->d->m_latency;
}

AkElement::ElementState AkAudioMixer::state() const
{
    return this->d->m_state;
}

void AkAudioMixer::setOutputCaps(const AkAudioCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_outputCaps = outputCaps;
    this->d->m_outputConverter.setOutputCaps(outputCaps);
    emit this->outputCapsChanged(outputCaps);
}

void AkAudioMixer::setLatency(int latency)
{
    if (this->d->m_latency == latency)
        return;

    this->d->m_latency = latency;
    emit this->latencyChanged(latency);
}

bool AkAudioMixer::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return false;

    if (state == AkElement::ElementStatePlaying) {
        {
            QMutexLocker locker(&this->d->m_slotsMutex);

            if (!this->d->allocateSources())
                return false;
        }

        this->d->startMixLoop();
    } else {
        this->d->stopMixLoop();
        this->d->clearSources();
    }

    this->d->m_state = state;
    emit this->stateChanged(state);

    return true;
}

void AkAudioMixer::resetInputs()
{
    this->setInputs(0);
}

void AkAudioMixer::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkAudioMixer::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void AkAudioMixer::resetLatency()
{
    this->setLatency(25);
}

AkPacket AkAudioMixer::iStream(const AkPacket &packet)
{
    if (this->d->m_state != AkElement::ElementStatePlaying)
        return {};

    AkAudioPacket audioPacket(packet);

    if (!audioPacket)
        return {};

    const size_t id = size_t(packet.id());

    QMutexLocker locker(&this->d->m_slotsMutex);

    if (int(id) >= this->d->m_slots.size())
        return {};

    auto &slot = this->d->m_slots[int(id)];

    // Convert incoming packet to mix format.
    auto mixPacket = slot.converter.convert(audioPacket);

    if (!mixPacket)
        return {};

    bool planar   = mixPacket.caps().planar();
    int  channels = mixPacket.caps().channels();
    auto samples  = int(mixPacket.samples());

    // Write converted samples into the ring buffer(s).
    if (planar) {
        for (int c = 0; c < channels; ++c) {
            auto src = reinterpret_cast<const float *>(mixPacket.constPlane(c));
            slot.rings[c].write(src, samples);
        }
    } else {
        auto src = reinterpret_cast<const float *>(mixPacket.constData());
        slot.rings[0].write(src, samples * channels);
    }

    locker.unlock();

    // Wake the mix thread.
    this->d->m_mixMutex.lock();
    this->d->m_mixCond.wakeOne();
    this->d->m_mixMutex.unlock();

    return {};
}

void AkAudioMixer::setInputs(size_t inputs)
{
    if (this->d->m_inputs == inputs)
        return;

    this->d->m_inputs = inputs;
    emit this->inputsChanged(inputs);
}

void AkAudioMixer::registerTypes()
{
    qRegisterMetaType<AkAudioMixer>("AkAudioMixer");
    qmlRegisterType<AkAudioMixer>("Ak", 1, 0, "AkAudioMixer");
}

AkAudioMixerPrivate::AkAudioMixerPrivate(AkAudioMixer *self):
    self(self)
{
}

AkAudioMixerPrivate::~AkAudioMixerPrivate()
{
    this->stopMixLoop();
}

AkAudioCaps AkAudioMixerPrivate::mixCaps(const AkAudioCaps &outputCaps)
{
    // Only SampleFormat_flt is forced to simplify the accumulation arithmetic.
    // Layout, planar and rate are inherited from outputCaps to avoid
    // unnecessary conversions in m_outputConverter.
    return {AkAudioCaps::SampleFormat_flt,
            outputCaps.layout(),
            outputCaps.planar(),
            outputCaps.rate()};
}

bool AkAudioMixerPrivate::allocateSources()
{
    if (this->m_inputs < 1) {
        qCritical() << "You must define 1 or more inputs for the Mixer";

        return false;
    }

    if (!this->m_outputCaps) {
        qCritical() << "Mixer output caps not set";

        return false;
    }

    auto caps     = mixCaps(this->m_outputCaps);
    int  channels = caps.channels();
    bool planar   = caps.planar();
    int  nSamples = this->m_latency * caps.rate() / 1000;
    int  ringCap  = RING_BUFFER_MULTIPLIER * nSamples;

    this->m_slots.resize(this->m_inputs);

    for (auto &slot: this->m_slots) {
        slot.converter.setOutputCaps(caps);

        // Allocate one ring per channel for planar, one ring for interleaved.
        int nRings = planar? channels: 1;
        int samplesPerRing = planar? ringCap: ringCap * channels;
        slot.rings.resize(nRings);

        for (auto &ring: slot.rings)
            ring.allocate(samplesPerRing);
    }

    return true;
}

void AkAudioMixerPrivate::clearSources()
{
    this->m_slots.clear();
}

void AkAudioMixerPrivate::startMixLoop()
{
    if (this->m_run)
        return;

    this->m_run = true;
    this->m_mixLoopResult =
            QtConcurrent::run(&this->m_threadPool,
                              &AkAudioMixerPrivate::mixLoop,
                              this);
}

void AkAudioMixerPrivate::stopMixLoop()
{
    if (!this->m_run)
        return;

    this->m_run = false;
    this->m_mixMutex.lock();
    this->m_mixCond.wakeAll();
    this->m_mixMutex.unlock();
    this->m_mixLoopResult.waitForFinished();
}

void AkAudioMixerPrivate::mixLoop()
{
    auto caps     = AkAudioMixerPrivate::mixCaps(this->m_outputCaps);
    int  channels = caps.channels();
    bool planar   = caps.planar();
    int  nSamples = this->m_latency * this->m_outputCaps.rate() / 1000;

    // Duration of one output frame in microseconds.
    qint64 frameDurationUs = qint64(nSamples) * 1000000LL / caps.rate();

    int tmpSize = planar ? nSamples : channels * nSamples;
    auto tmp = new float[tmpSize];
    AkAudioPacket mixPacket(caps, nSamples);
    qint64 streamId = Ak::id();

    QElapsedTimer et;
    et.start();
    qint64 nextWakeUs = et.nsecsElapsed() / 1000;

    while (this->m_run) {
        // Sleep until either new data arrives or the next frame deadline.
        qint64 nowUs   = et.nsecsElapsed() / 1000;
        qint64 sleepMs = (nextWakeUs - nowUs + 999) / 1000;

        if (sleepMs > 0) {
            this->m_mixMutex.lock();
            this->m_mixCond.wait(&this->m_mixMutex, ulong(sleepMs));
            this->m_mixMutex.unlock();
        }

        if (!this->m_run)
            break;

        // Not yet time to produce the next frame — go back to sleep.
        nowUs = et.nsecsElapsed() / 1000;

        if (nowUs < nextWakeUs)
            continue;

        nextWakeUs += frameDurationUs;

        memset(mixPacket.data(), 0, mixPacket.size());

        this->m_slotsMutex.lock();

        for (auto &slot: this->m_slots) {
            if (planar) {
                for (int c = 0; c < channels; ++c) {
                    // Only mix this channel if enough real samples are available.
                    // If not, it contributes silence (buffer is already zeroed).
                    if (slot.rings[c].availableRead() >= nSamples) {
                        slot.rings[c].read(tmp, nSamples);
                        auto acc = reinterpret_cast<float *>(mixPacket.plane(c));

                        for (int i = 0; i < nSamples; ++i)
                            acc[i] += tmp[i];
                    }
                }
            } else {
                // Interleaved: single ring holds channels * nSamples floats.
                int needed = channels * nSamples;

                if (slot.rings[0].availableRead() >= needed) {
                    slot.rings[0].read(tmp, needed);
                    auto acc = reinterpret_cast<float *>(mixPacket.data());

                    for (int i = 0; i < needed; ++i)
                        acc[i] += tmp[i];
                }
            }
        }

        this->m_slotsMutex.unlock();

        normalizeFrame(mixPacket);

        // Convert to outputCaps if needed, then emit.
        auto converted = this->m_outputConverter.convert(mixPacket);
        converted.setPts(et.elapsed() * caps.rate() / 1000);
        converted.setDuration(nSamples);
        converted.setTimeBase({1, caps.rate()});
        converted.setIndex(0);
        converted.setId(streamId);

        if (converted)
            emit this->self->oStream(converted);
    }

    delete[] tmp;
}

void AkAudioMixerPrivate::normalizeFrame(AkAudioPacket &packet)
{
    auto data = reinterpret_cast<float *>(packet.data());
    size_t totalSamples = packet.size() / sizeof(float);
    float peak = 1.0f;

    for (size_t i = 0; i < totalSamples; ++i) {
        float value = qAbs(data[i]);

        if (value > peak)
            peak = value;
    }

    if (peak > 1.0f)
        for (int i = 0; i < totalSamples; ++i)
            data[i] /= peak;
}

#include "moc_akaudiomixer.cpp"
