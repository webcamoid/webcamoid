/* Webcamoid, webcam capture application.
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
#include <QString>
#include <QSysInfo>
#include <QRegularExpression>

#ifdef Q_OS_WIN32
// For __cpuid on Windows (x86/x86_64)
#include <intrin.h>
#elif defined(Q_OS_UNIX)
    #ifdef Q_PROCESSOR_X86
        #include <cpuid.h>
    #endif

    #ifdef Q_OS_MACOS
        #include <sys/sysctl.h>
        #include <mach/machine.h>
    #endif
#endif

#include "aksimd.h"
#include "akpluginmanager.h"
#include "iak/aksimdoptimizations.h"

struct SimdSidToStr
{
    AkSimd::SimdInstructionSet sid;
    const char *str;

    static const SimdSidToStr *table()
    {
        static const SimdSidToStr akSimdSidToStr[] = {
            {AkSimd::SimdInstructionSet_MMX   , "MMX"   },
            {AkSimd::SimdInstructionSet_SSE   , "SSE"   },
            {AkSimd::SimdInstructionSet_SSE2  , "SSE2"  },
            {AkSimd::SimdInstructionSet_SSE4_1, "SSE4.1"},
            {AkSimd::SimdInstructionSet_AVX   , "AVX"   },
            {AkSimd::SimdInstructionSet_AVX2  , "AVX2"  },
            {AkSimd::SimdInstructionSet_NEON  , "NEON"  },
            {AkSimd::SimdInstructionSet_SVE   , "SVE"   },
            {AkSimd::SimdInstructionSet_RVV   , "RVV"   },
            {AkSimd::SimdInstructionSet_none  , ""      },
        };

        return akSimdSidToStr;
    }

    static const SimdSidToStr *bySid(AkSimd::SimdInstructionSet sid)
    {
        auto it = table();

        for (; it->sid != AkSimd::SimdInstructionSet_none; ++it)
            if (it->sid == sid)
                return it;

        return it;
    }

    static const SimdSidToStr *byStr(const char *str)
    {
        auto it = table();

        for (; it->sid != AkSimd::SimdInstructionSet_none; ++it)
            if (strncmp(it->str, str, 1024) == 0)
                return it;

        return it;
    }
};

class AkSimdPrivate
{
    public:
        AkSimd *self;
        AkSimdOptimizationsPtr m_simdPlugin;
        AkSimd::SimdInstructionSet m_instructionSet {AkSimd::SimdInstructionSet_none};

        static bool haveMMX();
        static bool haveSSE();
        static bool haveSSE2();
        static bool haveSSE4_1();
        static bool haveAVX();
        static bool haveAVX2();
        static bool haveNEON();
        static bool haveSVE();
        static bool haveRVV();

        explicit AkSimdPrivate(AkSimd *self):
            self(self)
        {

        }
};

AkSimd::AkSimd(QObject *parent):
    QObject(parent)
{
    this->d = new AkSimdPrivate(this);
}

AkSimd::AkSimd(const QString &name, SimdInstructionSet wanted, QObject *parent):
    QObject(parent)
{
    this->d = new AkSimdPrivate(this);
    this->load(name, wanted);
}

AkSimd::AkSimd(const AkSimd &other):
    QObject()
{
    Q_UNUSED(other)
    this->d = new AkSimdPrivate(this);
}

AkSimd::~AkSimd()
{
    delete this->d;
}

bool AkSimd::load(const QString &name,
                  SimdInstructionSet wanted)
{
    this->d->m_instructionSet = SimdInstructionSet_none;
    this->d->m_simdPlugin = {};
    auto prefix = QString("SimdOptimizations/%1/Impl/").arg(name);

    if (wanted != SimdInstructionSet_none) {
        auto ss = SimdSidToStr::bySid(wanted);

        if (ss->sid == SimdInstructionSet_none)
            return false;

        auto pluginId = QString("%1%2").arg(prefix).arg(ss->str);
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginId);

        if (this->d->m_simdPlugin.isNull())
            return false;

        this->d->m_instructionSet = wanted;

        return true;
    }

    auto plugins =
            akPluginManager->listPlugins(QString("^%1").arg(prefix),
                                         {"SimdOptimizations"},
                                         AkPluginManager::FilterRegexp);

    if (plugins.isEmpty())
        return false;

    SimdInstructionSet selectedIntructionSet = wanted;

#ifdef Q_PROCESSOR_X86
    auto pluginIdAVX2 = QString("%1AVX2").arg(prefix);
    auto pluginIdAVX = QString("%1AVX").arg(prefix);
    auto pluginIdSSE2 = QString("%1SSE2").arg(prefix);
    auto pluginIdSSE4_1 = QString("%1SSE4.1").arg(prefix);
    auto pluginIdSSE = QString("%1SSE").arg(prefix);
    auto pluginIdMMX = QString("%1MMX").arg(prefix);

    if (AkSimdPrivate::haveAVX2() && plugins.contains(pluginIdAVX2)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdAVX2);
        selectedIntructionSet = SimdInstructionSet_AVX2;
    } else if (AkSimdPrivate::haveAVX() && plugins.contains(pluginIdAVX)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdAVX);
        selectedIntructionSet = SimdInstructionSet_AVX;
    } else if (AkSimdPrivate::haveSSE4_1() && plugins.contains(pluginIdSSE4_1)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdSSE4_1);
        selectedIntructionSet = SimdInstructionSet_SSE4_1;
    } else if (AkSimdPrivate::haveSSE2() && plugins.contains(pluginIdSSE2)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdSSE2);
        selectedIntructionSet = SimdInstructionSet_SSE2;
    } else if (AkSimdPrivate::haveSSE() && plugins.contains(pluginIdSSE)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdSSE);
        selectedIntructionSet = SimdInstructionSet_SSE;
    } else if (AkSimdPrivate::haveMMX() && plugins.contains(pluginIdMMX)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdMMX);
        selectedIntructionSet = SimdInstructionSet_MMX;
    }
#elif defined(Q_PROCESSOR_ARM)
    auto pluginIdNEON = QString("%1NEON").arg(prefix);
    auto pluginIdSVE = QString("%1SVE").arg(prefix);

    if (AkSimdPrivate::haveSVE() && plugins.contains(pluginIdSVE)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdSVE);
        selectedIntructionSet = SimdInstructionSet_SVE;
    } else if (AkSimdPrivate::haveNEON() && plugins.contains(pluginIdNEON)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdNEON);
        selectedIntructionSet = SimdInstructionSet_NEON;
    }
#elif defined(Q_PROCESSOR_RISCV)
    auto pluginIdRVV = QString("%1RVV").arg(prefix);

    if (AkSimdPrivate::haveRVV() && plugins.contains(pluginIdRVV)) {
        this->d->m_simdPlugin =
                akPluginManager->create<AkSimdOptimizations>(pluginIdRVV);
        selectedIntructionSet = SimdInstructionSet_RVV;
    }
#endif

    if (this->d->m_simdPlugin.isNull())
        return false;

    this->d->m_instructionSet = selectedIntructionSet;

    return true;
}

AkSimd::SimdInstructionSet AkSimd::loadedInstructionSet() const
{
    return this->d->m_instructionSet;
}

QFunctionPointer AkSimd::resolve(const char *functionName) const
{
    if (this->d->m_simdPlugin)
        return this->d->m_simdPlugin->resolve(functionName);

    return nullptr;
}

AkSimd::SimdInstructionSets AkSimd::supportedInstructions()
{
    SimdInstructionSets instructions = SimdInstructionSet_none;

#ifdef Q_PROCESSOR_X86
    if (AkSimdPrivate::haveMMX())
        instructions |= SimdInstructionSet_MMX;

    if (AkSimdPrivate::haveSSE())
        instructions |= SimdInstructionSet_SSE;

    if (AkSimdPrivate::haveSSE2())
        instructions |= SimdInstructionSet_SSE2;

    if (AkSimdPrivate::haveSSE4_1())
        instructions |= SimdInstructionSet_SSE4_1;

    if (AkSimdPrivate::haveAVX())
        instructions |= SimdInstructionSet_AVX;

    if (AkSimdPrivate::haveAVX2())
        instructions |= SimdInstructionSet_AVX2;
#elif defined(Q_PROCESSOR_ARM)
    if (AkSimdPrivate::haveNEON())
        instructions |= SimdInstructionSet_NEON;

    if (AkSimdPrivate::haveSVE())
        instructions |= SimdInstructionSet_SVE;
#elif defined(Q_PROCESSOR_RISCV)
    if (AkSimdPrivate::haveRVV())
        instructions |= SimdInstructionSet_RVV;
#endif

    return instructions;
}

AkSimd::SimdInstructionSet AkSimd::preferredInstructionSet(SimdInstructionSets instructionSets)
{
#ifdef Q_PROCESSOR_X86
    if (AkSimdPrivate::haveAVX2()
        && instructionSets.testFlag(SimdInstructionSet_AVX2))
        return SimdInstructionSet_AVX2;

    if (AkSimdPrivate::haveAVX()
        && instructionSets.testFlag(SimdInstructionSet_AVX))
        return SimdInstructionSet_AVX;

    if (AkSimdPrivate::haveSSE4_1()
        && instructionSets.testFlag(SimdInstructionSet_SSE4_1))
        return SimdInstructionSet_SSE4_1;

    if (AkSimdPrivate::haveSSE2()
        && instructionSets.testFlag(SimdInstructionSet_SSE2))
        return SimdInstructionSet_SSE2;

    if (AkSimdPrivate::haveSSE()
        && instructionSets.testFlag(SimdInstructionSet_SSE))
        return SimdInstructionSet_SSE;

    if (AkSimdPrivate::haveMMX()
        && instructionSets.testFlag(SimdInstructionSet_MMX))
        return SimdInstructionSet_MMX;
#elif defined(Q_PROCESSOR_ARM)
    if (AkSimdPrivate::haveSVE()
        && instructionSets.testFlag(SimdInstructionSet_SVE))
        return SimdInstructionSet_SVE;

    if (AkSimdPrivate::haveNEON()
        && instructionSets.testFlag(SimdInstructionSet_NEON))
        return SimdInstructionSet_NEON;
#elif defined(Q_PROCESSOR_RISCV)
    if (AkSimdPrivate::haveRVV()
        && instructionSets.testFlag(SimdInstructionSet_RVV))
        return SimdInstructionSet_RVV;
#endif

    return SimdInstructionSet_none;
}

AkSimd::SimdInstructionSet AkSimd::preferredInstructionSet()
{
#ifdef Q_PROCESSOR_X86
    if (AkSimdPrivate::haveAVX2())
        return SimdInstructionSet_AVX2;

    if (AkSimdPrivate::haveAVX())
        return SimdInstructionSet_AVX;

    if (AkSimdPrivate::haveSSE4_1())
        return SimdInstructionSet_SSE4_1;

    if (AkSimdPrivate::haveSSE2())
        return SimdInstructionSet_SSE2;

    if (AkSimdPrivate::haveSSE())
        return SimdInstructionSet_SSE;

    if (AkSimdPrivate::haveMMX())
        return SimdInstructionSet_MMX;
#elif defined(Q_PROCESSOR_ARM)
    if (AkSimdPrivate::haveSVE())
        return SimdInstructionSet_SVE;

    if (AkSimdPrivate::haveNEON())
        return SimdInstructionSet_NEON;
#elif defined(Q_PROCESSOR_RISCV)
    if (AkSimdPrivate::haveRVV())
        return SimdInstructionSet_RVV;
#endif

    return SimdInstructionSet_none;
}

int AkSimd::preferredAlign(SimdInstructionSet wanted)
{
    if (wanted != SimdInstructionSet_none)
        switch (wanted) {
        case SimdInstructionSet_AVX2:
        case SimdInstructionSet_AVX:
        case SimdInstructionSet_SVE:
        case SimdInstructionSet_RVV:
            return 32;
        case SimdInstructionSet_SSE4_1:
        case SimdInstructionSet_SSE2:
        case SimdInstructionSet_SSE:
        case SimdInstructionSet_NEON:
            return 16;
        default:
            return 8;
        }

#ifdef Q_PROCESSOR_X86
    if (AkSimdPrivate::haveAVX() || AkSimdPrivate::haveAVX2())
        return 32;
    else if (AkSimdPrivate::haveSSE()
             || AkSimdPrivate::haveSSE2()
             || AkSimdPrivate::haveSSE4_1())
        return 16;
#elif defined(Q_PROCESSOR_ARM)
    if (AkSimdPrivate::haveNEON())
        return 32;
    else if (AkSimdPrivate::haveNEON())
        return 16;
#elif defined(Q_PROCESSOR_RISCV)
    if (AkSimdPrivate::haveRVV())
        return 32;
#endif

    return 8;
}

void AkSimd::afree(void *ptr)
{
#ifdef Q_OS_WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void *AkSimd::amalloc(size_t size, int align)
{
#ifdef Q_OS_WIN32
    return _aligned_malloc(size, align);
#else
    void *ptr = nullptr;

    if (posix_memalign(&ptr, align, size) == 0)
        return ptr;

    // Over-allocate to ensure alignment

    return malloc(size + align - 1);
#endif
}

void *AkSimd::amalloc(size_t size)
{
    return amalloc(size, preferredAlign());
}

QString AkSimd::instructionSetToString(SimdInstructionSet instructionSet)
{
    return {SimdSidToStr::bySid(instructionSet)->str};
}

bool AkSimdPrivate::haveMMX()
{
#ifdef Q_PROCESSOR_X86
    static bool akSimdMMXDetected = false;
    static bool akSimdHaveMMX = false;

    if (akSimdMMXDetected)
        return akSimdHaveMMX;

    akSimdMMXDetected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        __cpuid(info, 1);

        // Bit 23 in EDX indicates MMX
        akSimdHaveMMX = (info[3] & (1 << 23)) != 0;

        return akSimdHaveMMX;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;

        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            // Bit 23 in EDX indicates MMX
            akSimdHaveMMX = (edx & (1 << 23)) != 0;

            return akSimdHaveMMX;
        }

        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveSSE()
{
#ifdef Q_PROCESSOR_X86
    static bool akSimdSSEDetected = false;
    static bool akSimdHaveSSE = false;

    if (akSimdSSEDetected)
        return akSimdHaveSSE;

    akSimdSSEDetected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        __cpuid(info, 1);

        // Bit 25 in EDX indicates SSE
        akSimdHaveSSE = (info[3] & (1 << 25)) != 0;

        return akSimdHaveSSE;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;

        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            // Bit 25 in EDX indicates SSE
            akSimdHaveSSE = (edx & (1 << 25)) != 0;

            return akSimdHaveSSE;
        }

        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveSSE2()
{
#ifdef Q_PROCESSOR_X86
    static bool akSimdSSE2Detected = false;
    static bool akSimdHaveSSE2 = false;

    if (akSimdSSE2Detected)
        return akSimdHaveSSE2;

    akSimdSSE2Detected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        __cpuid(info, 1);

        // Bit 26 in EDX indicates SSE2
        akSimdHaveSSE2 = (info[3] & (1 << 26)) != 0;

        return akSimdHaveSSE2;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;

        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            // Bit 26 in EDX indicates SSE2
            akSimdHaveSSE2 = (edx & (1 << 26)) != 0;

            return akSimdHaveSSE2;
        }

        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveSSE4_1()
{
#ifdef Q_PROCESSOR_X86
    static bool akSimdSSE4_1Detected = false;
    static bool akSimdHaveSSE4_1 = false;

    if (akSimdSSE4_1Detected)
        return akSimdHaveSSE4_1;

    akSimdSSE4_1Detected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        __cpuid(info, 1);

        // Bit 19 in ECX indicates SSE4.1
        akSimdHaveSSE4_1 = (info[2] & (1 << 19)) != 0;

        return akSimdHaveSSE4_1;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;

        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            // Bit 19 in ECX indicates SSE4.1
            akSimdHaveSSE4_1 = (ecx & (1 << 19)) != 0;

            return akSimdHaveSSE4_1;
        }

        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveAVX()
{
#if defined(Q_PROCESSOR_X86)
    static bool akSimdAVXDetected = false;
    static bool akSimdHaveAVX = false;

    if (akSimdAVXDetected)
        return akSimdHaveAVX;

    akSimdAVXDetected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        __cpuid(info, 1);

        // Bit 28 in ECX indicates AVX support
        akSimdHaveAVX = (info[2] & (1 << 28)) != 0;

        return akSimdHaveAVX;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;
        __cpuid(1, eax, ebx, ecx, edx);

        // Bit 28 in ECX indicates AVX support
        akSimdHaveAVX = (ecx & (1 << 28)) != 0;

        return akSimdHaveAVX;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveAVX2()
{
#if defined(Q_PROCESSOR_X86)
    static bool akSimdAVX2Detected = false;
    static bool akSimdHaveAVX2 = false;

    if (akSimdAVX2Detected)
        return akSimdHaveAVX2;

    akSimdAVX2Detected = true;

    #ifdef Q_OS_WIN32
        int info[4];
        // Query CPUID with function 7, subfunction 0 for AVX2
        __cpuidex(info, 7, 0);

        // Bit 5 in EBX indicates AVX2 support
        akSimdHaveAVX2 = (info[1] & (1 << 5)) != 0;

        return akSimdHaveAVX2;
    #else
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;

        // Query CPUID with function 7, subfunction 0
        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
            // Bit 5 in EBX indicates AVX2 support
            akSimdHaveAVX2 = (ebx & (1 << 5)) != 0;

            return akSimdHaveAVX2;
        }

        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveNEON()
{
#if Q_PROCESSOR_ARM >= 8
    return true; // NEON is standard on aarch64
#elif defined(Q_PROCESSOR_ARM)
    #if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        static bool akSimdNEONDetected = false;
        static bool akSimdHaveNEON = false;

        if (akSimdNEONDetected)
            return akSimdHaveNEON;

        akSimdNEONDetected = true;

        // Check CPU info for NEON support
        QFile cpuinfo("/proc/cpuinfo");

        if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto content = QString::fromLatin1(cpuinfo.readAll());
            cpuinfo.close();
            akSimdHaveNEON = content.contains("neon", Qt::CaseInsensitive);

            return akSimdHaveNEON;
        }

        return false; // Conservative fallback
    #else
        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveSVE()
{
#ifndef Q_OS_UNIX
    return false;
#elif Q_PROCESSOR_ARM >= 8
    static bool akSimdSVEDetected = false;
    static bool akSimdHaveSVE = false;

    if (akSimdSVEDetected)
        return akSimdHaveSVE;

    akSimdSVEDetected = true;

    #ifdef Q_OS_MACOS
        qint64 value = 0;
        size_t size = sizeof(qint64);

        if (sysctlbyname("hw.optional.sve", &value, &size, nullptr, 0) == 0) {
            akSimdHaveSVE = value != 0;

            return akSimdHaveSVE;
        }

        return false;
    #else
        // Check CPU info for SVE support
        QFile cpuinfo("/proc/cpuinfo");

        if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto content = QString::fromLatin1(cpuinfo.readAll());
            cpuinfo.close();
            akSimdHaveSVE = content.contains("sve", Qt::CaseInsensitive);

            return akSimdHaveSVE;
        }

        return false; // Conservative fallback
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

bool AkSimdPrivate::haveRVV()
{
#ifdef Q_PROCESSOR_RISCV
    #if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        static bool akSimdRVVDetected = false;
        static bool akSimdHaveRVV = false;

        if (akSimdRVVDetected)
            return akSimdHaveRVV;

        akSimdRVVDetected = true;

        // Parse /proc/cpuinfo using QFile to look for the 'v' extension
        QFile cpuinfo("/proc/cpuinfo");

        if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto content = QString::fromLatin1(cpuinfo.readAll());
            cpuinfo.close();
            akSimdHaveRVV =
                    content.contains(QRegularExpression("isa.*v",
                                                        QRegularExpression::CaseInsensitiveOption));

            return akSimdHaveRVV;
        }

        return false; // Conservative fallback
    #else
        // On Windows or macOS, RVV is not commonly supported
        return false;
    #endif
#else
    return false; // Not supported on other architectures
#endif
}

QDebug operator <<(QDebug debug, AkSimd::SimdInstructionSet instructionSet)
{
    debug.nospace() << AkSimd::instructionSetToString(instructionSet).toStdString().c_str();

    return debug.space();
}

#include "moc_aksimd.cpp"
