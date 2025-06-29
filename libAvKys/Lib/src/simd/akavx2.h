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

#ifndef AKSIMDAVX2_H
#define AKSIMDAVX2_H

#include <QtGlobal>
#include <immintrin.h>

#define AKSIMDAVX2I32_DEFAULT_SIZE 8
#define AKSIMDAVX2I32_ALIGN        32

class AkSimdAVX2I32
{
    public:
        using VectorType = __m256i;
        using NativeType = qint32;

        inline AkSimdAVX2I32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDAVX2I32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return _mm256_load_si256(reinterpret_cast<const VectorType *>(data));
        }

        inline VectorType load(NativeType value) const
        {
            return _mm256_set1_epi32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            _mm256_store_si256(reinterpret_cast<__m256i *>(data), vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return _mm256_add_epi32(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return _mm256_sub_epi32(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return _mm256_mullo_epi32(a, b);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return _mm256_mullo_epi32(a, _mm256_set1_epi32(b));
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            return _mm256_cvtps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(a),
                                                    _mm256_cvtepi32_ps(b)));
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return _mm256_cvtps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(a),
                                                    _mm256_set1_ps(static_cast<float>(b))));
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto aps = _mm256_cvtepi32_ps(a);
            auto bps = _mm256_cvtepi32_ps(b);

            auto result =
                    _mm256_and_ps(_mm256_div_ps(aps, bps),
                                  _mm256_cmp_ps(bps,
                                                _mm256_setzero_ps(),
                                                _CMP_NEQ_OQ));

            return _mm256_cvtps_epi32(result);
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return _mm256_setzero_si256();

            auto aps = _mm256_cvtepi32_ps(a);
            auto bps = _mm256_set1_ps(static_cast<float>(b));

            return _mm256_cvtps_epi32(_mm256_div_ps(aps, bps));
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return _mm256_srai_epi32(a, static_cast<int>(shift));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return _mm256_min_epi32(a, b);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return _mm256_max_epi32(a, b);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
 };

#endif // AKSIMDAVX2_H
