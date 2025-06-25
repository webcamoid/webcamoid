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

#ifndef AKSIMDSSE2_H
#define AKSIMDSSE2_H

#include <QtGlobal>
#include <emmintrin.h>

#define AKSIMDSSE2I32_DEFAULT_SIZE 4
#define AKSIMDSSE2I32_ALIGN        16

class AkSimdSSE2I32
{
    public:
        using VectorType = __m128i;
        using NativeType = qint32;

        inline AkSimdSSE2I32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDSSE2I32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return _mm_load_si128(reinterpret_cast<const VectorType *>(data));
        }

        inline VectorType load(NativeType value) const
        {
            return _mm_set1_epi32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            _mm_store_si128(reinterpret_cast<VectorType *>(data), vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return _mm_add_epi32(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return _mm_sub_epi32(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(a),
                                              _mm_cvtepi32_ps(b)));
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
                return _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(a),
                                                  _mm_set1_ps(static_cast<float>(b))));
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            return _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(a),
                                              _mm_cvtepi32_ps(b)));
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(a),
                                              _mm_set1_ps(static_cast<float>(b))));
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto aps = _mm_cvtepi32_ps(a);
            auto bps = _mm_cvtepi32_ps(b);

            return _mm_cvtps_epi32(_mm_and_ps(_mm_div_ps(aps, bps),
                                              _mm_cmpneq_ps(bps, _mm_setzero_ps())));
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return _mm_setzero_si128();

            auto aps = _mm_cvtepi32_ps(a);
            auto bps = _mm_set1_ps(static_cast<float>(b));

            return _mm_cvtps_epi32(_mm_div_ps(aps, bps));
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return _mm_srai_epi32(a, static_cast<int>(shift));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return _mm_cvtps_epi32(_mm_min_ps(_mm_cvtepi32_ps(a),
                                              _mm_cvtepi32_ps(b)));
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return _mm_cvtps_epi32(_mm_max_ps(_mm_cvtepi32_ps(a),
                                              _mm_cvtepi32_ps(b)));
        }

        inline VectorType bound(VectorType a, VectorType min, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
};

#endif // AKSIMDSSE2_H
