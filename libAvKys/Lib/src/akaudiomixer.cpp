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

#include <QDebug>
#include <QMutex>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QQmlEngine>
#include <QVector>
#include <QWaitCondition>
#include <memory>
#include <vector>

#include "akaudiomixer.h"
#include "akaudioconverter.h"
#include "akaudiopacket.h"
#include "akfrac.h"
#include "akringbuffer.h"

#define DEFAULT_LATENCY 25

class SourceSlot
{
    public:
        AkAudioConverter converter;

        // One ring buffer per channel (planar) or a single one (interleaved).
        QVector<AkRingBuffer<float>> rings;

        // Back-pressure: one condition per slot. The producer (write) waits
        // on this when the ring buffers are full. The consumer (read) wakes it
        // after consuming data.
        QWaitCondition notFull;
        QMutex notFullMutex;

        SourceSlot()
        {

        }

        SourceSlot(const SourceSlot &other):
            converter(other.converter),
            rings(other.rings)
        {
        }

        SourceSlot &operator =(const SourceSlot &other)
        {
            if (this != &other) {
                this->converter = other.converter;
                this->rings = other.rings;
            }

            return *this;
        }
};

class AkAudioMixerPrivate
{
    public:
        size_t m_inputs {0};
        AkAudioCaps m_outputCaps;
        int m_latency {DEFAULT_LATENCY};
        std::vector<std::unique_ptr<SourceSlot>> m_slots;
        mutable QReadWriteLock m_slotsLock;
        AkAudioConverter m_outputConverter;
        qint64 m_pts {0};

        static AkAudioCaps mixCaps(const AkAudioCaps &outputCaps);
        static void normalizeFrame(AkAudioPacket &packet);
};

AkAudioMixer::AkAudioMixer(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioMixerPrivate;
}

AkAudioMixer::~AkAudioMixer()
{
    this->deallocate();
    delete this->d;
}

size_t AkAudioMixer::inputs() const
{
    QReadLocker locker(&this->d->m_slotsLock);

    return this->d->m_inputs;
}

AkAudioCaps AkAudioMixer::outputCaps() const
{
    QReadLocker locker(&this->d->m_slotsLock);

    return this->d->m_outputCaps;
}

int AkAudioMixer::latency() const
{
    QReadLocker locker(&this->d->m_slotsLock);

    return this->d->m_latency;
}

AkAudioPacket AkAudioMixer::read(size_t samples) const
{
    if (samples < 1)
        return {};

    QReadLocker locker(&this->d->m_slotsLock);

    if (this->d->m_slots.empty())
        return {};

    const auto mixCaps = AkAudioMixerPrivate::mixCaps(this->d->m_outputCaps);
    const int channels = mixCaps.channels();
    const bool planar = mixCaps.planar();
    const int nSamples = int(samples);

    AkAudioPacket mixPacket(mixCaps, nSamples, true);

    if (planar) {
        QVector<float> temporary(nSamples, 0.0f);

        for (auto &slot: this->d->m_slots) {
            for (int channel = 0; channel < channels; ++channel) {
                auto &ring = slot->rings[channel];

                // read() returns only the samples currently available.
                // The rest of temporary is already zero (silence).
                const int read = ring.read(temporary.data(), nSamples);

                if (read < 1)
                    continue;

                auto output = reinterpret_cast<float *>(mixPacket.plane(channel));

                for (int i = 0; i < read; ++i)
                    output[i] += temporary[i];
            }

            // Wake any producer blocked because this slot was full.
            slot->notFull.wakeAll();
        }
    } else {
        const int totalSamples = nSamples * channels;
        QVector<float> temporary(totalSamples, 0.0f);

        for (auto &slot: this->d->m_slots) {
            auto &ring = slot->rings[0];

            const int read = ring.read(temporary.data(), totalSamples);

            if (read < 1)
                continue;

            auto output = reinterpret_cast<float *>(mixPacket.data());

            for (int i = 0; i < read; ++i)
                output[i] += temporary[i];

            // Wake any producer blocked because this slot was full.
            slot->notFull.wakeAll();
        }
    }

    AkAudioMixerPrivate::normalizeFrame(mixPacket);

    auto outputPacket = this->d->m_outputConverter.convert(mixPacket);

    if (!outputPacket)
        return {};

    outputPacket.setPts(this->d->m_pts);
    outputPacket.setDuration(nSamples);
    outputPacket.setTimeBase({1, mixCaps.rate()});
    this->d->m_pts += nSamples;

    return outputPacket;
}

