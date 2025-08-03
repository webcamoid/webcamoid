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

#ifndef AKSIMDSVE_H
#define AKSIMDSVE_H

#include <QtGlobal>
#include <arm_sve.h>

#define AKSIMDSVEI32_DEFAULT_SIZE 8
#define AKSIMDSVEI32_ALIGN        32

class AkSimdSVEI32
{
    public:
        using VectorType = svint32_t;
        using NativeType = int32_t;

        inline AkSimdSVEI32():
            m_size(qMin<size_t>(svcntw(), AKSIMDSVEI32_DEFAULT_SIZE))
        {
        }

        inline size_t size() const
        {
            return this->m_size;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return svld1_s32(svptrue_b32(), data);
        }

        inline VectorType load(NativeType value) const
        {
            return svdup_n_s32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            svst1_s32(svptrue_b32(), data, vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return svadd_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return svsub_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return svmul_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return svmul_n_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            return svdiv_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return svdiv_n_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto pg = svptrue_b32();
            auto mask = svcmpeq_n_s32(pg, b, 0);

            return svsel_s32(mask, svdup_n_s32(0), svdiv_s32_z(pg, a, b));
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return svdup_n_s32(0);

            return svdiv_n_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return svasr_n_s32_z(svptrue_b32(), a, shift);
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return svmin_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return svmax_s32_z(svptrue_b32(), a, b);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }

    private:
        size_t m_size {0};
};

#endif // AKSIMDSVE_H
