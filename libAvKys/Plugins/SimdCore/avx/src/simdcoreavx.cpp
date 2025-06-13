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

#include <immintrin.h>

#include "simdcoreavx.h"

class SimdCoreAVXPrivate
{
    public:
        // Optimized draw functions

        static void drawFast8bits3APack(int oWidth,
                                        int *srcWidthOffset,
                                        int *dstWidthOffset,
                                        size_t xiShift,
                                        size_t yiShift,
                                        size_t ziShift,
                                        size_t aiShift,
                                        size_t alphaShift,
                                        const quint8 *src_line,
                                        quint8 *dst_line,
                                        qint64 *aiMultTable,
                                        qint64 *aoMultTable,
                                        qint64 *alphaDivTable,
                                        int *x);
        static void drawFast8bits1APack(int oWidth,
                                        int *srcWidthOffset,
                                        int *dstWidthOffset,
                                        size_t xiShift,
                                        size_t aiShift,
                                        size_t alphaShift,
                                        const quint8 *src_line,
                                        quint8 *dst_line,
                                        qint64 *aiMultTable,
                                        qint64 *aoMultTable,
                                        qint64 *alphaDivTable,
                                        int *x);
        static void drawFastLc8bits3APack(int oWidth,
                                          int iDiffX,
                                          int oDiffX,
                                          int oMultX,
                                          size_t xiWidthDiv,
                                          size_t xiStep,
                                          size_t xiShift,
                                          size_t yiShift,
                                          size_t ziShift,
                                          size_t aiShift,
                                          size_t alphaShift,
                                          const quint8 *src_line,
                                          quint8 *dst_line,
                                          qint64 *aiMultTable,
                                          qint64 *aoMultTable,
                                          qint64 *alphaDivTable,
                                          int *x);
        static void drawFastLc8bits1APack(int oWidth,
                                          int iDiffX,
                                          int oDiffX,
                                          int oMultX,
                                          size_t xiWidthDiv,
                                          size_t xiStep,
                                          size_t xiShift,
                                          size_t aiShift,
                                          size_t alphaShift,
                                          const quint8 *src_line,
                                          quint8 *dst_line,
                                          qint64 *aiMultTable,
                                          qint64 *aoMultTable,
                                          qint64 *alphaDivTable,
                                          int *x);

        // Optimized fill functions

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

SimdCoreAVX::SimdCoreAVX(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCoreAVXPrivate;
}

SimdCoreAVX::~SimdCoreAVX()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCoreAVXPrivate::func);

