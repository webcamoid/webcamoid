/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#include <QFile>
#include <QSettings>
#include <QThread>

#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#endif

#include "akcpufeatures.h"

class AkCpuFeaturesPrivate
{
    public:
        AkCpuFeatures *self;

        explicit AkCpuFeaturesPrivate(AkCpuFeatures *self):
            self(self)
        {
        }
};

AkCpuFeatures::AkCpuFeatures(QObject *parent):
    QObject(parent)
{
    this->d = new AkCpuFeaturesPrivate(this);
}

AkCpuFeatures::AkCpuFeatures(const AkCpuFeatures &other):
    QObject()
{
    Q_UNUSED(other)
    this->d = new AkCpuFeaturesPrivate(this);
}

AkCpuFeatures::~AkCpuFeatures()
{
    delete this->d;
}

quint64 AkCpuFeatures::frequency()
{
#ifdef Q_OS_WIN32
    // Windows: Read CPU frequency from registry
    QSettings settings("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                       QSettings::NativeFormat);
    bool ok;
    auto freqMHz = settings.value("~MHz", 0).toULongLong(&ok);

    if (ok && freqMHz > 0)
        return 1'000'000L * freqMHz;
#elif defined(Q_OS_MACOS)
    // macOS: Use sysctl to get CPU frequency
    qint64 freq;
    size_t size = sizeof(freq);

    if (sysctlbyname("hw.cpufrequency", &freq, &size, nullptr, 0) == 0
        && freq > 0) {
        return quint64(freq);
    }
#else
    QFile freqFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    if (freqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bool ok;
        auto freqKHz = QString(freqFile.readAll()).trimmed().toULongLong(&ok);
        freqFile.close();

        if (ok && freqKHz > 0)
            return 1000 * freqKHz;
    }
#endif

    return 2'000'000'000L;
}

/**
 * @brief Calculates the minimum byte threshold to enable OpenMP parallelization.
 *
 * This function estimates the minimum number of bytes that justifies using OpenMP
 * based on the computational complexity of the operation, the size of each data element,
 * the number of threads, and the OpenMP overhead. It uses SIMD instruction support
 * from AkSimd and CPU frequency to estimate the sequential processing time per byte (tByte).
 *
 * @param operationsPerByte Estimated number of operations per byte for the operation.
 * @param instructionSet best available SIMD optimizations.
 * @param threads Number of threads to use (0 or negative uses ideal thread count).
 * @param overheadNs OpenMP overhead in nanoseconds (default: 50,000 ns).
 * @return Minimum byte threshold for enabling OpenMP parallelization.
 */
size_t AkCpuFeatures::paralellizableBytesThreshold(int operationsPerByte,
                                                   AkSimd::SimdInstructionSet instructionSet,
                                                   int threads,
                                                   quint64 overheadNs)
{
    // Ensure the number of threads is at least the ideal thread count
    threads = qMax(qMax(threads, QThread::idealThreadCount()), 2);

    // Adjust operations per byte based on SIMD instruction support
    qreal simdFactor = 1.0;

    if (instructionSet == AkSimd::SimdInstructionSet_none)
        instructionSet = AkSimd::preferredInstructionSet();

    switch (instructionSet) {
    case AkSimd::SimdInstructionSet_AVX2:
    case AkSimd::SimdInstructionSet_SVE:
    case AkSimd::SimdInstructionSet_RVV:
        simdFactor = 0.5; // 50% fewer cycles due to advanced SIMD (256–512 bits)
        break;

    case AkSimd::SimdInstructionSet_SSE4_1:
        simdFactor = 0.6; // 40% fewer cycles with SSE4.1 (128 bits, enhanced instructions)
        break;

    case AkSimd::SimdInstructionSet_SSE:
    case AkSimd::SimdInstructionSet_SSE2:
    case AkSimd::SimdInstructionSet_NEON:
        simdFactor = 0.7; // 30% fewer cycles with SSE/SSE2/NEON (128 bits)
        break;

    case AkSimd::SimdInstructionSet_MMX:
        simdFactor = 0.9; // 10% fewer cycles with MMX (64 bits)
        break;

    default:
        break; // No SIMD, use baseline cycles
    }

    // Estimate cycles per byte: assume 1–2 cycles per operation, averaged at 1.5
    auto cyclesPerByte = operationsPerByte * 1.5 * simdFactor;

    // Convert cycles to nanoseconds: tByte = cycles / CPU frequency (GHz)
    auto tByte = 1e9 * cyclesPerByte / frequency();

    // Calculate threshold: B = T_overhead / [T_byte * (1 - 1/N_threads)]
    auto thresholdBytes = overheadNs / (tByte * (1.0 - 1.0 / threads));

    // Round up and ensure a minimum threshold to avoid overhead on small buffers
    const size_t minThreshold = 5000; // Minimum ~5,000 bytes

    return qMax<size_t>(thresholdBytes, minThreshold);
}

#include "moc_akcpufeatures.cpp"
