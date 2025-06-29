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

#ifndef AKSIMD_H
#define AKSIMD_H

#include <QObject>

#include "akcommons.h"

class AkSimd;
class AkSimdPrivate;

using AkSimdPtr = QSharedPointer<AkSimd>;

class AKCOMMONS_EXPORT AkSimd: public QObject
{
    Q_OBJECT
    Q_FLAGS(SimdInstructionSet)

    public:
        enum SimdInstructionSet
        {
            SimdInstructionSet_none   = 0x0,
            SimdInstructionSet_MMX    = 0x1,   // MMX (x86/x86_64)
            SimdInstructionSet_SSE    = 0x2,   // SSE (x86/x86_64)
            SimdInstructionSet_SSE2   = 0x4,   // SSE2 (x86/x86_64)
            SimdInstructionSet_SSE4_1 = 0x8,   // SSE4.1 (x86/x86_64)
            SimdInstructionSet_AVX    = 0x10,  // AVX (x86/x86_64)
            SimdInstructionSet_AVX2   = 0x20,  // AVX2 (x86/x86_64)
            SimdInstructionSet_NEON   = 0x40,  // NEON (ARM)
            SimdInstructionSet_SVE    = 0x80,  // SVE (ARM)
            SimdInstructionSet_RVV    = 0x100, // RVV (RISC-V Vector Extension)
        };
        Q_DECLARE_FLAGS(SimdInstructionSets, SimdInstructionSet)
        Q_FLAG(SimdInstructionSets)
        Q_ENUM(SimdInstructionSet)

        AkSimd(QObject *parent=nullptr);
        AkSimd(const QString &name,
               AkSimd::SimdInstructionSet wanted=SimdInstructionSet_none,
               QObject *parent=nullptr);
        AkSimd(const AkSimd &other);
        ~AkSimd();

        Q_INVOKABLE bool load(const QString &name,
                              SimdInstructionSet wanted=SimdInstructionSet_none);
        Q_INVOKABLE SimdInstructionSet loadedInstructionSet() const;
        Q_INVOKABLE QFunctionPointer resolve(const char *functionName) const;
        Q_INVOKABLE static SimdInstructionSets supportedInstructions();
        Q_INVOKABLE static SimdInstructionSet preferredInstructionSet();
        Q_INVOKABLE static SimdInstructionSet preferredInstructionSet(SimdInstructionSets instructionSets);
        Q_INVOKABLE static int preferredAlign(SimdInstructionSet wanted=SimdInstructionSet_none);
        Q_INVOKABLE static void afree(void *ptr);
        Q_INVOKABLE static void *amalloc(size_t size, int align);
        Q_INVOKABLE static void *amalloc(size_t size);

        template <typename T>
        static T *amallocT(size_t size, int align)
        {
            return reinterpret_cast<T *>(amalloc(size * sizeof(T), align));
        }

        template <typename T>
        static T *amallocT(size_t size)
        {
            return reinterpret_cast<T *>(amalloc(size * sizeof(T)));
        }

        Q_INVOKABLE static QString instructionSetToString(SimdInstructionSet instructionSet);

    private:
        AkSimdPrivate *d;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkSimd::SimdInstructionSet instructionSet);

Q_DECLARE_METATYPE(AkSimd)
Q_DECLARE_METATYPE(AkSimd::SimdInstructionSet)

#endif // AKSIMD_H
