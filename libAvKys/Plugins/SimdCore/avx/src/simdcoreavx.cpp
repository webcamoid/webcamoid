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

        static void drawFast8bits3A(int oWidth,
                                    const int *srcWidthOffsetX,
                                    const int *srcWidthOffsetY,
                                    const int *srcWidthOffsetZ,
                                    const int *srcWidthOffsetA,
                                    const int *dstWidthOffsetX,
                                    const int *dstWidthOffsetY,
                                    const int *dstWidthOffsetZ,
                                    const int *dstWidthOffsetA,
                                    const quint8 *src_line_x,
                                    const quint8 *src_line_y,
                                    const quint8 *src_line_z,
                                    const quint8 *src_line_a,
                                    quint8 *dst_line_x,
                                    quint8 *dst_line_y,
                                    quint8 *dst_line_z,
                                    quint8 *dst_line_a,
                                    int *x);
        static void drawFast8bits1A(int oWidth,
                                    const int *srcWidthOffsetX,
                                    const int *srcWidthOffsetA,
                                    const int *dstWidthOffsetX,
                                    const int *dstWidthOffsetA,
                                    const quint8 *src_line_x,
                                    const quint8 *src_line_a,
                                    quint8 *dst_line_x,
                                    quint8 *dst_line_a,
                                    int *x);
        static void drawFastLc8bits3A(int oWidth,
                                      int iDiffX,
                                      int oDiffX,
                                      int oMultX,
                                      size_t xiWidthDiv,
                                      size_t yiWidthDiv,
                                      size_t ziWidthDiv,
                                      size_t aiWidthDiv,
                                      size_t xiStep,
                                      size_t yiStep,
                                      size_t ziStep,
                                      size_t aiStep,
                                      const quint8 *src_line_x,
                                      const quint8 *src_line_y,
                                      const quint8 *src_line_z,
                                      const quint8 *src_line_a,
                                      quint8 *dst_line_x,
                                      quint8 *dst_line_y,
                                      quint8 *dst_line_z,
                                      quint8 *dst_line_a,
                                      int *x);
        static void drawFastLc8bits1A(int oWidth,
                                      int iDiffX,
                                      int oDiffX,
                                      int oMultX,
                                      size_t xiWidthDiv,
                                      size_t aiWidthDiv,
                                      size_t xiStep,
                                      size_t aiStep,
                                      const quint8 *src_line_x,
                                      const quint8 *src_line_a,
                                      quint8 *dst_line_x,
                                      quint8 *dst_line_a,
                                      int *x);
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
    CHECK_FUNCTION(drawFast8bits1A)
    CHECK_FUNCTION(drawFast8bits3A)
    CHECK_FUNCTION(drawFastLc8bits1A)
    CHECK_FUNCTION(drawFastLc8bits3A)

    return nullptr;
}

