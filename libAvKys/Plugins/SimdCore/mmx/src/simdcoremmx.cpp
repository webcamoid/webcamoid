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

#include <mmintrin.h>

#include "simdcoremmx.h"

class SimdCoreMMXPrivate
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
};

SimdCoreMMX::SimdCoreMMX(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCoreMMXPrivate;
}

SimdCoreMMX::~SimdCoreMMX()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCoreMMXPrivate::func);

QFunctionPointer SimdCoreMMX::resolve(const char *functionName) const
{
    CHECK_FUNCTION(fill1_8)
    CHECK_FUNCTION(fill1_16)
    CHECK_FUNCTION(fill1_32)
    CHECK_FUNCTION(fill1A_8)
    CHECK_FUNCTION(fill1A_16)
    CHECK_FUNCTION(fill1A_32)
    CHECK_FUNCTION(fill3_8)
    CHECK_FUNCTION(fill3_16)
    CHECK_FUNCTION(fill3_32)
    CHECK_FUNCTION(fill3A_8)
    CHECK_FUNCTION(fill3A_16)
    CHECK_FUNCTION(fill3A_32)

    return nullptr;
}

void SimdCoreMMXPrivate::fill3_8(const int *dstWidthOffsetX,
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
    // Prepare MMX registers with values
    __m64 xo_val = _mm_set1_pi8(static_cast<quint8>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi8(static_cast<quint8>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi8(static_cast<quint8>(zo_ << zoShift));
    __m64 mask_xo = _mm_set1_pi8(static_cast<quint8>(maskXo));
    __m64 mask_yo = _mm_set1_pi8(static_cast<quint8>(maskYo));
    __m64 mask_zo = _mm_set1_pi8(static_cast<quint8>(maskZo));

    // Process 8 pixels at a time with MMX
    size_t mmx_width = width - (width % 8); // Process up to multiple of 8

    for (size_t i = *x; i < mmx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        // Load current values
        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);

        // Apply masks and new values
        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);

        // Store results in native endianess
        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
    }

    *x = mmx_width; // Update x for remaining scalar processing
    _mm_empty(); // Clear MMX state
}

void SimdCoreMMXPrivate::fill3_16(const int *dstWidthOffsetX,
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
    // Prepare MMX registers with values
    __m64 xo_val = _mm_set1_pi16(static_cast<quint16>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi16(static_cast<quint16>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi16(static_cast<quint16>(zo_ << zoShift));
    __m64 mask_xo = _mm_set1_pi16(static_cast<quint16>(maskXo));
    __m64 mask_yo = _mm_set1_pi16(static_cast<quint16>(maskYo));
    __m64 mask_zo = _mm_set1_pi16(static_cast<quint16>(maskZo));

    // Process 4 pixels at a time with MMX
    size_t mmx_width = width - (width % 4); // Process up to multiple of 4

    for (size_t i = *x; i < mmx_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        // Load current values
        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);

        // Apply masks and new values
        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);

        // Store results in native endianess
        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
    }

    *x = mmx_width; // Update x for remaining scalar processing
    _mm_empty(); // Clear MMX state
}

void SimdCoreMMXPrivate::fill3_32(const int *dstWidthOffsetX,
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
    // Prepare MMX registers with values
    __m64 xo_val = _mm_set1_pi32(static_cast<quint32>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi32(static_cast<quint32>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi32(static_cast<quint32>(zo_ << zoShift));
    __m64 mask_xo = _mm_set1_pi32(static_cast<quint32>(maskXo));
    __m64 mask_yo = _mm_set1_pi32(static_cast<quint32>(maskYo));
    __m64 mask_zo = _mm_set1_pi32(static_cast<quint32>(maskZo));

    // Process 2 pixels at a time with MMX
    size_t mmx_width = width - (width % 2); // Process up to multiple of 2

    for (size_t i = *x; i < mmx_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        // Load current values
        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);

        // Apply masks and new values
        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);

        // Store results in native endianess
        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
    }

    *x = mmx_width; // Update x for remaining scalar processing
    _mm_empty(); // Clear MMX state
}

