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

#ifndef AKSIMDMMX_H
#define AKSIMDMMX_H

#include <QtGlobal>
#include <mmintrin.h>

#define AKSIMDMMXI32_DEFAULT_SIZE 2
#define AKSIMDMMXI32_ALIGN        8

class AkSimdMMXI32
{
    public:
        using VectorType = __m64;
        using NativeType = qint32;

        inline AkSimdMMXI32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDMMXI32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return *reinterpret_cast<const VectorType *>(data);
        }

        inline VectorType load(NativeType value) const
        {
            return _mm_set1_pi32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            *reinterpret_cast<VectorType *>(data) = vec;
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return _mm_add_pi32(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return _mm_sub_pi32(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));
            auto b0 = _mm_cvtsi64_si32(b);
            auto b1 = _mm_cvtsi64_si32(_mm_srli_si64(b, 32));

            return _mm_set_pi32(a1 * b1, a0 * b0);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));

            return _mm_set_pi32(a1 * b, a0 * b);
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));
            auto b0 = _mm_cvtsi64_si32(b);
            auto b1 = _mm_cvtsi64_si32(_mm_srli_si64(b, 32));

            return _mm_set_pi32(a1 / b1, a0 / b0);
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));

            return _mm_set_pi32(a1 / b, a0 / b);
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));
            auto b0 = _mm_cvtsi64_si32(b);
            auto b1 = _mm_cvtsi64_si32(_mm_srli_si64(b, 32));

            return _mm_set_pi32(b1? a1 / b1: 0, b0? a0 / b0: 0);
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return _mm_setzero_si64();

            auto a0 = _mm_cvtsi64_si32(a);
            auto a1 = _mm_cvtsi64_si32(_mm_srli_si64(a, 32));

            return _mm_set_pi32(a1 / b, a0 / b);
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return _mm_srai_pi32(a, int(shift));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            auto mask = _mm_cmpgt_pi32(a, b);

            return _mm_or_si64(_mm_andnot_si64(mask, a), _mm_and_si64(mask, b));
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            auto mask = _mm_cmpgt_pi32(a, b);

            return _mm_or_si64(_mm_and_si64(mask, a), _mm_andnot_si64(mask, b));
        }

        inline VectorType bound(VectorType a, VectorType min, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
};

#endif // AKSIMDMMX_H
