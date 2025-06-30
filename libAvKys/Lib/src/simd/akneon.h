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

#ifndef AKSIMDNEON_H
#define AKSIMDNEON_H

#include <QtGlobal>
#include <arm_neon.h>

#define AKSIMDNEONF32_DEFAULT_SIZE 4
#define AKSIMDNEONF32_ALIGN        16

class AkSimdNEONF32
{
    public:
        using VectorType = float32x4_t;
        using NativeType = float;

        inline AkSimdNEONF32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDNEONF32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return vld1q_f32(data);
        }

        inline VectorType load(NativeType value) const
        {
            return vdupq_n_f32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            vst1q_f32(data, vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return vaddq_f32(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return vsubq_f32(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return vmulq_f32(a, b);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return vmulq_f32(a, vdupq_n_f32(b));
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            // Compute approximate reciprocal of b
            auto bReciprocal = vrecpeq_f32(b); // Initial reciprocal estimate

            // Refine with Newton-Raphson
            bReciprocal = vmulq_f32(bReciprocal, vrecpsq_f32(b, bReciprocal));

            // Perform division: c = a / b =~ a * (1 / b)

            return vmulq_f32(a, bReciprocal);
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return this->div(a, vdupq_n_f32(b));
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            // Compute approximate reciprocal of b
            auto bReciprocal = vrecpeq_f32(b); // Initial reciprocal estimate

            // Refine with Newton-Raphson
            bReciprocal = vmulq_f32(bReciprocal, vrecpsq_f32(b, bReciprocal));

            // Perform division: c = a / b =~ a * (1 / b)
            auto result = vmulq_f32(a, bReciprocal);

            // Create a mask where b == 0 (0xFFFFFFFF for b == 0, 0x00000000 otherwise)
            auto zeroMask = vceqq_s32(vcvtq_s32_f32(b), vdupq_n_s32(0));
            // Invert mask to 0.0f where b == 0, 1.0f otherwise
            auto fMask = vcvtq_f32_u32(vmvnq_u32(zeroMask));

            // Apply mask: set result to 0 where b == 0

            return vmulq_f32(result, fMask);
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (qFuzzyIsNull(b))
                return vdupq_n_f32(0.f);

            return this->div(a, vdupq_n_f32(b));
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return this->div(a, static_cast<NativeType>(1 << shift));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return vbslq_f32(vcltq_f32(a, b), a, b);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return vbslq_f32(vcgtq_f32(a, b), a, b);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
};

#define AKSIMDNEONI32_DEFAULT_SIZE 4
#define AKSIMDNEONI32_ALIGN        16

class AkSimdNEONI32
{
    public:
        using VectorType = int32x4_t;
        using NativeType = qint32;

        inline AkSimdNEONI32()
        {
        }

        inline size_t size() const
        {
            return AKSIMDNEONI32_DEFAULT_SIZE;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            return vld1q_s32(data);
        }

        inline VectorType load(NativeType value) const
        {
            return vdupq_n_s32(value);
        }

        inline void store(NativeType *data, VectorType vec) const
        {
            vst1q_s32(data, vec);
        }

        inline VectorType add(VectorType a, VectorType b) const
        {
            return vaddq_s32(a, b);
        }

        inline VectorType sub(VectorType a, VectorType b) const
        {
            return vsubq_s32(a, b);
        }

        inline VectorType mul(VectorType a, VectorType b) const
        {
            return vmulq_s32(a, b);
        }

        inline VectorType mul(VectorType a, NativeType b) const
        {
            return vmulq_s32(a, vdupq_n_s32(b));
        }

        inline VectorType div(VectorType a, VectorType b) const
        {
            auto af = vcvtq_f32_s32(a);
            auto bf = vcvtq_f32_s32(b);

            // Compute approximate reciprocal of b
            auto bReciprocal = vrecpeq_f32(bf); // Initial reciprocal estimate

            // Refine with Newton-Raphson
            bReciprocal = vmulq_f32(bReciprocal, vrecpsq_f32(bf, bReciprocal));

            // Perform division: c = a / b =~ a * (1 / b)

            return vcvtq_s32_f32(vmulq_f32(af, bReciprocal));
        }

        inline VectorType div(VectorType a, NativeType b) const
        {
            return this->div(a, vdupq_n_s32(b));
        }

        inline VectorType sdiv(VectorType a, VectorType b) const
        {
            auto af = vcvtq_f32_s32(a);
            auto bf = vcvtq_f32_s32(b);

            // Compute approximate reciprocal of b
            auto bReciprocal = vrecpeq_f32(bf); // Initial reciprocal estimate

            // Refine with Newton-Raphson
            bReciprocal = vmulq_f32(bReciprocal, vrecpsq_f32(bf, bReciprocal));

            // Perform division: c = a / b =~ a * (1 / b)
            auto result = vmulq_f32(af, bReciprocal);

            // Create a mask where b == 0 (0xFFFFFFFF for b == 0, 0x00000000 otherwise)
            auto zeroMask = vceqq_s32(b, vdupq_n_s32(0));
            // Invert mask to 0.0f where b == 0, 1.0f otherwise
            auto fMask = vcvtq_f32_u32(vmvnq_u32(zeroMask));

            // Apply mask: set result to 0 where b == 0

            return vcvtq_s32_f32(vmulq_f32(result, fMask));
        }

        inline VectorType sdiv(VectorType a, NativeType b) const
        {
            if (b == 0)
                return vdupq_n_s32(0);

            return this->div(a, vdupq_n_s32(b));
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            return this->div(a, static_cast<NativeType>(1 << shift));
        }

        inline VectorType min(VectorType a, VectorType b) const
        {
            return vminq_s32(a, b);
        }

        inline VectorType max(VectorType a, VectorType b) const
        {
            return vmaxq_s32(a, b);
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            return this->max(min, this->min(a, max));
        }
};

#endif // AKSIMDNEON_H