void SimdCoreAVXPrivate::drawFast8bits3A(int oWidth,
                                         const int *srcWidthOffsetX,
                                         const int *srcWidthOffsetY,
                                         const int *srcWidthOffsetZ,
                                         const int *srcWidthOffsetA,
                                         const int *dstWidthOffsetX,
                                         const int *dstWidthOffsetY,
                                         const int *dstWidthOffsetZ,
                                         const int *dstWidthOffsetA,
                                         const quint8 *src_line_x,
                                         const quint8 *src_line_y,
                                         const quint8 *src_line_z,
                                         const quint8 *src_line_a,
                                         quint8 *dst_line_x,
                                         quint8 *dst_line_y,
                                         quint8 *dst_line_z,
                                         quint8 *dst_line_a,
                                         int *x)
{
    // Process 16 pixels at a time (256 bits / 16 bits = 16 pixels after unpacking)
    const int simd_width = 16;
    int i = *x;

    // Gather 16 pixels for each channel manually into aligned arrays
    alignas(32) quint8 src_x_data[16], src_y_data[16], src_z_data[16], src_a_data[16];
    alignas(32) quint8 dst_x_data[16], dst_y_data[16], dst_z_data[16], dst_a_data[16];

    for (; i <= oWidth - simd_width; i += simd_width) {
        // Load pixel data using offset arrays
        for (int j = 0; j < simd_width; ++j) {
            src_x_data[j] = src_line_x[srcWidthOffsetX[i + j]];
            src_y_data[j] = src_line_y[srcWidthOffsetY[i + j]];
            src_z_data[j] = src_line_z[srcWidthOffsetZ[i + j]];
            src_a_data[j] = src_line_a[srcWidthOffsetA[i + j]];

            dst_x_data[j] = dst_line_x[dstWidthOffsetX[i + j]];
            dst_y_data[j] = dst_line_y[dstWidthOffsetY[i + j]];
            dst_z_data[j] = dst_line_z[dstWidthOffsetZ[i + j]];
            dst_a_data[j] = dst_line_a[dstWidthOffsetA[i + j]];
        }

        // Load gathered data into AVX registers
        __m256i src_x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_x_data));
        __m256i src_y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_y_data));
        __m256i src_z = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_z_data));
        __m256i src_a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_a_data));

        __m256i dst_x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_x_data));
        __m256i dst_y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_y_data));
        __m256i dst_z = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_z_data));
        __m256i dst_a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_a_data));

        // Unpack to 16-bit for multiplication (low 16 bytes)
        __m256i src_a_lo = _mm256_unpacklo_epi8(src_a, _mm256_setzero_si256());
        __m256i inv_a_lo = _mm256_sub_epi16(_mm256_set1_epi16(255), src_a_lo); // 255 - src_alpha

        // Process X channel: (src_x * src_alpha + dst_x * (255 - src_alpha)) >> 8
        __m256i src_x_lo = _mm256_unpacklo_epi8(src_x, _mm256_setzero_si256());
        __m256i dst_x_lo = _mm256_unpacklo_epi8(dst_x, _mm256_setzero_si256());

        src_x_lo = _mm256_mullo_epi16(src_x_lo, src_a_lo);
        dst_x_lo = _mm256_mullo_epi16(dst_x_lo, inv_a_lo);
        src_x_lo = _mm256_add_epi16(src_x_lo, dst_x_lo);
        src_x_lo = _mm256_srli_epi16(src_x_lo, 8);
        __m256i result_x = _mm256_packus_epi16(src_x_lo, _mm256_setzero_si256());

        // Process Y channel: (src_y * src_alpha + dst_y * (255 - src_alpha)) >> 8
        __m256i src_y_lo = _mm256_unpacklo_epi8(src_y, _mm256_setzero_si256());
        __m256i dst_y_lo = _mm256_unpacklo_epi8(dst_y, _mm256_setzero_si256());

        src_y_lo = _mm256_mullo_epi16(src_y_lo, src_a_lo);
        dst_y_lo = _mm256_mullo_epi16(dst_y_lo, inv_a_lo);
        src_y_lo = _mm256_add_epi16(src_y_lo, dst_y_lo);
        src_y_lo = _mm256_srli_epi16(src_y_lo, 8);
        __m256i result_y = _mm256_packus_epi16(src_y_lo, _mm256_setzero_si256());

        // Process Z channel: (src_z * src_alpha + dst_z * (255 - src_alpha)) >> 8
        __m256i src_z_lo = _mm256_unpacklo_epi8(src_z, _mm256_setzero_si256());
        __m256i dst_z_lo = _mm256_unpacklo_epi8(dst_z, _mm256_setzero_si256());

        src_z_lo = _mm256_mullo_epi16(src_z_lo, src_a_lo);
        dst_z_lo = _mm256_mullo_epi16(dst_z_lo, inv_a_lo);
        src_z_lo = _mm256_add_epi16(src_z_lo, dst_z_lo);
        src_z_lo = _mm256_srli_epi16(src_z_lo, 8);
        __m256i result_z = _mm256_packus_epi16(src_z_lo, _mm256_setzero_si256());

        // Process A channel: dst_a = src_a + dst_a * (255 - src_a) / 255
        __m256i dst_a_lo = _mm256_unpacklo_epi8(dst_a, _mm256_setzero_si256());
        dst_a_lo = _mm256_mullo_epi16(dst_a_lo, inv_a_lo);
        dst_a_lo = _mm256_srli_epi16(dst_a_lo, 8);
        dst_a_lo = _mm256_add_epi16(dst_a_lo, src_a_lo);
        __m256i result_a = _mm256_packus_epi16(dst_a_lo, _mm256_setzero_si256());

        // Store results back to temporary arrays
        alignas(32) quint8 result_x_data[16], result_y_data[16], result_z_data[16], result_a_data[16];
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_x_data), result_x);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_y_data), result_y);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_z_data), result_z);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_a_data), result_a);

        // Scatter results to destination using offset arrays
        for (int j = 0; j < simd_width; ++j) {
            dst_line_x[dstWidthOffsetX[i + j]] = result_x_data[j];
            dst_line_y[dstWidthOffsetY[i + j]] = result_y_data[j];
            dst_line_z[dstWidthOffsetZ[i + j]] = result_z_data[j];
            dst_line_a[dstWidthOffsetA[i + j]] = result_a_data[j];
        }
    }

    // Update x for scalar processing of remaining pixels
    *x = i;
}

