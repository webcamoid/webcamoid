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

#include <arm_neon.h>

#include "simdcoreneon.h"

class SimdCoreNEONPrivate
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

SimdCoreNEON::SimdCoreNEON(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCoreNEONPrivate;
}

SimdCoreNEON::~SimdCoreNEON()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCoreNEONPrivate::func);

QFunctionPointer SimdCoreNEON::resolve(const char *functionName) const
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

void SimdCoreNEONPrivate::fill3_8(const int *dstWidthOffsetX,
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
    uint8x16_t xo_val = vdupq_n_u8(static_cast<quint8>(xo_ << xoShift));
    uint8x16_t yo_val = vdupq_n_u8(static_cast<quint8>(yo_ << yoShift));
    uint8x16_t zo_val = vdupq_n_u8(static_cast<quint8>(zo_ << zoShift));
    uint8x16_t mask_xo = vdupq_n_u8(static_cast<quint8>(maskXo));
    uint8x16_t mask_yo = vdupq_n_u8(static_cast<quint8>(maskYo));
    uint8x16_t mask_zo = vdupq_n_u8(static_cast<quint8>(maskZo));

    size_t neon_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < neon_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        uint8x16_t xo = vld1q_u8(line_x + xd_x);
        uint8x16_t yo = vld1q_u8(line_y + xd_y);
        uint8x16_t zo = vld1q_u8(line_z + xd_z);

        xo = vandq_u8(xo, mask_xo);
        yo = vandq_u8(yo, mask_yo);
        zo = vandq_u8(zo, mask_zo);
        xo = vorrq_u8(xo, xo_val);
        yo = vorrq_u8(yo, yo_val);
        zo = vorrq_u8(zo, zo_val);

        vst1q_u8(line_x + xd_x, xo);
        vst1q_u8(line_y + xd_y, yo);
        vst1q_u8(line_z + xd_z, zo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3_16(const int *dstWidthOffsetX,
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
    uint16x8_t xo_val = vdupq_n_u16(static_cast<quint16>(xo_ << xoShift));
    uint16x8_t yo_val = vdupq_n_u16(static_cast<quint16>(yo_ << yoShift));
    uint16x8_t zo_val = vdupq_n_u16(static_cast<quint16>(zo_ << zoShift));
    uint16x8_t mask_xo = vdupq_n_u16(static_cast<quint16>(maskXo));
    uint16x8_t mask_yo = vdupq_n_u16(static_cast<quint16>(maskYo));
    uint16x8_t mask_zo = vdupq_n_u16(static_cast<quint16>(maskZo));

    size_t neon_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < neon_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        uint16x8_t xo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_x + xd_x));
        uint16x8_t yo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_y + xd_y));
        uint16x8_t zo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_z + xd_z));

        xo = vandq_u16(xo, mask_xo);
        yo = vandq_u16(yo, mask_yo);
        zo = vandq_u16(zo, mask_zo);
        xo = vorrq_u16(xo, xo_val);
        yo = vorrq_u16(yo, yo_val);
        zo = vorrq_u16(zo, zo_val);

        vst1q_u16(reinterpret_cast<uint16_t *>(line_x + xd_x), xo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_y + xd_y), yo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_z + xd_z), zo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3_32(const int *dstWidthOffsetX,
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
    uint32x4_t xo_val = vdupq_n_u32(static_cast<quint32>(xo_ << xoShift));
    uint32x4_t yo_val = vdupq_n_u32(static_cast<quint32>(yo_ << yoShift));
    uint32x4_t zo_val = vdupq_n_u32(static_cast<quint32>(zo_ << zoShift));
    uint32x4_t mask_xo = vdupq_n_u32(static_cast<quint32>(maskXo));
    uint32x4_t mask_yo = vdupq_n_u32(static_cast<quint32>(maskYo));
    uint32x4_t mask_zo = vdupq_n_u32(static_cast<quint32>(maskZo));

    size_t neon_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < neon_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        uint32x4_t xo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_x + xd_x));
        uint32x4_t yo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_y + xd_y));
        uint32x4_t zo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_z + xd_z));

        xo = vandq_u32(xo, mask_xo);
        yo = vandq_u32(yo, mask_yo);
        zo = vandq_u32(zo, mask_zo);
        xo = vorrq_u32(xo, xo_val);
        yo = vorrq_u32(yo, yo_val);
        zo = vorrq_u32(zo, zo_val);

        vst1q_u32(reinterpret_cast<uint32_t *>(line_x + xd_x), xo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_y + xd_y), yo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_z + xd_z), zo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3_64(const int *dstWidthOffsetX,
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
    uint64x2_t xo_val = vdupq_n_u64(static_cast<quint64>(xo_ << xoShift));
    uint64x2_t yo_val = vdupq_n_u64(static_cast<quint64>(yo_ << yoShift));
    uint64x2_t zo_val = vdupq_n_u64(static_cast<quint64>(zo_ << zoShift));
    uint64x2_t mask_xo = vdupq_n_u64(static_cast<quint64>(maskXo));
    uint64x2_t mask_yo = vdupq_n_u64(static_cast<quint64>(maskYo));
    uint64x2_t mask_zo = vdupq_n_u64(static_cast<quint64>(maskZo));

    size_t neon_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < neon_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        uint64x2_t xo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_x + xd_x));
        uint64x2_t yo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_y + xd_y));
        uint64x2_t zo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_z + xd_z));

        xo = vandq_u64(xo, mask_xo);
        yo = vandq_u64(yo, mask_yo);
        zo = vandq_u64(zo, mask_zo);
        xo = vorrq_u64(xo, xo_val);
        yo = vorrq_u64(yo, yo_val);
        zo = vorrq_u64(zo, zo_val);

        vst1q_u64(reinterpret_cast<uint64_t *>(line_x + xd_x), xo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_y + xd_y), yo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_z + xd_z), zo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3A_8(const int *dstWidthOffsetX,
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
    uint8x16_t xo_val = vdupq_n_u8(static_cast<quint8>(xo_ << xoShift));
    uint8x16_t yo_val = vdupq_n_u8(static_cast<quint8>(yo_ << yoShift));
    uint8x16_t zo_val = vdupq_n_u8(static_cast<quint8>(zo_ << zoShift));
    uint8x16_t ao_val = vdupq_n_u8(static_cast<quint8>(ao_ << aoShift));
    uint8x16_t mask_xo = vdupq_n_u8(static_cast<quint8>(maskXo));
    uint8x16_t mask_yo = vdupq_n_u8(static_cast<quint8>(maskYo));
    uint8x16_t mask_zo = vdupq_n_u8(static_cast<quint8>(maskZo));
    uint8x16_t mask_ao = vdupq_n_u8(static_cast<quint8>(maskAo));

    size_t neon_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < neon_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        uint8x16_t xo = vld1q_u8(line_x + xd_x);
        uint8x16_t yo = vld1q_u8(line_y + xd_y);
        uint8x16_t zo = vld1q_u8(line_z + xd_z);
        uint8x16_t ao = vld1q_u8(line_a + xd_a);

        xo = vandq_u8(xo, mask_xo);
        yo = vandq_u8(yo, mask_yo);
        zo = vandq_u8(zo, mask_zo);
        ao = vandq_u8(ao, mask_ao);
        xo = vorrq_u8(xo, xo_val);
        yo = vorrq_u8(yo, yo_val);
        zo = vorrq_u8(zo, zo_val);
        ao = vorrq_u8(ao, ao_val);

        vst1q_u8(line_x + xd_x, xo);
        vst1q_u8(line_y + xd_y, yo);
        vst1q_u8(line_z + xd_z, zo);
        vst1q_u8(line_a + xd_a, ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3A_16(const int *dstWidthOffsetX,
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
    uint16x8_t xo_val = vdupq_n_u16(static_cast<quint16>(xo_ << xoShift));
    uint16x8_t yo_val = vdupq_n_u16(static_cast<quint16>(yo_ << yoShift));
    uint16x8_t zo_val = vdupq_n_u16(static_cast<quint16>(zo_ << zoShift));
    uint16x8_t ao_val = vdupq_n_u16(static_cast<quint16>(ao_ << aoShift));
    uint16x8_t mask_xo = vdupq_n_u16(static_cast<quint16>(maskXo));
    uint16x8_t mask_yo = vdupq_n_u16(static_cast<quint16>(maskYo));
    uint16x8_t mask_zo = vdupq_n_u16(static_cast<quint16>(maskZo));
    uint16x8_t mask_ao = vdupq_n_u16(static_cast<quint16>(maskAo));

    size_t neon_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < neon_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        uint16x8_t xo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_x + xd_x));
        uint16x8_t yo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_y + xd_y));
        uint16x8_t zo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_z + xd_z));
        uint16x8_t ao = vld1q_u16(reinterpret_cast<const uint16_t *>(line_a + xd_a));

        xo = vandq_u16(xo, mask_xo);
        yo = vandq_u16(yo, mask_yo);
        zo = vandq_u16(zo, mask_zo);
        ao = vandq_u16(ao, mask_ao);
        xo = vorrq_u16(xo, xo_val);
        yo = vorrq_u16(yo, yo_val);
        zo = vorrq_u16(zo, zo_val);
        ao = vorrq_u16(ao, ao_val);

        vst1q_u16(reinterpret_cast<uint16_t *>(line_x + xd_x), xo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_y + xd_y), yo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_z + xd_z), zo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3A_32(const int *dstWidthOffsetX,
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
    uint32x4_t xo_val = vdupq_n_u32(static_cast<quint32>(xo_ << xoShift));
    uint32x4_t yo_val = vdupq_n_u32(static_cast<quint32>(yo_ << yoShift));
    uint32x4_t zo_val = vdupq_n_u32(static_cast<quint32>(zo_ << zoShift));
    uint32x4_t ao_val = vdupq_n_u32(static_cast<quint32>(ao_ << aoShift));
    uint32x4_t mask_xo = vdupq_n_u32(static_cast<quint32>(maskXo));
    uint32x4_t mask_yo = vdupq_n_u32(static_cast<quint32>(maskYo));
    uint32x4_t mask_zo = vdupq_n_u32(static_cast<quint32>(maskZo));
    uint32x4_t mask_ao = vdupq_n_u32(static_cast<quint32>(maskAo));

    size_t neon_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < neon_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        uint32x4_t xo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_x + xd_x));
        uint32x4_t yo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_y + xd_y));
        uint32x4_t zo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_z + xd_z));
        uint32x4_t ao = vld1q_u32(reinterpret_cast<const uint32_t *>(line_a + xd_a));

        xo = vandq_u32(xo, mask_xo);
        yo = vandq_u32(yo, mask_yo);
        zo = vandq_u32(zo, mask_zo);
        ao = vandq_u32(ao, mask_ao);
        xo = vorrq_u32(xo, xo_val);
        yo = vorrq_u32(yo, yo_val);
        zo = vorrq_u32(zo, zo_val);
        ao = vorrq_u32(ao, ao_val);

        vst1q_u32(reinterpret_cast<uint32_t *>(line_x + xd_x), xo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_y + xd_y), yo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_z + xd_z), zo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill3A_64(const int *dstWidthOffsetX,
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
    uint64x2_t xo_val = vdupq_n_u64(static_cast<quint64>(xo_ << xoShift));
    uint64x2_t yo_val = vdupq_n_u64(static_cast<quint64>(yo_ << yoShift));
    uint64x2_t zo_val = vdupq_n_u64(static_cast<quint64>(zo_ << zoShift));
    uint64x2_t ao_val = vdupq_n_u64(static_cast<quint64>(ao_ << aoShift));
    uint64x2_t mask_xo = vdupq_n_u64(static_cast<quint64>(maskXo));
    uint64x2_t mask_yo = vdupq_n_u64(static_cast<quint64>(maskYo));
    uint64x2_t mask_zo = vdupq_n_u64(static_cast<quint64>(maskZo));
    uint64x2_t mask_ao = vdupq_n_u64(static_cast<quint64>(maskAo));

    size_t neon_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < neon_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        uint64x2_t xo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_x + xd_x));
        uint64x2_t yo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_y + xd_y));
        uint64x2_t zo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_z + xd_z));
        uint64x2_t ao = vld1q_u64(reinterpret_cast<const uint64_t *>(line_a + xd_a));

        xo = vandq_u64(xo, mask_xo);
        yo = vandq_u64(yo, mask_yo);
        zo = vandq_u64(zo, mask_zo);
        ao = vandq_u64(ao, mask_ao);
        xo = vorrq_u64(xo, xo_val);
        yo = vorrq_u64(yo, yo_val);
        zo = vorrq_u64(zo, zo_val);
        ao = vorrq_u64(ao, ao_val);

        vst1q_u64(reinterpret_cast<uint64_t *>(line_x + xd_x), xo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_y + xd_y), yo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_z + xd_z), zo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1_8(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    uint8x16_t xo_val = vdupq_n_u8(static_cast<quint8>(xo_ << xoShift));
    uint8x16_t mask_xo = vdupq_n_u8(static_cast<quint8>(maskXo));

    size_t neon_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < neon_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];

        uint8x16_t xo = vld1q_u8(line_x + xd_x);

        xo = vandq_u8(xo, mask_xo);
        xo = vorrq_u8(xo, xo_val);

        vst1q_u8(line_x + xd_x, xo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1_16(const int *dstWidthOffsetX,
                                   size_t xoShift,
                                   quint64 maskXo,
                                   qint64 xo_,
                                   size_t width,
                                   quint8 *line_x,
                                   size_t *x)
{
    uint16x8_t xo_val = vdupq_n_u16(static_cast<quint16>(xo_ << xoShift));
    uint16x8_t mask_xo = vdupq_n_u16(static_cast<quint16>(maskXo));

    size_t neon_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < neon_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];

        uint16x8_t xo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_x + xd_x));

        xo = vandq_u16(xo, mask_xo);
        xo = vorrq_u16(xo, xo_val);

        vst1q_u16(reinterpret_cast<uint16_t *>(line_x + xd_x), xo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1_32(const int *dstWidthOffsetX,
                                   size_t xoShift,
                                   quint64 maskXo,
                                   qint64 xo_,
                                   size_t width,
                                   quint8 *line_x,
                                   size_t *x)
{
    uint32x4_t xo_val = vdupq_n_u32(static_cast<quint32>(xo_ << xoShift));
    uint32x4_t mask_xo = vdupq_n_u32(static_cast<quint32>(maskXo));

    size_t neon_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < neon_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];

        uint32x4_t xo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_x + xd_x));

        xo = vandq_u32(xo, mask_xo);
        xo = vorrq_u32(xo, xo_val);

        vst1q_u32(reinterpret_cast<uint32_t *>(line_x + xd_x), xo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1_64(const int *dstWidthOffsetX,
                                   size_t xoShift,
                                   quint64 maskXo,
                                   qint64 xo_,
                                   size_t width,
                                   quint8 *line_x,
                                   size_t *x)
{
    uint64x2_t xo_val = vdupq_n_u64(static_cast<quint64>(xo_ << xoShift));
    uint64x2_t mask_xo = vdupq_n_u64(static_cast<quint64>(maskXo));

    size_t neon_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < neon_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];

        uint64x2_t xo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_x + xd_x));

        xo = vandq_u64(xo, mask_xo);
        xo = vorrq_u64(xo, xo_val);

        vst1q_u64(reinterpret_cast<uint64_t *>(line_x + xd_x), xo);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1A_8(const int *dstWidthOffsetX,
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
    uint8x16_t xo_val = vdupq_n_u8(static_cast<quint8>(xo_ << xoShift));
    uint8x16_t ao_val = vdupq_n_u8(static_cast<quint8>(ao_ << aoShift));
    uint8x16_t mask_xo = vdupq_n_u8(static_cast<quint8>(maskXo));
    uint8x16_t mask_ao = vdupq_n_u8(static_cast<quint8>(maskAo));

    size_t neon_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < neon_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        uint8x16_t xo = vld1q_u8(line_x + xd_x);
        uint8x16_t ao = vld1q_u8(line_a + xd_a);

        xo = vandq_u8(xo, mask_xo);
        ao = vandq_u8(ao, mask_ao);
        xo = vorrq_u8(xo, xo_val);
        ao = vorrq_u8(ao, ao_val);

        vst1q_u8(line_x + xd_x, xo);
        vst1q_u8(line_a + xd_a, ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1A_16(const int *dstWidthOffsetX,
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
    uint16x8_t xo_val = vdupq_n_u16(static_cast<quint16>(xo_ << xoShift));
    uint16x8_t ao_val = vdupq_n_u16(static_cast<quint16>(ao_ << aoShift));
    uint16x8_t mask_xo = vdupq_n_u16(static_cast<quint16>(maskXo));
    uint16x8_t mask_ao = vdupq_n_u16(static_cast<quint16>(maskAo));

    size_t neon_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < neon_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        uint16x8_t xo = vld1q_u16(reinterpret_cast<const uint16_t *>(line_x + xd_x));
        uint16x8_t ao = vld1q_u16(reinterpret_cast<const uint16_t *>(line_a + xd_a));

        xo = vandq_u16(xo, mask_xo);
        ao = vandq_u16(ao, mask_ao);
        xo = vorrq_u16(xo, xo_val);
        ao = vorrq_u16(ao, ao_val);

        vst1q_u16(reinterpret_cast<uint16_t *>(line_x + xd_x), xo);
        vst1q_u16(reinterpret_cast<uint16_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1A_32(const int *dstWidthOffsetX,
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
    uint32x4_t xo_val = vdupq_n_u32(static_cast<quint32>(xo_ << xoShift));
    uint32x4_t ao_val = vdupq_n_u32(static_cast<quint32>(ao_ << aoShift));
    uint32x4_t mask_xo = vdupq_n_u32(static_cast<quint32>(maskXo));
    uint32x4_t mask_ao = vdupq_n_u32(static_cast<quint32>(maskAo));

    size_t neon_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < neon_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        uint32x4_t xo = vld1q_u32(reinterpret_cast<const uint32_t *>(line_x + xd_x));
        uint32x4_t ao = vld1q_u32(reinterpret_cast<const uint32_t *>(line_a + xd_a));

        xo = vandq_u32(xo, mask_xo);
        ao = vandq_u32(ao, mask_ao);
        xo = vorrq_u32(xo, xo_val);
        ao = vorrq_u32(ao, ao_val);

        vst1q_u32(reinterpret_cast<uint32_t *>(line_x + xd_x), xo);
        vst1q_u32(reinterpret_cast<uint32_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

void SimdCoreNEONPrivate::fill1A_64(const int *dstWidthOffsetX,
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
    uint64x2_t xo_val = vdupq_n_u64(static_cast<quint64>(xo_ << xoShift));
    uint64x2_t ao_val = vdupq_n_u64(static_cast<quint64>(ao_ << aoShift));
    uint64x2_t mask_xo = vdupq_n_u64(static_cast<quint64>(maskXo));
    uint64x2_t mask_ao = vdupq_n_u64(static_cast<quint64>(maskAo));

    size_t neon_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < neon_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        uint64x2_t xo = vld1q_u64(reinterpret_cast<const uint64_t *>(line_x + xd_x));
        uint64x2_t ao = vld1q_u64(reinterpret_cast<const uint64_t *>(line_a + xd_a));

        xo = vandq_u64(xo, mask_xo);
        ao = vandq_u64(ao, mask_ao);
        xo = vorrq_u64(xo, xo_val);
        ao = vorrq_u64(ao, ao_val);

        vst1q_u64(reinterpret_cast<uint64_t *>(line_x + xd_x), xo);
        vst1q_u64(reinterpret_cast<uint64_t *>(line_a + xd_a), ao);
    }

    *x = neon_width;
}

#include "moc_simdcoreneon.cpp"