size_t AkAudioMixer::write(const AkAudioPacket &packet)
{
    if (!packet)
        return 0;

    const qint64 sourceId = packet.id();

    if (sourceId < 0)
        return 0;

    const int slotIndex = int(sourceId);

    AkAudioPacket mixPacket;
    bool planar = false;
    int channels = 0;
    int nSamples = 0;

    {
        QReadLocker locker(&this->d->m_slotsLock);

        if (this->d->m_slots.empty())
            return 0;

        if (slotIndex >= this->d->m_slots.size())
            return 0;

        auto *slot = this->d->m_slots[slotIndex].get();

        if (!slot)
            return 0;

        mixPacket = slot->converter.convert(packet);

        if (!mixPacket)
            return 0;

        planar = mixPacket.caps().planar();
        channels = mixPacket.caps().channels();
        nSamples = int(mixPacket.samples());

        if (nSamples < 1)
            return 0;
    }

    bool hasSpace = false;

    do {
        QMutex *mutex = nullptr;
        QWaitCondition *cond = nullptr;

        {
            QReadLocker locker(&this->d->m_slotsLock);

            if (slotIndex >= this->d->m_slots.size())
                return 0;

            auto *slot = this->d->m_slots[slotIndex].get();

            if (!slot)
                return 0;

            if (planar) {
                hasSpace = true;

                for (int ch = 0; ch < channels; ++ch)
                    if (slot->rings[ch].availableWrite() < nSamples) {
                        hasSpace = false;
                        break;
                    }
            } else {
                hasSpace = slot->rings[0].availableWrite() >= nSamples * channels;
            }

            if (!hasSpace) {
                mutex = &slot->notFullMutex;
                cond = &slot->notFull;
            }
        }

        if (!hasSpace) {
            QMutexLocker condLocker(mutex);
            cond->wait(mutex);
        }
    } while (!hasSpace);

    {
        QReadLocker locker(&this->d->m_slotsLock);

        if (slotIndex >= this->d->m_slots.size())
            return 0;

        auto *slot = this->d->m_slots[slotIndex].get();

        if (!slot)
            return 0;

        size_t written = 0;

        if (planar) {
            for (int channel = 0; channel < channels; ++channel) {
                auto sourceData = reinterpret_cast<const float *>(mixPacket.constPlane(channel));
                written += slot->rings[channel].write(sourceData, nSamples);
            }
        } else {
            auto sourceData = reinterpret_cast<const float *>(mixPacket.constData());
            written += slot->rings[0].write(sourceData, nSamples * channels);
        }

        return written;
    }
}

void AkAudioMixer::setInputs(size_t inputs)
{
    {
        QWriteLocker locker(&this->d->m_slotsLock);

        if (this->d->m_inputs == inputs)
            return;

        this->d->m_inputs = inputs;
    }

    emit this->inputsChanged(inputs);
}

void AkAudioMixer::setOutputCaps(const AkAudioCaps &outputCaps)
{
    {
        QWriteLocker locker(&this->d->m_slotsLock);

        if (this->d->m_outputCaps == outputCaps)
            return;

        this->d->m_outputCaps = outputCaps;
    }

    emit this->outputCapsChanged(outputCaps);
}

void AkAudioMixer::setLatency(int latency)
{
    {
        QWriteLocker locker(&this->d->m_slotsLock);

        if (this->d->m_latency == latency)
            return;

        this->d->m_latency = latency;
    }

    emit this->latencyChanged(latency);
}

void AkAudioMixer::resetInputs()
{
    this->setInputs(0);
}

void AkAudioMixer::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkAudioMixer::resetLatency()
{
    this->setLatency(DEFAULT_LATENCY);
}

bool AkAudioMixer::allocate()
{
    this->deallocate();

    QWriteLocker locker(&this->d->m_slotsLock);

    if (this->d->m_inputs < 1) {
        qCritical() << "You must define 1 or more inputs for the Mixer";

        return false;
    }

    if (!this->d->m_outputCaps) {
        qCritical() << "Mixer output caps not set";

        return false;
    }

    auto mixCaps = AkAudioMixerPrivate::mixCaps(this->d->m_outputCaps);
    int rate = mixCaps.rate();

    if (rate < 1)
        rate = 44100;

    int channels = mixCaps.channels();

    if (channels < 1)
        channels = 1;

    bool planar = mixCaps.planar();
    int nSamples = this->d->m_latency * rate / 1000;

    if (nSamples < 1)
        nSamples = 1;

    this->d->m_slots.resize(this->d->m_inputs);

    for (auto &slot: this->d->m_slots) {
        slot = std::make_unique<SourceSlot>();
        slot->converter.setOutputCaps(mixCaps);

        if (planar) {
            for (int ch = 0; ch < channels; ++ch)
                slot->rings << AkRingBuffer<float>(nSamples);
        } else {
            slot->rings << AkRingBuffer<float>(nSamples * channels);
        }
    }

    this->d->m_outputConverter.setOutputCaps(this->d->m_outputCaps);
    this->d->m_pts = 0;

    return true;
}

void AkAudioMixer::deallocate()
{
    QWriteLocker locker(&this->d->m_slotsLock);

    // Wake any producers blocked in write() before clearing the slots.
    for (auto &slot: this->d->m_slots)
        if (slot)
            slot->notFull.wakeAll();

    // Move to a local variable so slots outlive the write-lock.
    // This guarantees that any thread still in wait() sees valid
    // QMutex/QWaitCondition objects until it returns from wait().
    auto oldSlots = std::move(this->d->m_slots);
}

void AkAudioMixer::registerTypes()
{
    qRegisterMetaType<AkAudioMixer>("AkAudioMixer");
    qmlRegisterType<AkAudioMixer>("Ak", 1, 0, "AkAudioMixer");
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

void AkAudioMixerPrivate::normalizeFrame(AkAudioPacket &packet)
{
    const auto caps = packet.caps();
    const int channels = caps.channels();
    const int nSamples = int(packet.samples());
    float peak = 1.0f;

    if (caps.planar()) {
        for (int channel = 0; channel < channels; ++channel) {
            auto data = reinterpret_cast<float *>(packet.plane(channel));

            for (int i = 0; i < nSamples; ++i)
                peak = qMax(peak, qAbs(data[i]));
        }

        if (peak > 1.0f) {
            for (int channel = 0; channel < channels; ++channel) {
                auto data = reinterpret_cast<float *>(packet.plane(channel));

                for (int i = 0; i < nSamples; ++i)
                    data[i] /= peak;
            }
        }
    } else {
        const int totalSamples = nSamples * channels;
        auto data = reinterpret_cast<float *>(packet.data());

        for (int i = 0; i < totalSamples; ++i)
            peak = qMax(peak, qAbs(data[i]));

        if (peak > 1.0f)
            for (int i = 0; i < totalSamples; ++i)
                data[i] /= peak;
    }
}

#include "moc_akaudiomixer.cpp"