QFunctionPointer SimdCoreAVX::resolve(const char *functionName) const
{
    CHECK_FUNCTION(drawFast8bits1APack)
    CHECK_FUNCTION(drawFast8bits3APack)
    CHECK_FUNCTION(drawFastLc8bits1APack)
    CHECK_FUNCTION(drawFastLc8bits3APack)
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

void SimdCoreAVXPrivate::drawFast8bits3APack(int oWidth,
                                             int *srcWidthOffset,
                                             int *dstWidthOffset,
                                             size_t xiShift,
                                             size_t yiShift,
                                             size_t ziShift,
                                             size_t aiShift,
                                             size_t alphaShift,
                                             const quint8 *src_line,
                                             quint8 *dst_line,
                                             qint64 *aiMultTable,
                                             qint64 *aoMultTable,
                                             qint64 *alphaDivTable,
                                             int *x)
{
    int maxX = oWidth - (oWidth % 8);
    __m256i maskFF = _mm256_set1_epi8(0xff);

    for (; *x < maxX; *x += 8) {
        __m256i src_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_line + srcWidthOffset[*x]));
        __m256i dst_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_line + dstWidthOffset[*x]));

        __m256i xi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, xiShift), maskFF);
        __m256i yi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, yiShift), maskFF);
        __m256i zi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, ziShift), maskFF);
        __m256i ai = _mm256_and_si256(_mm256_srli_epi32(src_pixels, aiShift), maskFF);

        __m256i xo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, xiShift), maskFF);
        __m256i yo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, yiShift), maskFF);
        __m256i zo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, ziShift), maskFF);
        __m256i ao = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, aiShift), maskFF);

        quint32 xi_arr[8], yi_arr[8], zi_arr[8], ai_arr[8];
        quint32 xo_arr[8], yo_arr[8], zo_arr[8], ao_arr[8];
        _mm256_store_si256(reinterpret_cast<__m256i *>(xi_arr), xi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(yi_arr), yi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(zi_arr), zi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ai_arr), ai);
        _mm256_store_si256(reinterpret_cast<__m256i *>(xo_arr), xo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(yo_arr), yo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(zo_arr), zo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ao_arr), ao);

        quint32 result[8];

        for (int j = 0; j < 8; ++j) {
            size_t alphaMask = (size_t(ai_arr[j]) << 8) | size_t(ao_arr[j]);
            qint64 xt = (qint64(xi_arr[j]) * aiMultTable[alphaMask] + qint64(xo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 yt = (qint64(yi_arr[j]) * aiMultTable[alphaMask] + qint64(yo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 zt = (qint64(zi_arr[j]) * aiMultTable[alphaMask] + qint64(zo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 at = alphaDivTable[alphaMask];

            result[j] = (quint32(xt) << xiShift)
                      | (quint32(yt) << yiShift)
                      | (quint32(zt) << ziShift)
                      | (quint32(at) << aiShift);
        }

        _mm256_store_si256(reinterpret_cast<__m256i *>(dst_line + dstWidthOffset[*x]),
                           _mm256_load_si256(reinterpret_cast<__m256i *>(result)));
    }
}

void SimdCoreAVXPrivate::drawFast8bits1APack(int oWidth,
                                             int *srcWidthOffset,
                                             int *dstWidthOffset,
                                             size_t xiShift,
                                             size_t aiShift,
                                             size_t alphaShift,
                                             const quint8 *src_line,
                                             quint8 *dst_line,
                                             qint64 *aiMultTable,
                                             qint64 *aoMultTable,
                                             qint64 *alphaDivTable,
                                             int *x)
{
    int maxX = oWidth - (oWidth % 16);
    __m256i maskFF = _mm256_set1_epi8(0xff);

    for (; *x < maxX; *x += 16) {
        __m256i src_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_line + srcWidthOffset[*x]));
        __m256i dst_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_line + dstWidthOffset[*x]));

        __m256i xi = _mm256_and_si256(_mm256_srli_epi16(src_pixels, xiShift), maskFF);
        __m256i ai = _mm256_and_si256(_mm256_srli_epi16(src_pixels, aiShift), maskFF);
        __m256i xo = _mm256_and_si256(_mm256_srli_epi16(dst_pixels, xiShift), maskFF);
        __m256i ao = _mm256_and_si256(_mm256_srli_epi16(dst_pixels, aiShift), maskFF);

        quint16 xi_arr[16], ai_arr[16], xo_arr[16], ao_arr[16];
        _mm256_store_si256(reinterpret_cast<__m256i *>(xi_arr), xi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ai_arr), ai);
        _mm256_store_si256(reinterpret_cast<__m256i *>(xo_arr), xo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ao_arr), ao);

        quint16 result[16];

        for (int j = 0; j < 16; ++j) {
            size_t alphaMask = (size_t(ai_arr[j]) << 8) | size_t(ao_arr[j]);
            qint64 xt = (qint64(xi_arr[j]) * aiMultTable[alphaMask] + qint64(xo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 at = alphaDivTable[alphaMask];

            result[j] = (quint16(xt) << xiShift) | (quint16(at) << aiShift);
        }

        _mm256_store_si256(reinterpret_cast<__m256i *>(dst_line + dstWidthOffset[*x]),
                           _mm256_load_si256(reinterpret_cast<__m256i *>(result)));
    }
}

