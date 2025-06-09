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

#include <riscv_vector.h>

#include "simdcorervv.h"

class SimdCoreRVVPrivate
{
    public:
        static void fill3_8(const int *dstWidthOffsetX,
                            const int *dstWidthOffsetY,
                            const int *dstWidthOffsetZ,
                            size_t xoShift,
                            size_t yoShift,
                            size_t zoShift,
                            quint64 maskXo,
                            quint64 maskYo,
                            quint64 maskZo,
                            qint64 xo_,
                            qint64 yo_,
                            qint64 zo_,
                            size_t width,
                            quint8 *line_x,
                            quint8 *line_y,
                            quint8 *line_z,
                            size_t *x);
        static void fill3_16(const int *dstWidthOffsetX,
                             const int *dstWidthOffsetY,
                             const int *dstWidthOffsetZ,
                             size_t xoShift,
                             size_t yoShift,
                             size_t zoShift,
                             quint64 maskXo,
                             quint64 maskYo,
                             quint64 maskZo,
                             qint64 xo_,
                             qint64 yo_,
                             qint64 zo_,
                             size_t width,
                             quint8 *line_x,
                             quint8 *line_y,
                             quint8 *line_z,
                             size_t *x);
        static void fill3_32(const int *dstWidthOffsetX,
                             const int *dstWidthOffsetY,
                             const int *dstWidthOffsetZ,
                             size_t xoShift,
                             size_t yoShift,
                             size_t zoShift,
                             quint64 maskXo,
                             quint64 maskYo,
                             quint64 maskZo,
                             qint64 xo_,
                             qint64 yo_,
                             qint64 zo_,
                             size_t width,
                             quint8 *line_x,
                             quint8 *line_y,
                             quint8 *line_z,
                             size_t *x);
        static void fill3_64(const int *dstWidthOffsetX,
                             const int *dstWidthOffsetY,
                             const int *dstWidthOffsetZ,
                             size_t xoShift,
                             size_t yoShift,
                             size_t zoShift,
                             quint64 maskXo,
                             quint64 maskYo,
                             quint64 maskZo,
                             qint64 xo_,
                             qint64 yo_,
                             qint64 zo_,
                             size_t width,
                             quint8 *line_x,
                             quint8 *line_y,
                             quint8 *line_z,
                             size_t *x);
        static void fill3A_8(const int *dstWidthOffsetX,
                             const int *dstWidthOffsetY,
                             const int *dstWidthOffsetZ,
                             const int *dstWidthOffsetA,
                             size_t xoShift,
                             size_t yoShift,
                             size_t zoShift,
                             size_t aoShift,
                             quint64 maskXo,
                             quint64 maskYo,
                             quint64 maskZo,
                             quint64 maskAo,
                             qint64 xo_,
                             qint64 yo_,
                             qint64 zo_,
                             qint64 ao_,
                             size_t width,
                             quint8 *line_x,
                             quint8 *line_y,
                             quint8 *line_z,
                             quint8 *line_a,
                             size_t *x);
        static void fill3A_16(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetY,
                              const int *dstWidthOffsetZ,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t yoShift,
                              size_t zoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskYo,
                              quint64 maskZo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 yo_,
                              qint64 zo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_y,
                              quint8 *line_z,
                              quint8 *line_a,
                              size_t *x);
        static void fill3A_32(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetY,
                              const int *dstWidthOffsetZ,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t yoShift,
                              size_t zoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskYo,
                              quint64 maskZo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 yo_,
                              qint64 zo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_y,
                              quint8 *line_z,
                              quint8 *line_a,
                              size_t *x);
        static void fill3A_64(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetY,
                              const int *dstWidthOffsetZ,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t yoShift,
                              size_t zoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskYo,
                              quint64 maskZo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 yo_,
                              qint64 zo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_y,
                              quint8 *line_z,
                              quint8 *line_a,
                              size_t *x);
        static void fill1_8(const int *dstWidthOffsetX,
                            size_t xoShift,
                            quint64 maskXo,
                            qint64 xo_,
                            size_t width,
                            quint8 *line_x,
                            size_t *x);
        static void fill1_16(const int *dstWidthOffsetX,
                             size_t xoShift,
                             quint64 maskXo,
                             qint64 xo_,
                             size_t width,
                             quint8 *line_x,
                             size_t *x);
        static void fill1_32(const int *dstWidthOffsetX,
                             size_t xoShift,
                             quint64 maskXo,
                             qint64 xo_,
                             size_t width,
                             quint8 *line_x,
                             size_t *x);
        static void fill1_64(const int *dstWidthOffsetX,
                             size_t xoShift,
                             quint64 maskXo,
                             qint64 xo_,
                             size_t width,
                             quint8 *line_x,
                             size_t *x);
        static void fill1A_8(const int *dstWidthOffsetX,
                             const int *dstWidthOffsetA,
                             size_t xoShift,
                             size_t aoShift,
                             quint64 maskXo,
                             quint64 maskAo,
                             qint64 xo_,
                             qint64 ao_,
                             size_t width,
                             quint8 *line_x,
                             quint8 *line_a,
                             size_t *x);
        static void fill1A_16(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_a,
                              size_t *x);
        static void fill1A_32(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_a,
                              size_t *x);
        static void fill1A_64(const int *dstWidthOffsetX,
                              const int *dstWidthOffsetA,
                              size_t xoShift,
                              size_t aoShift,
                              quint64 maskXo,
                              quint64 maskAo,
                              qint64 xo_,
                              qint64 ao_,
                              size_t width,
                              quint8 *line_x,
                              quint8 *line_a,
                              size_t *x);
};

