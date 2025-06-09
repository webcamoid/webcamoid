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

#include <xmmintrin.h>

#include "simdcoresse.h"

class SimdCoreSSEPrivate
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

SimdCoreSSE::SimdCoreSSE(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCoreSSEPrivate;
}

SimdCoreSSE::~SimdCoreSSE()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCoreSSEPrivate::func);

QFunctionPointer SimdCoreSSE::resolve(const char *functionName) const
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

void SimdCoreSSEPrivate::fill3_8(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi8(static_cast<quint8>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi8(static_cast<quint8>(zo_ << zoShift));
    __m128i mask_xo = _mm_set1_epi8(static_cast<quint8>(maskXo));
    __m128i mask_yo = _mm_set1_epi8(static_cast<quint8>(maskYo));
    __m128i mask_zo = _mm_set1_epi8(static_cast<quint8>(maskZo));

    size_t sse_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < sse_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill3_16(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi16(static_cast<quint16>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi16(static_cast<quint16>(zo_ << zoShift));
    __m128i mask_xo = _mm_set1_epi16(static_cast<quint16>(maskXo));
    __m128i mask_yo = _mm_set1_epi16(static_cast<quint16>(maskYo));
    __m128i mask_zo = _mm_set1_epi16(static_cast<quint16>(maskZo));

    size_t sse_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < sse_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill3_32(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi32(static_cast<quint32>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi32(static_cast<quint32>(zo_ << zoShift));
    __m128i mask_xo = _mm_set1_epi32(static_cast<quint32>(maskXo));
    __m128i mask_yo = _mm_set1_epi32(static_cast<quint32>(maskYo));
    __m128i mask_zo = _mm_set1_epi32(static_cast<quint32>(maskZo));

    size_t sse_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < sse_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill3A_8(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi8(static_cast<quint8>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi8(static_cast<quint8>(zo_ << zoShift));
    __m128i ao_val = _mm_set1_epi8(static_cast<quint8>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi8(static_cast<quint8>(maskXo));
    __m128i mask_yo = _mm_set1_epi8(static_cast<quint8>(maskYo));
    __m128i mask_zo = _mm_set1_epi8(static_cast<quint8>(maskZo));
    __m128i mask_ao = _mm_set1_epi8(static_cast<quint8>(maskAo));

    size_t sse_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < sse_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill3A_16(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi16(static_cast<quint16>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi16(static_cast<quint16>(zo_ << zoShift));
    __m128i ao_val = _mm_set1_epi16(static_cast<quint16>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi16(static_cast<quint16>(maskXo));
    __m128i mask_yo = _mm_set1_epi16(static_cast<quint16>(maskYo));
    __m128i mask_zo = _mm_set1_epi16(static_cast<quint16>(maskZo));
    __m128i mask_ao = _mm_set1_epi16(static_cast<quint16>(maskAo));

    size_t sse_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < sse_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill3A_32(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m128i yo_val = _mm_set1_epi32(static_cast<quint32>(yo_ << yoShift));
    __m128i zo_val = _mm_set1_epi32(static_cast<quint32>(zo_ << zoShift));
    __m128i ao_val = _mm_set1_epi32(static_cast<quint32>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi32(static_cast<quint32>(maskXo));
    __m128i mask_yo = _mm_set1_epi32(static_cast<quint32>(maskYo));
    __m128i mask_zo = _mm_set1_epi32(static_cast<quint32>(maskZo));
    __m128i mask_ao = _mm_set1_epi32(static_cast<quint32>(maskAo));

    size_t sse_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < sse_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i yo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_y + xd_y));
        __m128i zo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_z + xd_z));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        yo = _mm_and_si128(yo, mask_yo);
        zo = _mm_and_si128(zo, mask_zo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        yo = _mm_or_si128(yo, yo_val);
        zo = _mm_or_si128(zo, zo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_y + xd_y), yo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_z + xd_z), zo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1_8(const int *dstWidthOffsetX,
                                 size_t xoShift,
                                 quint64 maskXo,
                                 qint64 xo_,
                                 size_t width,
                                 quint8 *line_x,
                                 size_t *x)
{
    __m128i xo_val = _mm_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m128i mask_xo = _mm_set1_epi8(static_cast<quint8>(maskXo));

    size_t sse_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < sse_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));

        xo = _mm_and_si128(xo, mask_xo);
        xo = _mm_or_si128(xo, xo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1_16(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m128i xo_val = _mm_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m128i mask_xo = _mm_set1_epi16(static_cast<quint16>(maskXo));

    size_t sse_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < sse_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));

        xo = _mm_and_si128(xo, mask_xo);
        xo = _mm_or_si128(xo, xo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1_32(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m128i xo_val = _mm_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m128i mask_xo = _mm_set1_epi32(static_cast<quint32>(maskXo));

    size_t sse_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < sse_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));

        xo = _mm_and_si128(xo, mask_xo);
        xo = _mm_or_si128(xo, xo_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1A_8(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m128i ao_val = _mm_set1_epi8(static_cast<quint8>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi8(static_cast<quint8>(maskXo));
    __m128i mask_ao = _mm_set1_epi8(static_cast<quint8>(maskAo));

    size_t sse_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < sse_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1A_16(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m128i ao_val = _mm_set1_epi16(static_cast<quint16>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi16(static_cast<quint16>(maskXo));
    __m128i mask_ao = _mm_set1_epi16(static_cast<quint16>(maskAo));

    size_t sse_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < sse_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

void SimdCoreSSEPrivate::fill1A_32(const int *dstWidthOffsetX,
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
    __m128i xo_val = _mm_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m128i ao_val = _mm_set1_epi32(static_cast<quint32>(ao_ << aoShift));
    __m128i mask_xo = _mm_set1_epi32(static_cast<quint32>(maskXo));
    __m128i mask_ao = _mm_set1_epi32(static_cast<quint32>(maskAo));

    size_t sse_width = width - (width % 4); // Process 4 pixels

    for (size_t i = *x; i < sse_width; i += 4) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m128i xo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_x + xd_x));
        __m128i ao = _mm_loadu_si128(reinterpret_cast<const __m128i *>(line_a + xd_a));

        xo = _mm_and_si128(xo, mask_xo);
        ao = _mm_and_si128(ao, mask_ao);
        xo = _mm_or_si128(xo, xo_val);
        ao = _mm_or_si128(ao, ao_val);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_x + xd_x), xo);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(line_a + xd_a), ao);
    }

    *x = sse_width;
}

#include "moc_simdcoresse.cpp"
