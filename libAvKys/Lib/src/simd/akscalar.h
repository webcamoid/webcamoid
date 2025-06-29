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

#ifndef AKSCALAR_H
#define AKSCALAR_H

#include <QtGlobal>
#include <cstring>

template <typename DataType, int N>
class AkSimdScalar
{
    public:
        struct VectorType
        {
            DataType data[N];
        };

        using NativeType = DataType;

        inline AkSimdScalar()
        {
        }

        inline size_t size() const
        {
            return N;
        }

        inline static void end()
        {
        }

        inline VectorType load(const NativeType *data) const
        {
            VectorType v;
            memcpy(v.data, data, N * sizeof(NativeType));

            return v;
        }

        inline VectorType load(NativeType value) const
        {
            VectorType v;

            for (int i = 0; i < N; ++i)
                v.data[i] = value;

            return v;
        }

        inline void store(NativeType *data, const VectorType &vec) const
        {
            memcpy(data, vec.data, N * sizeof(NativeType));
        }

        inline VectorType add(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] + b.data[i];

            return r;
        }

        inline VectorType sub(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] - b.data[i];

            return r;
        }

        inline VectorType mul(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] * b.data[i];

            return r;
        }

        inline VectorType mul(const VectorType &a, NativeType b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] * b;

            return r;
        }

        inline VectorType div(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] / b.data[i];

            return r;
        }

        inline VectorType div(const VectorType &a, NativeType b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] / b;

            return r;
        }

        inline VectorType sdiv(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = b.data[i]? a.data[i] / b.data[i]: 0;

            return r;
        }

        inline VectorType sdiv(const VectorType &a, NativeType b) const
        {
            VectorType r;

            if (b)
                memset(r.data, 0, N * sizeof(NativeType));
            else
                for (int i = 0; i < N; ++i)
                    r.data[i] = a.data[i] / b;

            return r;
        }

        inline VectorType shr(VectorType a, size_t shift) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = a.data[i] >> shift;

            return r;
        }

        inline VectorType min(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = qMin(a.data[i], b.data[i]);

            return r;
        }

        inline VectorType max(const VectorType &a, const VectorType &b) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = qMax(a.data[i], b.data[i]);

            return r;
        }

        inline VectorType bound(VectorType min, VectorType a, VectorType max) const
        {
            VectorType r;

            for (int i = 0; i < N; ++i)
                r.data[i] = qBound(min.data[i], a.data[i], max.data[i]);

            return r;
        }
};

#define AKSIMDSCALARI32_DEFAULT_SIZE 4
#define AKSIMDSCALARI32_ALIGN        16

using AkSimdScalarI32 = AkSimdScalar<qint32, AKSIMDSCALARI32_DEFAULT_SIZE>;

#endif // AKSCALAR_H