SimdCoreRVV::SimdCoreRVV(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCoreRVVPrivate;
}

SimdCoreRVV::~SimdCoreRVV()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCoreRVVPrivate::func);

QFunctionPointer SimdCoreRVV::resolve(const char *functionName) const
{
    CHECK_FUNCTION(fill1_8)
    CHECK_FUNCTION(fill1_16)
    CHECK_FUNCTION(fill1_32)
    CHECK_FUNCTION(fill1_64)
    CHECK_FUNCTION(fill1A_8)
    CHECK_FUNCTION(fill1A_16)
    CHECK_FUNCTION(fill1A_32)
    CHECK_FUNCTION(fill1A_64)
    CHECK_FUNCTION(fill3_8)
    CHECK_FUNCTION(fill3_16)
    CHECK_FUNCTION(fill3_32)
    CHECK_FUNCTION(fill3_64)
    CHECK_FUNCTION(fill3A_8)
    CHECK_FUNCTION(fill3A_16)
    CHECK_FUNCTION(fill3A_32)
    CHECK_FUNCTION(fill3A_64)

    return nullptr;
}

void SimdCoreRVVPrivate::fill3_8(const int *dstWidthOffsetX,
                                 const int *dstWidthOffsetY,
                                 const int *dstWidthOffsetZ,
                                 size_t xoShift,
                                 size_t yoShift,
                                 size_t zoShift,
                                 quint64 maskXo,
                                 quint64 maskYo,
                                 quint64 maskZo,
                                 qint64 xo_,
                                 qint64 yo_,
                                 qint64 zo_,
                                 size_t width,
                                 quint8 *line_x,
                                 quint8 *line_y,
                                 quint8 *line_z,
                                 size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e8m8();
    vuint8m8_t xo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(xo_ << xoShift), vlmax);
    vuint8m8_t yo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(yo_ << yoShift), vlmax);
    vuint8m8_t zo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(zo_ << zoShift), vlmax);
    vuint8m8_t mask_xo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskXo), vlmax);
    vuint8m8_t mask_yo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskYo), vlmax);
    vuint8m8_t mask_zo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskZo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        size_t vl = __riscv_vsetvl_e8m8(width - i);
        vuint8m8_t xo = __riscv_vle8_v_u8m8(line_x + xd_x, vl);
        vuint8m8_t yo = __riscv_vle8_v_u8m8(line_y + xd_y, vl);
        vuint8m8_t zo = __riscv_vle8_v_u8m8(line_z + xd_z, vl);

        xo = __riscv_vand_vv_u8m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u8m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u8m8(zo, mask_zo, vl);
        xo = __riscv_vor_vv_u8m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u8m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u8m8(zo, zo_val, vl);

        __riscv_vse8_v_u8m8(line_x + xd_x, xo, vl);
        __riscv_vse8_v_u8m8(line_y + xd_y, yo, vl);
        __riscv_vse8_v_u8m8(line_z + xd_z, zo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3_16(const int *dstWidthOffsetX,
                                  const int *dstWidthOffsetY,
                                  const int *dstWidthOffsetZ,
                                  size_t xoShift,
                                  size_t yoShift,
                                  size_t zoShift,
                                  quint64 maskXo,
                                  quint64 maskYo,
                                  quint64 maskZo,
                                  qint64 xo_,
                                  qint64 yo_,
                                  qint64 zo_,
                                  size_t width,
                                  quint8 *line_x,
                                  quint8 *line_y,
                                  quint8 *line_z,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e16m8();
    vuint16m8_t xo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(xo_ << xoShift), vlmax);
    vuint16m8_t yo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(yo_ << yoShift), vlmax);
    vuint16m8_t zo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(zo_ << zoShift), vlmax);
    vuint16m8_t mask_xo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskXo), vlmax);
    vuint16m8_t mask_yo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskYo), vlmax);
    vuint16m8_t mask_zo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskZo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        size_t vl = __riscv_vsetvl_e16m8(width - i);
        vuint16m8_t xo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_x + xd_x), vl);
        vuint16m8_t yo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_y + xd_y), vl);
        vuint16m8_t zo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_z + xd_z), vl);

        xo = __riscv_vand_vv_u16m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u16m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u16m8(zo, mask_zo, vl);
        xo = __riscv_vor_vv_u16m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u16m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u16m8(zo, zo_val, vl);

        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_x + xd_x), xo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_y + xd_y), yo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_z + xd_z), zo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3_32(const int *dstWidthOffsetX,
                                  const int *dstWidthOffsetY,
                                  const int *dstWidthOffsetZ,
                                  size_t xoShift,
                                  size_t yoShift,
                                  size_t zoShift,
                                  quint64 maskXo,
                                  quint64 maskYo,
                                  quint64 maskZo,
                                  qint64 xo_,
                                  qint64 yo_,
                                  qint64 zo_,
                                  size_t width,
                                  quint8 *line_x,
                                  quint8 *line_y,
                                  quint8 *line_z,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e32m8();
    vuint32m8_t xo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(xo_ << xoShift), vlmax);
    vuint32m8_t yo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(yo_ << yoShift), vlmax);
    vuint32m8_t zo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(zo_ << zoShift), vlmax);
    vuint32m8_t mask_xo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskXo), vlmax);
    vuint32m8_t mask_yo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskYo), vlmax);
    vuint32m8_t mask_zo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskZo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        size_t vl = __riscv_vsetvl_e32m8(width - i);
        vuint32m8_t xo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_x + xd_x), vl);
        vuint32m8_t yo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_y + xd_y), vl);
        vuint32m8_t zo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_z + xd_z), vl);

        xo = __riscv_vand_vv_u32m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u32m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u32m8(zo, mask_zo, vl);
        xo = __riscv_vor_vv_u32m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u32m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u32m8(zo, zo_val, vl);

        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_x + xd_x), xo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_y + xd_y), yo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_z + xd_z), zo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3_64(const int *dstWidthOffsetX,
                                  const int *dstWidthOffsetY,
                                  const int *dstWidthOffsetZ,
                                  size_t xoShift,
                                  size_t yoShift,
                                  size_t zoShift,
                                  quint64 maskXo,
                                  quint64 maskYo,
                                  quint64 maskZo,
                                  qint64 xo_,
                                  qint64 yo_,
                                  qint64 zo_,
                                  size_t width,
                                  quint8 *line_x,
                                  quint8 *line_y,
                                  quint8 *line_z,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e64m8();
    vuint64m8_t xo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(xo_ << xoShift), vlmax);
    vuint64m8_t yo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(yo_ << yoShift), vlmax);
    vuint64m8_t zo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(zo_ << zoShift), vlmax);
    vuint64m8_t mask_xo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskXo), vlmax);
    vuint64m8_t mask_yo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskYo), vlmax);
    vuint64m8_t mask_zo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskZo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        size_t vl = __riscv_vsetvl_e64m8(width - i);
        vuint64m8_t xo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_x + xd_x), vl);
        vuint64m8_t yo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_y + xd_y), vl);
        vuint64m8_t zo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_z + xd_z), vl);

        xo = __riscv_vand_vv_u64m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u64m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u64m8(zo, mask_zo, vl);
        xo = __riscv_vor_vv_u64m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u64m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u64m8(zo, zo_val, vl);

        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_x + xd_x), xo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_y + xd_y), yo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_z + xd_z), zo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3A_8(const int *dstWidthOffsetX,
                                  const int *dstWidthOffsetY,
                                  const int *dstWidthOffsetZ,
                                  const int *dstWidthOffsetA,
                                  size_t xoShift,
                                  size_t yoShift,
                                  size_t zoShift,
                                  size_t aoShift,
                                  quint64 maskXo,
                                  quint64 maskYo,
                                  quint64 maskZo,
                                  quint64 maskAo,
                                  qint64 xo_,
                                  qint64 yo_,
                                  qint64 zo_,
                                  qint64 ao_,
                                  size_t width,
                                  quint8 *line_x,
                                  quint8 *line_y,
                                  quint8 *line_z,
                                  quint8 *line_a,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e8m8();
    vuint8m8_t xo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(xo_ << xoShift), vlmax);
    vuint8m8_t yo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(yo_ << yoShift), vlmax);
    vuint8m8_t zo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(zo_ << zoShift), vlmax);
    vuint8m8_t ao_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(ao_ << aoShift), vlmax);
    vuint8m8_t mask_xo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskXo), vlmax);
    vuint8m8_t mask_yo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskYo), vlmax);
    vuint8m8_t mask_zo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskZo), vlmax);
    vuint8m8_t mask_ao = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e8m8(width - i);
        vuint8m8_t xo = __riscv_vle8_v_u8m8(line_x + xd_x, vl);
        vuint8m8_t yo = __riscv_vle8_v_u8m8(line_y + xd_y, vl);
        vuint8m8_t zo = __riscv_vle8_v_u8m8(line_z + xd_z, vl);
        vuint8m8_t ao = __riscv_vle8_v_u8m8(line_a + xd_a, vl);

        xo = __riscv_vand_vv_u8m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u8m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u8m8(zo, mask_zo, vl);
        ao = __riscv_vand_vv_u8m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u8m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u8m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u8m8(zo, zo_val, vl);
        ao = __riscv_vor_vv_u8m8(ao, ao_val, vl);

        __riscv_vse8_v_u8m8(line_x + xd_x, xo, vl);
        __riscv_vse8_v_u8m8(line_y + xd_y, yo, vl);
        __riscv_vse8_v_u8m8(line_z + xd_z, zo, vl);
        __riscv_vse8_v_u8m8(line_a + xd_a, ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3A_16(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetY,
                                   const int *dstWidthOffsetZ,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift, size_t yoShift,
                                   size_t zoShift, size_t aoShift,
                                   quint64 maskXo, quint64 maskYo,
                                   quint64 maskZo, quint64 maskAo,
                                   qint64 xo_,
                                   qint64 yo_,
                                   qint64 zo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_y,
                                   quint8 *line_z,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e16m8();
    vuint16m8_t xo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(xo_ << xoShift), vlmax);
    vuint16m8_t yo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(yo_ << yoShift), vlmax);
    vuint16m8_t zo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(zo_ << zoShift), vlmax);
    vuint16m8_t ao_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(ao_ << aoShift), vlmax);
    vuint16m8_t mask_xo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskXo), vlmax);
    vuint16m8_t mask_yo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskYo), vlmax);
    vuint16m8_t mask_zo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskZo), vlmax);
    vuint16m8_t mask_ao = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e16m8(width - i);
        vuint16m8_t xo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_x + xd_x), vl);
        vuint16m8_t yo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_y + xd_y), vl);
        vuint16m8_t zo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_z + xd_z), vl);
        vuint16m8_t ao = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u16m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u16m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u16m8(zo, mask_zo, vl);
        ao = __riscv_vand_vv_u16m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u16m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u16m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u16m8(zo, zo_val, vl);
        ao = __riscv_vor_vv_u16m8(ao, ao_val, vl);

        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_x + xd_x), xo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_y + xd_y), yo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_z + xd_z), zo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3A_32(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetY,
                                   const int *dstWidthOffsetZ,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift,
                                   size_t yoShift,
                                   size_t zoShift,
                                   size_t aoShift,
                                   quint64 maskXo,
                                   quint64 maskYo,
                                   quint64 maskZo,
                                   quint64 maskAo,
                                   qint64 xo_,
                                   qint64 yo_,
                                   qint64 zo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_y,
                                   quint8 *line_z,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e32m8();
    vuint32m8_t xo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(xo_ << xoShift), vlmax);
    vuint32m8_t yo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(yo_ << yoShift), vlmax);
    vuint32m8_t zo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(zo_ << zoShift), vlmax);
    vuint32m8_t ao_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(ao_ << aoShift), vlmax);
    vuint32m8_t mask_xo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskXo), vlmax);
    vuint32m8_t mask_yo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskYo), vlmax);
    vuint32m8_t mask_zo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskZo), vlmax);
    vuint32m8_t mask_ao = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e32m8(width - i);
        vuint32m8_t xo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_x + xd_x), vl);
        vuint32m8_t yo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_y + xd_y), vl);
        vuint32m8_t zo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_z + xd_z), vl);
        vuint32m8_t ao = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u32m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u32m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u32m8(zo, mask_zo, vl);
        ao = __riscv_vand_vv_u32m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u32m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u32m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u32m8(zo, zo_val, vl);
        ao = __riscv_vor_vv_u32m8(ao, ao_val, vl);

        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_x + xd_x), xo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_y + xd_y), yo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_z + xd_z), zo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill3A_64(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetY,
                                   const int *dstWidthOffsetZ,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift,
                                   size_t yoShift,
                                   size_t zoShift,
                                   size_t aoShift,
                                   quint64 maskXo,
                                   quint64 maskYo,
                                   quint64 maskZo,
                                   quint64 maskAo,
                                   qint64 xo_,
                                   qint64 yo_,
                                   qint64 zo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_y,
                                   quint8 *line_z,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e64m8();
    vuint64m8_t xo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(xo_ << xoShift), vlmax);
    vuint64m8_t yo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(yo_ << yoShift), vlmax);
    vuint64m8_t zo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(zo_ << zoShift), vlmax);
    vuint64m8_t ao_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(ao_ << aoShift), vlmax);
    vuint64m8_t mask_xo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskXo), vlmax);
    vuint64m8_t mask_yo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskYo), vlmax);
    vuint64m8_t mask_zo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskZo), vlmax);
    vuint64m8_t mask_ao = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e64m8(width - i);
        vuint64m8_t xo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_x + xd_x), vl);
        vuint64m8_t yo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_y + xd_y), vl);
        vuint64m8_t zo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_z + xd_z), vl);
        vuint64m8_t ao = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u64m8(xo, mask_xo, vl);
        yo = __riscv_vand_vv_u64m8(yo, mask_yo, vl);
        zo = __riscv_vand_vv_u64m8(zo, mask_zo, vl);
        ao = __riscv_vand_vv_u64m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u64m8(xo, xo_val, vl);
        yo = __riscv_vor_vv_u64m8(yo, yo_val, vl);
        zo = __riscv_vor_vv_u64m8(zo, zo_val, vl);
        ao = __riscv_vor_vv_u64m8(ao, ao_val, vl);

        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_x + xd_x), xo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_y + xd_y), yo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_z + xd_z), zo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1_8(const int *dstWidthOffsetX,
                                 size_t xoShift,
                                 quint64 maskXo,
                                 qint64 xo_,
                                 size_t width,
                                 quint8 *line_x,
                                 size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e8m8();
    vuint8m8_t xo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(xo_ << xoShift), vlmax);
    vuint8m8_t mask_xo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskXo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];

        size_t vl = __riscv_vsetvl_e8m8(width - i);
        vuint8m8_t xo = __riscv_vle8_v_u8m8(line_x + xd_x, vl);

        xo = __riscv_vand_vv_u8m8(xo, mask_xo, vl);
        xo = __riscv_vor_vv_u8m8(xo, xo_val, vl);

        __riscv_vse8_v_u8m8(line_x + xd_x, xo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1_16(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e16m8();
    vuint16m8_t xo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(xo_ << xoShift), vlmax);
    vuint16m8_t mask_xo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskXo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];

        size_t vl = __riscv_vsetvl_e16m8(width - i);
        vuint16m8_t xo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_x + xd_x), vl);

        xo = __riscv_vand_vv_u16m8(xo, mask_xo, vl);
        xo = __riscv_vor_vv_u16m8(xo, xo_val, vl);

        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_x + xd_x), xo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1_32(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e32m8();
    vuint32m8_t xo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(xo_ << xoShift), vlmax);
    vuint32m8_t mask_xo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskXo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];

        size_t vl = __riscv_vsetvl_e32m8(width - i);
        vuint32m8_t xo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_x + xd_x), vl);

        xo = __riscv_vand_vv_u32m8(xo, mask_xo, vl);
        xo = __riscv_vor_vv_u32m8(xo, xo_val, vl);

        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_x + xd_x), xo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1_64(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e64m8();
    vuint64m8_t xo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(xo_ << xoShift), vlmax);
    vuint64m8_t mask_xo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskXo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];

        size_t vl = __riscv_vsetvl_e64m8(width - i);
        vuint64m8_t xo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_x + xd_x), vl);

        xo = __riscv_vand_vv_u64m8(xo, mask_xo, vl);
        xo = __riscv_vor_vv_u64m8(xo, xo_val, vl);

        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_x + xd_x), xo, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1A_8(const int *dstWidthOffsetX,
                                  const int *dstWidthOffsetA,
                                  size_t xoShift,
                                  size_t aoShift,
                                  quint64 maskXo,
                                  quint64 maskAo,
                                  qint64 xo_,
                                  qint64 ao_,
                                  size_t width,
                                  quint8 *line_x,
                                  quint8 *line_a,
                                  size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e8m8();
    vuint8m8_t xo_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(xo_ << xoShift), vlmax);
    vuint8m8_t ao_val = __riscv_vmv_v_x_u8m8(static_cast<quint8>(ao_ << aoShift), vlmax);
    vuint8m8_t mask_xo = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskXo), vlmax);
    vuint8m8_t mask_ao = __riscv_vmv_v_x_u8m8(static_cast<quint8>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e8m8(width - i);
        vuint8m8_t xo = __riscv_vle8_v_u8m8(line_x + xd_x, vl);
        vuint8m8_t ao = __riscv_vle8_v_u8m8(line_a + xd_a, vl);

        xo = __riscv_vand_vv_u8m8(xo, mask_xo, vl);
        ao = __riscv_vand_vv_u8m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u8m8(xo, xo_val, vl);
        ao = __riscv_vor_vv_u8m8(ao, ao_val, vl);

        __riscv_vse8_v_u8m8(line_x + xd_x, xo, vl);
        __riscv_vse8_v_u8m8(line_a + xd_a, ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1A_16(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift,
                                   size_t aoShift,
                                   quint64 maskXo,
                                   quint64 maskAo,
                                   qint64 xo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e16m8();
    vuint16m8_t xo_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(xo_ << xoShift), vlmax);
    vuint16m8_t ao_val = __riscv_vmv_v_x_u16m8(static_cast<quint16>(ao_ << aoShift), vlmax);
    vuint16m8_t mask_xo = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskXo), vlmax);
    vuint16m8_t mask_ao = __riscv_vmv_v_x_u16m8(static_cast<quint16>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e16m8(width - i);
        vuint16m8_t xo = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_x + xd_x), vl);
        vuint16m8_t ao = __riscv_vle16_v_u16m8(reinterpret_cast<const uint16_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u16m8(xo, mask_xo, vl);
        ao = __riscv_vand_vv_u16m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u16m8(xo, xo_val, vl);
        ao = __riscv_vor_vv_u16m8(ao, ao_val, vl);

        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_x + xd_x), xo, vl);
        __riscv_vse16_v_u16m8(reinterpret_cast<uint16_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1A_32(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift,
                                   size_t aoShift,
                                   quint64 maskXo,
                                   quint64 maskAo,
                                   qint64 xo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e32m8();
    vuint32m8_t xo_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(xo_ << xoShift), vlmax);
    vuint32m8_t ao_val = __riscv_vmv_v_x_u32m8(static_cast<quint32>(ao_ << aoShift), vlmax);
    vuint32m8_t mask_xo = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskXo), vlmax);
    vuint32m8_t mask_ao = __riscv_vmv_v_x_u32m8(static_cast<quint32>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e32m8(width - i);
        vuint32m8_t xo = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_x + xd_x), vl);
        vuint32m8_t ao = __riscv_vle32_v_u32m8(reinterpret_cast<const uint32_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u32m8(xo, mask_xo, vl);
        ao = __riscv_vand_vv_u32m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u32m8(xo, xo_val, vl);
        ao = __riscv_vor_vv_u32m8(ao, ao_val, vl);

        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_x + xd_x), xo, vl);
        __riscv_vse32_v_u32m8(reinterpret_cast<uint32_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

void SimdCoreRVVPrivate::fill1A_64(const int *dstWidthOffsetX,
                                   const int *dstWidthOffsetA,
                                   size_t xoShift,
                                   size_t aoShift,
                                   quint64 maskXo,
                                   quint64 maskAo,
                                   qint64 xo_,
                                   qint64 ao_,
                                   size_t width,
                                   quint8 *line_x,
                                   quint8 *line_a,
                                   size_t *x)
{
    size_t vlmax = __riscv_vsetvlmax_e64m8();
    vuint64m8_t xo_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(xo_ << xoShift), vlmax);
    vuint64m8_t ao_val = __riscv_vmv_v_x_u64m8(static_cast<quint64>(ao_ << aoShift), vlmax);
    vuint64m8_t mask_xo = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskXo), vlmax);
    vuint64m8_t mask_ao = __riscv_vmv_v_x_u64m8(static_cast<quint64>(maskAo), vlmax);

    size_t rvv_width = width - (width % vlmax);

    for (size_t i = *x; i < rvv_width; i += vlmax) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        size_t vl = __riscv_vsetvl_e64m8(width - i);
        vuint64m8_t xo = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_x + xd_x), vl);
        vuint64m8_t ao = __riscv_vle64_v_u64m8(reinterpret_cast<const uint64_t *>(line_a + xd_a), vl);

        xo = __riscv_vand_vv_u64m8(xo, mask_xo, vl);
        ao = __riscv_vand_vv_u64m8(ao, mask_ao, vl);
        xo = __riscv_vor_vv_u64m8(xo, xo_val, vl);
        ao = __riscv_vor_vv_u64m8(ao, ao_val, vl);

        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_x + xd_x), xo, vl);
        __riscv_vse64_v_u64m8(reinterpret_cast<uint64_t *>(line_a + xd_a), ao, vl);
    }

    *x = rvv_width;
}

#include "moc_simdcorervv.cpp"