void SimdCoreAVXPrivate::drawFast8bits1A(int oWidth,
                                         const int *srcWidthOffsetX,
                                         const int *srcWidthOffsetA,
                                         const int *dstWidthOffsetX,
                                         const int *dstWidthOffsetA,
                                         const quint8 *src_line_x,
                                         const quint8 *src_line_a,
                                         quint8 *dst_line_x,
                                         quint8 *dst_line_a,
                                         int *x)
{
    // Constants
    const __m256i c_255 = _mm256_set1_epi8(255); // 255 for alpha complement
    const __m256i c_zero = _mm256_setzero_si256(); // Zero for unpacking

    // Process 32 pixels at a time
    int i = *x;

    for (; i <= oWidth - 32; i += 32) {
        // Gather 32 pixels for each channel into aligned arrays
        alignas(32) quint8 src_x_data[32], src_a_data[32];
        alignas(32) quint8 dst_x_data[32], dst_a_data[32];

        for (int j = 0; j < 32; ++j) {
            src_x_data[j] = src_line_x[srcWidthOffsetX[i + j]];
            src_a_data[j] = src_line_a[srcWidthOffsetA[i + j]];
            dst_x_data[j] = dst_line_x[dstWidthOffsetX[i + j]];
            dst_a_data[j] = dst_line_a[dstWidthOffsetA[i + j]];
        }

        // Load gathered data into AVX2 registers
        __m256i src_x = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_x_data));
        __m256i src_a = _mm256_load_si256(reinterpret_cast<const __m256i *>(src_a_data));
        __m256i dst_x = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_x_data));
        __m256i dst_a = _mm256_load_si256(reinterpret_cast<const __m256i *>(dst_a_data));

        // Compute complement alpha: (255 - src_a)
        __m256i complement_a = _mm256_sub_epi8(c_255, src_a);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Unpack to 16-bit to avoid overflow (low and high 16 pixels)
        __m256i src_x_lo = _mm256_unpacklo_epi8(src_x, c_zero);
        __m256i src_x_hi = _mm256_unpackhi_epi8(src_x, c_zero);
        __m256i src_a_lo = _mm256_unpacklo_epi8(src_a, c_zero);
        __m256i src_a_hi = _mm256_unpackhi_epi8(src_a, c_zero);
        __m256i dst_x_lo = _mm256_unpacklo_epi8(dst_x, c_zero);
        __m256i dst_x_hi = _mm256_unpackhi_epi8(dst_x, c_zero);
        __m256i comp_a_lo = _mm256_unpacklo_epi8(complement_a, c_zero);
        __m256i comp_a_hi = _mm256_unpackhi_epi8(complement_a, c_zero);

        // Compute src_x * src_a
        __m256i src_prod_lo = _mm256_mullo_epi16(src_x_lo, src_a_lo);
        __m256i src_prod_hi = _mm256_mullo_epi16(src_x_hi, src_a_hi);

        // Compute dst_x * (255 - src_a)
        __m256i dst_prod_lo = _mm256_mullo_epi16(dst_x_lo, comp_a_lo);
        __m256i dst_prod_hi = _mm256_mullo_epi16(dst_x_hi, comp_a_hi);

        // Sum products
        __m256i sum_lo = _mm256_add_epi16(src_prod_lo, dst_prod_lo);
        __m256i sum_hi = _mm256_add_epi16(src_prod_hi, dst_prod_hi);

        // Divide by 256 (approximates /255)
        sum_lo = _mm256_srli_epi16(sum_lo, 8);
        sum_hi = _mm256_srli_epi16(sum_hi, 8);

        // Pack back to 8-bit
        __m256i result_x = _mm256_packus_epi16(sum_lo, sum_hi);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Unpack destination alpha and complement alpha
        __m256i dst_a_lo = _mm256_unpacklo_epi8(dst_a, c_zero);
        __m256i dst_a_hi = _mm256_unpackhi_epi8(dst_a, c_zero);

        // Compute dst_a * (255 - src_a)
        __m256i alpha_prod_lo = _mm256_mullo_epi16(dst_a_lo, comp_a_lo);
        __m256i alpha_prod_hi = _mm256_mullo_epi16(dst_a_hi, comp_a_hi);

        // Divide by 256
        alpha_prod_lo = _mm256_srli_epi16(alpha_prod_lo, 8);
        alpha_prod_hi = _mm256_srli_epi16(alpha_prod_hi, 8);

        // Add src_a
        __m256i src_a_lo_16 = _mm256_unpacklo_epi8(src_a, c_zero);
        __m256i src_a_hi_16 = _mm256_unpackhi_epi8(src_a, c_zero);
        __m256i alpha_sum_lo = _mm256_add_epi16(alpha_prod_lo, src_a_lo_16);
        __m256i alpha_sum_hi = _mm256_add_epi16(alpha_prod_hi, src_a_hi_16);

        // Pack back to 8-bit
        __m256i result_a = _mm256_packus_epi16(alpha_sum_lo, alpha_sum_hi);

        // Store results
        alignas(32) quint8 result_x_data[32], result_a_data[32];
        _mm256_store_si256(reinterpret_cast<__m256i *>(result_x_data), result_x);
        _mm256_store_si256(reinterpret_cast<__m256i *>(result_a_data), result_a);

        for (int j = 0; j < 32; ++j) {
            dst_line_x[dstWidthOffsetX[i + j]] = result_x_data[j];
            dst_line_a[dstWidthOffsetA[i + j]] = result_a_data[j];
        }
    }

    // Update x for scalar fallback
    *x = i;
}

