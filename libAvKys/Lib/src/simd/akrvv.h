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

#ifndef AKSIMDRVV_H
#define AKSIMDRVV_H

#include <QtGlobal>
#include <riscv_vector.h>

#define AKSIMDRVVI32_DEFAULT_SIZE 8
#define AKSIMDRVVI32_ALIGN        32

class AkSimdRVVI32
{
    public:
        using VectorType = vint32m1_t;
        using NativeType = qint32;

        inline AkSimdRVVI32():
            m_size(__riscv_vsetvl_e32m1(AKSIMDRVVI32_DEFAULT_SIZE))
        {
        }

        inline size_t size() const
        {
            return qMin(this->m_size, AKSIMDRVVI32_DEFAULT_SIZE);
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return __riscv_vle32_v_i32m1(data, this->m_size);
        }

        inline VectorType load(NativeType value) const
        {
            return __riscv_vmv_v_x_i32m1(value, this->m_size);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            __riscv_vse32_v_i32m1(data, vec, this->m_size);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return __riscv_vadd_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return __riscv_vsub_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return __riscv_vmul_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return __riscv_vmul_vx_i32m1(a, b, this->m_size);
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            return __riscv_vdiv_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return __riscv_vdiv_vx_i32m1(a, b, this->m_size);
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto mask = __riscv_vmseq_vx_i32m1_b32(b, 0, this->m_size);

            return __riscv_vdiv_vv_i32m1_m(mask, a, b, this->m_size);
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return __riscv_vmv_v_x_i32m1(0, this->m_size);

            return __riscv_vdiv_vx_i32m1(a, b, this->m_size);
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            auto av = __riscv_vreinterpret_v_i32m1_u32m1(a);
            auto r = __riscv_vsrl_vx_u32m1(av, shift, this->m_size);

            return __riscv_vreinterpret_v_u32m1_i32m1(r);
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return __riscv_vmin_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return __riscv_vmax_vv_i32m1(a, b, this->m_size);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }

    private:
        size_t m_size {0};
};

#endif // AKSIMDRVV_H