void SimdCoreMMXPrivate::fill3A_8(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi8(static_cast<quint8>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi8(static_cast<quint8>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi8(static_cast<quint8>(zo_ << zoShift));
    __m64 ao_val = _mm_set1_pi8(static_cast<quint8>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi8(static_cast<quint8>(maskXo));
    __m64 mask_yo = _mm_set1_pi8(static_cast<quint8>(maskYo));
    __m64 mask_zo = _mm_set1_pi8(static_cast<quint8>(maskZo));
    __m64 mask_ao = _mm_set1_pi8(static_cast<quint8>(maskAo));

    size_t mmx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < mmx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill3A_16(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi16(static_cast<quint16>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi16(static_cast<quint16>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi16(static_cast<quint16>(zo_ << zoShift));
    __m64 ao_val = _mm_set1_pi16(static_cast<quint16>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi16(static_cast<quint16>(maskXo));
    __m64 mask_yo = _mm_set1_pi16(static_cast<quint16>(maskYo));
    __m64 mask_zo = _mm_set1_pi16(static_cast<quint16>(maskZo));
    __m64 mask_ao = _mm_set1_pi16(static_cast<quint16>(maskAo));

    size_t mmx_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < mmx_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill3A_32(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi32(static_cast<quint32>(xo_ << xoShift));
    __m64 yo_val = _mm_set1_pi32(static_cast<quint32>(yo_ << yoShift));
    __m64 zo_val = _mm_set1_pi32(static_cast<quint32>(zo_ << zoShift));
    __m64 ao_val = _mm_set1_pi32(static_cast<quint32>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi32(static_cast<quint32>(maskXo));
    __m64 mask_yo = _mm_set1_pi32(static_cast<quint32>(maskYo));
    __m64 mask_zo = _mm_set1_pi32(static_cast<quint32>(maskZo));
    __m64 mask_ao = _mm_set1_pi32(static_cast<quint32>(maskAo));

    size_t mmx_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < mmx_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 yo = *reinterpret_cast<__m64 *>(line_y + xd_y);
        __m64 zo = *reinterpret_cast<__m64 *>(line_z + xd_z);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        yo = _mm_and_si64(yo, mask_yo);
        zo = _mm_and_si64(zo, mask_zo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        yo = _mm_or_si64(yo, yo_val);
        zo = _mm_or_si64(zo, zo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_y + xd_y) = yo;
        *reinterpret_cast<__m64 *>(line_z + xd_z) = zo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1_8(const int *dstWidthOffsetX,
                                 size_t xoShift,
                                 quint64 maskXo,
                                 qint64 xo_,
                                 size_t width,
                                 quint8 *line_x,
                                 size_t *x)
{
    __m64 xo_val = _mm_set1_pi8(static_cast<quint8>(xo_ << xoShift));
    __m64 mask_xo = _mm_set1_pi8(static_cast<quint8>(maskXo));

    size_t mmx_width = width - (width % 8); // Process 8 pixels
    for (size_t i = *x; i < mmx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);

        xo = _mm_and_si64(xo, mask_xo);
        xo = _mm_or_si64(xo, xo_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1_16(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m64 xo_val = _mm_set1_pi16(static_cast<quint16>(xo_ << xoShift));
    __m64 mask_xo = _mm_set1_pi16(static_cast<quint16>(maskXo));

    size_t mmx_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < mmx_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);

        xo = _mm_and_si64(xo, mask_xo);
        xo = _mm_or_si64(xo, xo_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1_32(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m64 xo_val = _mm_set1_pi32(static_cast<quint32>(xo_ << xoShift));
    __m64 mask_xo = _mm_set1_pi32(static_cast<quint32>(maskXo));

    size_t mmx_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < mmx_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);

        xo = _mm_and_si64(xo, mask_xo);
        xo = _mm_or_si64(xo, xo_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1A_8(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi8(static_cast<quint8>(xo_ << xoShift));
    __m64 ao_val = _mm_set1_pi8(static_cast<quint8>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi8(static_cast<quint8>(maskXo));
    __m64 mask_ao = _mm_set1_pi8(static_cast<quint8>(maskAo));

    size_t mmx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < mmx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1A_16(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi16(static_cast<quint16>(xo_ << xoShift));
    __m64 ao_val = _mm_set1_pi16(static_cast<quint16>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi16(static_cast<quint16>(maskXo));
    __m64 mask_ao = _mm_set1_pi16(static_cast<quint16>(maskAo));

    size_t mmx_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < mmx_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

void SimdCoreMMXPrivate::fill1A_32(const int *dstWidthOffsetX,
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
    __m64 xo_val = _mm_set1_pi32(static_cast<quint32>(xo_ << xoShift));
    __m64 ao_val = _mm_set1_pi32(static_cast<quint32>(ao_ << aoShift));
    __m64 mask_xo = _mm_set1_pi32(static_cast<quint32>(maskXo));
    __m64 mask_ao = _mm_set1_pi32(static_cast<quint32>(maskAo));

    size_t mmx_width = width - (width % 2); // Process 2 pixels

    for (size_t i = *x; i < mmx_width; i += 2) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m64 xo = *reinterpret_cast<__m64 *>(line_x + xd_x);
        __m64 ao = *reinterpret_cast<__m64 *>(line_a + xd_a);

        xo = _mm_and_si64(xo, mask_xo);
        ao = _mm_and_si64(ao, mask_ao);
        xo = _mm_or_si64(xo, xo_val);
        ao = _mm_or_si64(ao, ao_val);

        *reinterpret_cast<__m64 *>(line_x + xd_x) = xo;
        *reinterpret_cast<__m64 *>(line_a + xd_a) = ao;
    }

    *x = mmx_width;
    _mm_empty();
}

#include "moc_simdcoremmx.cpp"