void SimdCoreAVXPrivate::drawFastLc8bits3A(int oWidth,
                                           int iDiffX,
                                           int oDiffX,
                                           int oMultX,
                                           size_t xiWidthDiv,
                                           size_t yiWidthDiv,
                                           size_t ziWidthDiv,
                                           size_t aiWidthDiv,
                                           size_t xiStep,
                                           size_t yiStep,
                                           size_t ziStep,
                                           size_t aiStep,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           quint8 *dst_line_a,
                                           int *x)
{
    // Constants
    const __m256i c_255 = _mm256_set1_epi8(255); // 255 for alpha complement
    const __m256i c_zero = _mm256_setzero_si256(); // Zero for unpacking

    // Process 16 pixels at a time
    int i = *x;

    for (; i <= oWidth - 16; i += 16) {
        // Gather 16 pixels for each channel
        alignas(32) quint8 src_x_data[16], src_y_data[16], src_z_data[16], src_a_data[16];
        alignas(32) quint8 dst_x_data[16], dst_y_data[16], dst_z_data[16], dst_a_data[16];

        // Compute source and destination indices
        for (int j = 0; j < 16; ++j) {
            int xs = ((i + j) * iDiffX + oMultX) / oDiffX;
            int xs_x = (xs >> xiWidthDiv) * xiStep;
            int xs_y = (xs >> yiWidthDiv) * yiStep;
            int xs_z = (xs >> ziWidthDiv) * ziStep;
            int xs_a = (xs >> aiWidthDiv) * aiStep;

            int xd = i + j;
            int xd_x = (xd >> xiWidthDiv) * xiStep;
            int xd_y = (xd >> yiWidthDiv) * yiStep;
            int xd_z = (xd >> ziWidthDiv) * ziStep;
            int xd_a = (xd >> aiWidthDiv) * aiStep;

            src_x_data[j] = src_line_x[xs_x];
            src_y_data[j] = src_line_y[xs_y];
            src_z_data[j] = src_line_z[xs_z];
            src_a_data[j] = src_line_a[xs_a];

            dst_x_data[j] = dst_line_x[xd_x];
            dst_y_data[j] = dst_line_y[xd_y];
            dst_z_data[j] = dst_line_z[xd_z];
            dst_a_data[j] = dst_line_a[xd_a];
        }

        // Load gathered data into AVX2 registers
        __m256i src_x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_x_data));
        __m256i src_y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_y_data));
        __m256i src_z = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_z_data));
        __m256i src_a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src_a_data));

        __m256i dst_x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_x_data));
        __m256i dst_y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_y_data));
        __m256i dst_z = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_z_data));
        __m256i dst_a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst_a_data));

        // Compute complement alpha: (255 - src_a)
        __m256i complement_a = _mm256_sub_epi8(c_255, src_a);

        // Unpack to 16-bit for multiplication (low and high 8 pixels)
        __m256i src_a_lo = _mm256_unpacklo_epi8(src_a, c_zero);
        __m256i src_a_hi = _mm256_unpackhi_epi8(src_a, c_zero);
        __m256i comp_a_lo = _mm256_unpacklo_epi8(complement_a, c_zero);
        __m256i comp_a_hi = _mm256_unpackhi_epi8(complement_a, c_zero);

        // --- X channel blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        __m256i src_x_lo = _mm256_unpacklo_epi8(src_x, c_zero);
        __m256i src_x_hi = _mm256_unpackhi_epi8(src_x, c_zero);
        __m256i dst_x_lo = _mm256_unpacklo_epi8(dst_x, c_zero);
        __m256i dst_x_hi = _mm256_unpackhi_epi8(dst_x, c_zero);

        __m256i src_x_prod_lo = _mm256_mullo_epi16(src_x_lo, src_a_lo);
        __m256i src_x_prod_hi = _mm256_mullo_epi16(src_x_hi, src_a_hi);
        __m256i dst_x_prod_lo = _mm256_mullo_epi16(dst_x_lo, comp_a_lo);
        __m256i dst_x_prod_hi = _mm256_mullo_epi16(dst_x_hi, comp_a_hi);

        __m256i sum_x_lo = _mm256_add_epi16(src_x_prod_lo, dst_x_prod_lo);
        __m256i sum_x_hi = _mm256_add_epi16(src_x_prod_hi, dst_x_prod_hi);

        sum_x_lo = _mm256_srli_epi16(sum_x_lo, 8); // Divide by 256
        sum_x_hi = _mm256_srli_epi16(sum_x_hi, 8);

        __m256i result_x = _mm256_packus_epi16(sum_x_lo, sum_x_hi);

        // --- Y channel blending: (src_y * src_a + dst_y * (255 - src_a)) / 255 ---
        __m256i src_y_lo = _mm256_unpacklo_epi8(src_y, c_zero);
        __m256i src_y_hi = _mm256_unpackhi_epi8(src_y, c_zero);
        __m256i dst_y_lo = _mm256_unpacklo_epi8(dst_y, c_zero);
        __m256i dst_y_hi = _mm256_unpackhi_epi8(dst_y, c_zero);

        __m256i src_y_prod_lo = _mm256_mullo_epi16(src_y_lo, src_a_lo);
        __m256i src_y_prod_hi = _mm256_mullo_epi16(src_y_hi, src_a_hi);
        __m256i dst_y_prod_lo = _mm256_mullo_epi16(dst_y_lo, comp_a_lo);
        __m256i dst_y_prod_hi = _mm256_mullo_epi16(dst_y_hi, comp_a_hi);

        __m256i sum_y_lo = _mm256_add_epi16(src_y_prod_lo, dst_y_prod_lo);
        __m256i sum_y_hi = _mm256_add_epi16(src_y_prod_hi, dst_y_prod_hi);

        sum_y_lo = _mm256_srli_epi16(sum_y_lo, 8);
        sum_y_hi = _mm256_srli_epi16(sum_y_hi, 8);

        __m256i result_y = _mm256_packus_epi16(sum_y_lo, sum_y_hi);

        // --- Z channel blending: (src_z * src_a + dst_z * (255 - src_a)) / 255 ---
        __m256i src_z_lo = _mm256_unpacklo_epi8(src_z, c_zero);
        __m256i src_z_hi = _mm256_unpackhi_epi8(src_z, c_zero);
        __m256i dst_z_lo = _mm256_unpacklo_epi8(dst_z, c_zero);
        __m256i dst_z_hi = _mm256_unpackhi_epi8(dst_z, c_zero);

        __m256i src_z_prod_lo = _mm256_mullo_epi16(src_z_lo, src_a_lo);
        __m256i src_z_prod_hi = _mm256_mullo_epi16(src_z_hi, src_a_hi);
        __m256i dst_z_prod_lo = _mm256_mullo_epi16(dst_z_lo, comp_a_lo);
        __m256i dst_z_prod_hi = _mm256_mullo_epi16(dst_z_hi, comp_a_hi);

        __m256i sum_z_lo = _mm256_add_epi16(src_z_prod_lo, dst_z_prod_lo);
        __m256i sum_z_hi = _mm256_add_epi16(src_z_prod_hi, dst_z_prod_hi);

        sum_z_lo = _mm256_srli_epi16(sum_z_lo, 8);
        sum_z_hi = _mm256_srli_epi16(sum_z_hi, 8);

        __m256i result_z = _mm256_packus_epi16(sum_z_lo, sum_z_hi);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        __m256i dst_a_lo = _mm256_unpacklo_epi8(dst_a, c_zero);
        __m256i dst_a_hi = _mm256_unpackhi_epi8(dst_a, c_zero);

        __m256i alpha_prod_lo = _mm256_mullo_epi16(dst_a_lo, comp_a_lo);
        __m256i alpha_prod_hi = _mm256_mullo_epi16(dst_a_hi, comp_a_hi);

        alpha_prod_lo = _mm256_srli_epi16(alpha_prod_lo, 8);
        alpha_prod_hi = _mm256_srli_epi16(alpha_prod_hi, 8);

        __m256i alpha_sum_lo = _mm256_add_epi16(alpha_prod_lo, src_a_lo);
        __m256i alpha_sum_hi = _mm256_add_epi16(alpha_prod_hi, src_a_hi);

        __m256i result_a = _mm256_packus_epi16(alpha_sum_lo, alpha_sum_hi);

        // Store results
        alignas(32) quint8 result_x_data[16], result_y_data[16], result_z_data[16], result_a_data[16];
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_x_data), result_x);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_y_data), result_y);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_z_data), result_z);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(result_a_data), result_a);

        for (int j = 0; j < 16; ++j) {
            int xd = i + j;
            int xd_x = (xd >> xiWidthDiv) * xiStep;
            int xd_y = (xd >> yiWidthDiv) * yiStep;
            int xd_z = (xd >> ziWidthDiv) * ziStep;
            int xd_a = (xd >> aiWidthDiv) * aiStep;

            dst_line_x[xd_x] = result_x_data[j];
            dst_line_y[xd_y] = result_y_data[j];
            dst_line_z[xd_z] = result_z_data[j];
            dst_line_a[xd_a] = result_a_data[j];
        }
    }

    // Update x for scalar fallback
    *x = i;
}