void SimdCoreAVXPrivate::drawFastLc8bits3APack(int oWidth,
                                               int iDiffX,
                                               int oDiffX,
                                               int oMultX,
                                               size_t xiWidthDiv,
                                               size_t xiStep,
                                               size_t xiShift,
                                               size_t yiShift,
                                               size_t ziShift,
                                               size_t aiShift,
                                               size_t alphaShift,
                                               const quint8 *src_line,
                                               quint8 *dst_line,
                                               qint64 *aiMultTable,
                                               qint64 *aoMultTable,
                                               qint64 *alphaDivTable,
                                               int *x)
{
    int maxX = oWidth - (oWidth % 8);
    __m256i maskFF = _mm256_set1_epi8(0xff);

    for (; *x < maxX; *x += 8) {
        int xs[8], xs_x[8], xd_x[8];

        for (int j = 0; j < 8; ++j) {
            xs[j] = ((*x + j) * iDiffX + oMultX) / oDiffX;
            xs_x[j] = (xs[j] >> xiWidthDiv) * xiStep;
            xd_x[j] = ((*x + j) >> xiWidthDiv) * xiStep;
        }

        __m256i src_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_line + xs_x[0]));
        __m256i dst_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_line + xd_x[0]));

        __m256i xi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, xiShift), maskFF);
        __m256i yi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, yiShift), maskFF);
        __m256i zi = _mm256_and_si256(_mm256_srli_epi32(src_pixels, ziShift), maskFF);
        __m256i ai = _mm256_and_si256(_mm256_srli_epi32(src_pixels, aiShift), maskFF);

        __m256i xo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, xiShift), maskFF);
        __m256i yo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, yiShift), maskFF);
        __m256i zo = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, ziShift), maskFF);
        __m256i ao = _mm256_and_si256(_mm256_srli_epi32(dst_pixels, aiShift), maskFF);

        quint32 xi_arr[8], yi_arr[8], zi_arr[8], ai_arr[8];
        quint32 xo_arr[8], yo_arr[8], zo_arr[8], ao_arr[8];
        _mm256_store_si256(reinterpret_cast<__m256i *>(xi_arr), xi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(yi_arr), yi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(zi_arr), zi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ai_arr), ai);
        _mm256_store_si256(reinterpret_cast<__m256i *>(xo_arr), xo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(yo_arr), yo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(zo_arr), zo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ao_arr), ao);

        quint32 result[8];

        for (int j = 0; j < 8; ++j) {
            size_t alphaMask = (size_t(ai_arr[j]) << 8) | size_t(ao_arr[j]);
            qint64 xt = (qint64(xi_arr[j]) * aiMultTable[alphaMask] + qint64(xo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 yt = (qint64(yi_arr[j]) * aiMultTable[alphaMask] + qint64(yo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 zt = (qint64(zi_arr[j]) * aiMultTable[alphaMask] + qint64(zo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 at = alphaDivTable[alphaMask];

            result[j] = (quint32(xt) << xiShift)
                      | (quint32(yt) << yiShift)
                      | (quint32(zt) << ziShift)
                      | (quint32(at) << aiShift);
        }

        _mm256_store_si256(reinterpret_cast<__m256i *>(dst_line + xd_x[0]),
                           _mm256_load_si256(reinterpret_cast<__m256i *>(result)));
    }
}

void SimdCoreAVXPrivate::drawFastLc8bits1APack(int oWidth,
                                               int iDiffX,
                                               int oDiffX,
                                               int oMultX,
                                               size_t xiWidthDiv,
                                               size_t xiStep,
                                               size_t xiShift,
                                               size_t aiShift,
                                               size_t alphaShift,
                                               const quint8 *src_line,
                                               quint8 *dst_line,
                                               qint64 *aiMultTable,
                                               qint64 *aoMultTable,
                                               qint64 *alphaDivTable,
                                               int *x)
{
    int maxX = oWidth - (oWidth % 16);
    __m256i maskFF = _mm256_set1_epi8(0xff);

    for (; *x < maxX; *x += 16) {
        int xs[16], xs_x[16], xd_x[16];

        for (int j = 0; j < 16; ++j) {
            xs[j] = ((*x + j) * iDiffX + oMultX) / oDiffX;
            xs_x[j] = (xs[j] >> xiWidthDiv) * xiStep;
            xd_x[j] = ((*x + j) >> xiWidthDiv) * xiStep;
        }

        __m256i src_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_line + xs_x[0]));
        __m256i dst_pixels = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_line + xd_x[0]));

        __m256i xi = _mm256_and_si256(_mm256_srli_epi16(src_pixels, xiShift), maskFF);
        __m256i ai = _mm256_and_si256(_mm256_srli_epi16(src_pixels, aiShift), maskFF);
        __m256i xo = _mm256_and_si256(_mm256_srli_epi16(dst_pixels, xiShift), maskFF);
        __m256i ao = _mm256_and_si256(_mm256_srli_epi16(dst_pixels, aiShift), maskFF);

        quint16 xi_arr[16], ai_arr[16], xo_arr[16], ao_arr[16];
        _mm256_store_si256(reinterpret_cast<__m256i *>(xi_arr), xi);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ai_arr), ai);
        _mm256_store_si256(reinterpret_cast<__m256i *>(xo_arr), xo);
        _mm256_store_si256(reinterpret_cast<__m256i *>(ao_arr), ao);

        quint16 result[16];

        for (int j = 0; j < 16; ++j) {
            size_t alphaMask = (size_t(ai_arr[j]) << 8) | size_t(ao_arr[j]);
            qint64 xt = (qint64(xi_arr[j]) * aiMultTable[alphaMask] + qint64(xo_arr[j]) * aoMultTable[alphaMask]) >> alphaShift;
            qint64 at = alphaDivTable[alphaMask];

            result[j] = (quint16(xt) << xiShift) | (quint16(at) << aiShift);
        }

        _mm256_store_si256(reinterpret_cast<__m256i *>(dst_line + xd_x[0]),
                           _mm256_load_si256(reinterpret_cast<__m256i *>(result)));
    }
}

