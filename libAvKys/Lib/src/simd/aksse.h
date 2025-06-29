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

#ifndef AKSIMDSSE_H
#define AKSIMDSSE_H

#include <QtGlobal>
#include <xmmintrin.h>

#define AKSIMDSSEF32_DEFAULT_SIZE 4
#define AKSIMDSSEF32_ALIGN        16

class AkSimdSSEF32
{
    public:
        using VectorType = __m128;
        using NativeType = float;

        inline AkSimdSSEF32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDSSEF32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return _mm_load_ps(data);
        }

        inline VectorType load(NativeType value) const
        {
            return _mm_set1_ps(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            _mm_store_ps(data, vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return _mm_add_ps(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return _mm_sub_ps(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return _mm_mul_ps(a, b);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return _mm_mul_ps(a, _mm_set1_ps(b));
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            return _mm_div_ps(a, b);
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return _mm_div_ps(a, _mm_set1_ps(b));
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            return _mm_and_ps(_mm_div_ps(a, b),
                              _mm_cmpneq_ps(b, _mm_setzero_ps()));
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (qFuzzyIsNull(b))
                return _mm_setzero_ps();

            auto vb = _mm_set1_ps(b);

            return _mm_and_ps(_mm_div_ps(a, vb),
                              _mm_cmpneq_ps(vb, _mm_setzero_ps()));
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return _mm_div_ps(a, _mm_set1_ps(static_cast<NativeType>(1 << shift)));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return _mm_min_ps(a, b);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return _mm_max_ps(a, b);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
};

#endif // AKSIMDSSE_H