void SimdCoreAVXPrivate::drawFastLc8bits1A(int oWidth,
                                           int iDiffX,
                                           int oDiffX,
                                           int oMultX,
                                           size_t xiWidthDiv,
                                           size_t aiWidthDiv,
                                           size_t xiStep,
                                           size_t aiStep,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_a,
                                           int *x)
{
    // Number of pixels processed per iteration (32 pixels with AVX)
    const int pixels_per_iteration = 32;
    int i = *x;

    // Process pixels in chunks of 32 using AVX
    while (i + pixels_per_iteration <= oWidth) {
        // Calculate source and destination offsets for 32 pixels
        alignas(32) uint8_t src_x_data[32], src_a_data[32], dst_x_data[32], dst_a_data[32];

        for (int j = 0; j < pixels_per_iteration; ++j) {
            int xs = ((i + j) * iDiffX + oMultX) / oDiffX;
            src_x_data[j] = src_line_x[(xs >> xiWidthDiv) * xiStep];
            src_a_data[j] = src_line_a[(xs >> aiWidthDiv) * aiStep];
            dst_x_data[j] = dst_line_x[((i + j) >> xiWidthDiv) * xiStep];
            dst_a_data[j] = dst_line_a[((i + j) >> aiWidthDiv) * aiStep];
        }

        // Load 32 pixels into AVX registers
        __m256i src_x = _mm256_load_si256(reinterpret_cast<__m256i *>(src_x_data)); // Load grayscale source
        __m256i src_a = _mm256_load_si256(reinterpret_cast<__m256i *>(src_a_data)); // Load alpha source
        __m256i dst_x = _mm256_load_si256(reinterpret_cast<__m256i *>(dst_x_data)); // Load grayscale destination
        __m256i dst_a = _mm256_load_si256(reinterpret_cast<__m256i *>(dst_a_data)); // Load alpha destination

        // Compute complement alpha: (255 - src_a)
        __m256i const_255 = _mm256_set1_epi8(255);
        __m256i complement_a = _mm256_sub_epi8(const_255, src_a);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Unpack to 16-bit to avoid overflow (process in two 128-bit lanes)
        __m256i src_x_lo = _mm256_unpacklo_epi8(src_x, _mm256_setzero_si256());
        __m256i src_x_hi = _mm256_unpackhi_epi8(src_x, _mm256_setzero_si256());
        __m256i src_a_lo = _mm256_unpacklo_epi8(src_a, _mm256_setzero_si256());
        __m256i src_a_hi = _mm256_unpackhi_epi8(src_a, _mm256_setzero_si256());
        __m256i dst_x_lo = _mm256_unpacklo_epi8(dst_x, _mm256_setzero_si256());
        __m256i dst_x_hi = _mm256_unpackhi_epi8(dst_x, _mm256_setzero_si256());
        __m256i comp_a_lo = _mm256_unpacklo_epi8(complement_a, _mm256_setzero_si256());
        __m256i comp_a_hi = _mm256_unpackhi_epi8(complement_a, _mm256_setzero_si256());

        // Compute src_x * src_a
        __m256i src_prod_lo = _mm256_mullo_epi16(src_x_lo, src_a_lo);
        __m256i src_prod_hi = _mm256_mullo_epi16(src_x_hi, src_a_hi);

        // Compute dst_x * (255 - src_a)
        __m256i dst_prod_lo = _mm256_mullo_epi16(dst_x_lo, comp_a_lo);
        __m256i dst_prod_hi = _mm256_mullo_epi16(dst_x_hi, comp_a_hi);

        // Sum products
        __m256i sum_lo = _mm256_add_epi16(src_prod_lo, dst_prod_lo);
        __m256i sum_hi = _mm256_add_epi16(src_prod_hi, dst_prod_hi);

        // Divide by 256 (approximates /255) using right shift
        sum_lo = _mm256_srli_epi16(sum_lo, 8);
        sum_hi = _mm256_srli_epi16(sum_hi, 8);

        // Pack back to 8-bit
        __m256i result_x = _mm256_packus_epi16(sum_lo, sum_hi);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Unpack dst_a
        __m256i dst_a_lo = _mm256_unpacklo_epi8(dst_a, _mm256_setzero_si256());
        __m256i dst_a_hi = _mm256_unpackhi_epi8(dst_a, _mm256_setzero_si256());

        // Compute dst_a * (255 - src_a)
        __m256i alpha_prod_lo = _mm256_mullo_epi16(dst_a_lo, comp_a_lo);
        __m256i alpha_prod_hi = _mm256_mullo_epi16(dst_a_hi, comp_a_hi);

        // Divide by 256
        alpha_prod_lo = _mm256_srli_epi16(alpha_prod_lo, 8);
        alpha_prod_hi = _mm256_srli_epi16(alpha_prod_hi, 8);

        // Add src_a
        __m256i alpha_sum_lo = _mm256_add_epi16(alpha_prod_lo, src_a_lo);
        __m256i alpha_sum_hi = _mm256_add_epi16(alpha_prod_hi, src_a_hi);

        // Pack back to 8-bit
        __m256i result_a = _mm256_packus_epi16(alpha_sum_lo, alpha_sum_hi);

        // Store results
        alignas(32) uint8_t result_x_data[32], result_a_data[32];
        _mm256_store_si256(reinterpret_cast<__m256i *>(result_x_data), result_x);
        _mm256_store_si256(reinterpret_cast<__m256i *>(result_a_data), result_a);

        // Write results back to destination
        for (int j = 0; j < pixels_per_iteration; ++j) {
            dst_line_x[((i + j) >> xiWidthDiv) * xiStep] = result_x_data[j];
            dst_line_a[((i + j) >> aiWidthDiv) * aiStep] = result_a_data[j];
        }

        // Advance index
        i += pixels_per_iteration;
    }

    // Update x for scalar fallback
    *x = i;
}

#include "moc_simdcoreavx.cpp"