void SimdCoreAVXPrivate::fill3_8(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi8(static_cast<quint8>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi8(static_cast<quint8>(zo_ << zoShift));
    __m256i mask_xo = _mm256_set1_epi8(static_cast<quint8>(maskXo));
    __m256i mask_yo = _mm256_set1_epi8(static_cast<quint8>(maskYo));
    __m256i mask_zo = _mm256_set1_epi8(static_cast<quint8>(maskZo));

    size_t avx_width = width - (width % 32); // Process 32 pixels

    for (size_t i = *x; i < avx_width; i += 32) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill3_16(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi16(static_cast<quint16>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi16(static_cast<quint16>(zo_ << zoShift));
    __m256i mask_xo = _mm256_set1_epi16(static_cast<quint16>(maskXo));
    __m256i mask_yo = _mm256_set1_epi16(static_cast<quint16>(maskYo));
    __m256i mask_zo = _mm256_set1_epi16(static_cast<quint16>(maskZo));

    size_t avx_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < avx_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill3_32(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi32(static_cast<quint32>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi32(static_cast<quint32>(zo_ << zoShift));
    __m256i mask_xo = _mm256_set1_epi32(static_cast<quint32>(maskXo));
    __m256i mask_yo = _mm256_set1_epi32(static_cast<quint32>(maskYo));
    __m256i mask_zo = _mm256_set1_epi32(static_cast<quint32>(maskZo));

    size_t avx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < avx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill3A_8(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi8(static_cast<quint8>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi8(static_cast<quint8>(zo_ << zoShift));
    __m256i ao_val = _mm256_set1_epi8(static_cast<quint8>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi8(static_cast<quint8>(maskXo));
    __m256i mask_yo = _mm256_set1_epi8(static_cast<quint8>(maskYo));
    __m256i mask_zo = _mm256_set1_epi8(static_cast<quint8>(maskZo));
    __m256i mask_ao = _mm256_set1_epi8(static_cast<quint8>(maskAo));

    size_t avx_width = width - (width % 32); // Process 32 pixels

    for (size_t i = *x; i < avx_width; i += 32) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill3A_16(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi16(static_cast<quint16>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi16(static_cast<quint16>(zo_ << zoShift));
    __m256i ao_val = _mm256_set1_epi16(static_cast<quint16>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi16(static_cast<quint16>(maskXo));
    __m256i mask_yo = _mm256_set1_epi16(static_cast<quint16>(maskYo));
    __m256i mask_zo = _mm256_set1_epi16(static_cast<quint16>(maskZo));
    __m256i mask_ao = _mm256_set1_epi16(static_cast<quint16>(maskAo));

    size_t avx_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < avx_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill3A_32(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m256i yo_val = _mm256_set1_epi32(static_cast<quint32>(yo_ << yoShift));
    __m256i zo_val = _mm256_set1_epi32(static_cast<quint32>(zo_ << zoShift));
    __m256i ao_val = _mm256_set1_epi32(static_cast<quint32>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi32(static_cast<quint32>(maskXo));
    __m256i mask_yo = _mm256_set1_epi32(static_cast<quint32>(maskYo));
    __m256i mask_zo = _mm256_set1_epi32(static_cast<quint32>(maskZo));
    __m256i mask_ao = _mm256_set1_epi32(static_cast<quint32>(maskAo));

    size_t avx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < avx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_y = dstWidthOffsetY[i];
        int xd_z = dstWidthOffsetZ[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i yo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_y + xd_y));
        __m256i zo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_z + xd_z));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        yo = _mm256_and_si256(yo, mask_yo);
        zo = _mm256_and_si256(zo, mask_zo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        yo = _mm256_or_si256(yo, yo_val);
        zo = _mm256_or_si256(zo, zo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_y + xd_y), yo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_z + xd_z), zo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1_8(const int *dstWidthOffsetX,
                                 size_t xoShift,
                                 quint64 maskXo,
                                 qint64 xo_,
                                 size_t width,
                                 quint8 *line_x,
                                 size_t *x)
{
    __m256i xo_val = _mm256_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m256i mask_xo = _mm256_set1_epi8(static_cast<quint8>(maskXo));

    size_t avx_width = width - (width % 32); // Process 32 pixels

    for (size_t i = *x; i < avx_width; i += 32) {
        int xd_x = dstWidthOffsetX[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));

        xo = _mm256_and_si256(xo, mask_xo);
        xo = _mm256_or_si256(xo, xo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1_16(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m256i xo_val = _mm256_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m256i mask_xo = _mm256_set1_epi16(static_cast<quint16>(maskXo));

    size_t avx_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < avx_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));

        xo = _mm256_and_si256(xo, mask_xo);
        xo = _mm256_or_si256(xo, xo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1_32(const int *dstWidthOffsetX,
                                  size_t xoShift,
                                  quint64 maskXo,
                                  qint64 xo_,
                                  size_t width,
                                  quint8 *line_x,
                                  size_t *x)
{
    __m256i xo_val = _mm256_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m256i mask_xo = _mm256_set1_epi32(static_cast<quint32>(maskXo));

    size_t avx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < avx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));

        xo = _mm256_and_si256(xo, mask_xo);
        xo = _mm256_or_si256(xo, xo_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1A_8(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi8(static_cast<quint8>(xo_ << xoShift));
    __m256i ao_val = _mm256_set1_epi8(static_cast<quint8>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi8(static_cast<quint8>(maskXo));
    __m256i mask_ao = _mm256_set1_epi8(static_cast<quint8>(maskAo));

    size_t avx_width = width - (width % 32); // Process 32 pixels

    for (size_t i = *x; i < avx_width; i += 32) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1A_16(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi16(static_cast<quint16>(xo_ << xoShift));
    __m256i ao_val = _mm256_set1_epi16(static_cast<quint16>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi16(static_cast<quint16>(maskXo));
    __m256i mask_ao = _mm256_set1_epi16(static_cast<quint16>(maskAo));

    size_t avx_width = width - (width % 16); // Process 16 pixels

    for (size_t i = *x; i < avx_width; i += 16) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

void SimdCoreAVXPrivate::fill1A_32(const int *dstWidthOffsetX,
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
    __m256i xo_val = _mm256_set1_epi32(static_cast<quint32>(xo_ << xoShift));
    __m256i ao_val = _mm256_set1_epi32(static_cast<quint32>(ao_ << aoShift));
    __m256i mask_xo = _mm256_set1_epi32(static_cast<quint32>(maskXo));
    __m256i mask_ao = _mm256_set1_epi32(static_cast<quint32>(maskAo));

    size_t avx_width = width - (width % 8); // Process 8 pixels

    for (size_t i = *x; i < avx_width; i += 8) {
        int xd_x = dstWidthOffsetX[i];
        int xd_a = dstWidthOffsetA[i];

        __m256i xo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_x + xd_x));
        __m256i ao = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(line_a + xd_a));

        xo = _mm256_and_si256(xo, mask_xo);
        ao = _mm256_and_si256(ao, mask_ao);
        xo = _mm256_or_si256(xo, xo_val);
        ao = _mm256_or_si256(ao, ao_val);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_x + xd_x), xo);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(line_a + xd_a), ao);
    }

    *x = avx_width;
}

#include "moc_simdcoreavx.cpp"
