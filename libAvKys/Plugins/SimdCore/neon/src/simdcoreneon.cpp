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
    CHECK_FUNCTION(drawFast8bits1A)
    CHECK_FUNCTION(drawFast8bits3A)
    CHECK_FUNCTION(drawFastLc8bits1A)
    CHECK_FUNCTION(drawFastLc8bits3A)

    return nullptr;
}

void SimdCoreNEONPrivate::drawFast8bits3A(int oWidth,
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
    const int simd_width = 8; // Process 8 pixels per NEON iteration
    int i = *x;

    // Aligned temporary buffers for gathering/scattering data
    alignas(16) quint8 src_x_data[16] = {0};
    alignas(16) quint8 src_y_data[16] = {0};
    alignas(16) quint8 src_z_data[16] = {0};
    alignas(16) quint8 src_a_data[16] = {0};
    alignas(16) quint8 dst_x_data[16] = {0};
    alignas(16) quint8 dst_y_data[16] = {0};
    alignas(16) quint8 dst_z_data[16] = {0};
    alignas(16) quint8 dst_a_data[16] = {0};
    alignas(16) quint8 result_x_data[16] = {0};
    alignas(16) quint8 result_y_data[16] = {0};
    alignas(16) quint8 result_z_data[16] = {0};
    alignas(16) quint8 result_a_data[16] = {0};

    // NEON processing loop
    for (; i <= oWidth - simd_width; i += simd_width) {
        // Gather source and destination data for 8 pixels
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

        // Load data into NEON registers
        uint8x8_t src_x = vld1_u8(src_x_data);
        uint8x8_t src_y = vld1_u8(src_y_data);
        uint8x8_t src_z = vld1_u8(src_z_data);
        uint8x8_t src_a = vld1_u8(src_a_data);
        uint8x8_t dst_x = vld1_u8(dst_x_data);
        uint8x8_t dst_y = vld1_u8(dst_y_data);
        uint8x8_t dst_z = vld1_u8(dst_z_data);
        uint8x8_t dst_a = vld1_u8(dst_a_data);

        // Extend to 16-bit for arithmetic
        uint16x8_t src_a_16 = vmovl_u8(src_a); // src_a to 16-bit
        uint16x8_t inv_a_16 = vsubq_u16(vdupq_n_u16(255), src_a_16); // 255 - src_a

        // Process R channel
        uint16x8_t src_x_16 = vmovl_u8(src_x);
        uint16x8_t dst_x_16 = vmovl_u8(dst_x);
        src_x_16 = vmulq_u16(src_x_16, src_a_16); // src_x * src_a
        dst_x_16 = vmulq_u16(dst_x_16, inv_a_16); // dst_x * (255 - src_a)
        src_x_16 = vaddq_u16(src_x_16, dst_x_16); // Sum
        src_x_16 = vshrq_n_u16(src_x_16, 8); // Divide by 255
        uint8x8_t result_x = vqmovn_u16(src_x_16); // Narrow to 8-bit

        // Process G channel
        uint16x8_t src_y_16 = vmovl_u8(src_y);
        uint16x8_t dst_y_16 = vmovl_u8(dst_y);
        src_y_16 = vmulq_u16(src_y_16, src_a_16);
        dst_y_16 = vmulq_u16(dst_y_16, inv_a_16);
        src_y_16 = vaddq_u16(src_y_16, dst_y_16);
        src_y_16 = vshrq_n_u16(src_y_16, 8);
        uint8x8_t result_y = vqmovn_u16(src_y_16);

        // Process B channel
        uint16x8_t src_z_16 = vmovl_u8(src_z);
        uint16x8_t dst_z_16 = vmovl_u8(dst_z);
        src_z_16 = vmulq_u16(src_z_16, src_a_16);
        dst_z_16 = vmulq_u16(dst_z_16, inv_a_16);
        src_z_16 = vaddq_u16(src_z_16, dst_z_16);
        src_z_16 = vshrq_n_u16(src_z_16, 8);
        uint8x8_t result_z = vqmovn_u16(src_z_16);

        // Process A channel
        uint16x8_t dst_a_16 = vmovl_u8(dst_a);
        dst_a_16 = vmulq_u16(dst_a_16, inv_a_16); // dst_a * (255 - src_a)
        dst_a_16 = vshrq_n_u16(dst_a_16, 8); // Divide by 255
        dst_a_16 = vaddq_u16(dst_a_16, src_a_16); // + src_a
        uint8x8_t result_a = vqmovn_u16(dst_a_16);

        // Store results to temporary buffers
        vst1_u8(result_x_data, result_x);
        vst1_u8(result_y_data, result_y);
        vst1_u8(result_z_data, result_z);
        vst1_u8(result_a_data, result_a);

        // Scatter results to destination
        for (int j = 0; j < simd_width; ++j) {
            dst_line_x[dstWidthOffsetX[i + j]] = result_x_data[j];
            dst_line_y[dstWidthOffsetY[i + j]] = result_y_data[j];
            dst_line_z[dstWidthOffsetZ[i + j]] = result_z_data[j];
            dst_line_a[dstWidthOffsetA[i + j]] = result_a_data[j];
        }
    }

    // Update x for scalar processing
    *x = i;
}

void SimdCoreNEONPrivate::drawFast8bits1A(int oWidth,
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
    const uint8x16_t c_255 = vdupq_n_u8(255); // 255 for alpha complement

    // Process 16 pixels at a time
    int i = *x;

    for (; i <= oWidth - 16; i += 16) {
        // Gather 16 pixels for each channel into aligned arrays
        alignas(16) quint8 src_x_data[16], src_a_data[16];
        alignas(16) quint8 dst_x_data[16], dst_a_data[16];

        for (int j = 0; j < 16; ++j) {
            src_x_data[j] = src_line_x[srcWidthOffsetX[i + j]];
            src_a_data[j] = src_line_a[srcWidthOffsetA[i + j]];
            dst_x_data[j] = dst_line_x[dstWidthOffsetX[i + j]];
            dst_a_data[j] = dst_line_a[dstWidthOffsetA[i + j]];
        }

        // Load gathered data into NEON registers
        uint8x16_t src_x = vld1q_u8(src_x_data);
        uint8x16_t src_a = vld1q_u8(src_a_data);
        uint8x16_t dst_x = vld1q_u8(dst_x_data);
        uint8x16_t dst_a = vld1q_u8(dst_a_data);

        // Compute complement alpha: (255 - src_a)
        uint8x16_t complement_a = vsubq_u8(c_255, src_a);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Unpack to 16-bit to avoid overflow (low and high 8 pixels)
        uint16x8_t src_x_lo = vmovl_u8(vget_low_u8(src_x));
        uint16x8_t src_x_hi = vmovl_u8(vget_high_u8(src_x));
        uint16x8_t src_a_lo = vmovl_u8(vget_low_u8(src_a));
        uint16x8_t src_a_hi = vmovl_u8(vget_high_u8(src_a));
        uint16x8_t dst_x_lo = vmovl_u8(vget_low_u8(dst_x));
        uint16x8_t dst_x_hi = vmovl_u8(vget_high_u8(dst_x));
        uint16x8_t comp_a_lo = vmovl_u8(vget_low_u8(complement_a));
        uint16x8_t comp_a_hi = vmovl_u8(vget_high_u8(complement_a));

        // Compute src_x * src_a
        uint16x8_t src_prod_lo = vmulq_u16(src_x_lo, src_a_lo);
        uint16x8_t src_prod_hi = vmulq_u16(src_x_hi, src_a_hi);

        // Compute dst_x * (255 - src_a)
        uint16x8_t dst_prod_lo = vmulq_u16(dst_x_lo, comp_a_lo);
        uint16x8_t dst_prod_hi = vmulq_u16(dst_x_hi, comp_a_hi);

        // Sum products
        uint16x8_t sum_lo = vaddq_u16(src_prod_lo, dst_prod_lo);
        uint16x8_t sum_hi = vaddq_u16(src_prod_hi, dst_prod_hi);

        // Divide by 256 (approximates /255)
        sum_lo = vshrq_n_u16(sum_lo, 8);
        sum_hi = vshrq_n_u16(sum_hi, 8);

        // Pack back to 8-bit
        uint8x16_t result_x = vcombine_u8(vqmovn_u16(sum_lo), vqmovn_u16(sum_hi));

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Unpack destination alpha and complement alpha
        uint16x8_t dst_a_lo = vmovl_u8(vget_low_u8(dst_a));
        uint16x8_t dst_a_hi = vmovl_u8(vget_high_u8(dst_a));

        // Compute dst_a * (255 - src_a)
        uint16x8_t alpha_prod_lo = vmulq_u16(dst_a_lo, comp_a_lo);
        uint16x8_t alpha_prod_hi = vmulq_u16(dst_a_hi, comp_a_hi);

        // Divide by 256
        alpha_prod_lo = vshrq_n_u16(alpha_prod_lo, 8);
        alpha_prod_hi = vshrq_n_u16(alpha_prod_hi, 8);

        // Add src_a
        uint16x8_t alpha_sum_lo = vaddq_u16(alpha_prod_lo, src_a_lo);
        uint16x8_t alpha_sum_hi = vaddq_u16(alpha_prod_hi, src_a_hi);

        // Pack back to 8-bit
        uint8x16_t result_a = vcombine_u8(vqmovn_u16(alpha_sum_lo), vqmovn_u16(alpha_sum_hi));

        // Store results
        alignas(16) quint8 result_x_data[16], result_a_data[16];
        vst1q_u8(result_x_data, result_x);
        vst1q_u8(result_a_data, result_a);

        for (int j = 0; j < 16; ++j) {
            dst_line_x[dstWidthOffsetX[i + j]] = result_x_data[j];
            dst_line_a[dstWidthOffsetA[i + j]] = result_a_data[j];
        }
    }

    // Update x for scalar fallback
    *x = i;
}

void SimdCoreNEONPrivate::drawFastLc8bits3A(int oWidth,
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
    const uint8x8_t c_255 = vdup_n_u8(255); // 255 for alpha complement
    const uint8x8_t c_zero = vdup_n_u8(0);  // Zero for unpacking

    // Process 8 pixels at a time
    int i = *x;

    for (; i <= oWidth - 8; i += 8) {
        // Gather 8 pixels for each channel
        alignas(16) quint8 src_x_data[8], src_y_data[8], src_z_data[8], src_a_data[8];
        alignas(16) quint8 dst_x_data[8], dst_y_data[8], dst_z_data[8], dst_a_data[8];

        // Compute source and destination indices
        for (int j = 0; j < 8; ++j) {
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

        // Load gathered data into NEON registers
        uint8x8_t src_x = vld1_u8(src_x_data);
        uint8x8_t src_y = vld1_u8(src_y_data);
        uint8x8_t src_z = vld1_u8(src_z_data);
        uint8x8_t src_a = vld1_u8(src_a_data);

        uint8x8_t dst_x = vld1_u8(dst_x_data);
        uint8x8_t dst_y = vld1_u8(dst_y_data);
        uint8x8_t dst_z = vld1_u8(dst_z_data);
        uint8x8_t dst_a = vld1_u8(dst_a_data);

        // Compute complement alpha: (255 - src_a)
        uint8x8_t complement_a = vsub_u8(c_255, src_a);

        // Unpack to 16-bit for multiplication
        uint16x8_t src_a_lo = vmovl_u8(src_a);
        uint16x8_t comp_a_lo = vmovl_u8(complement_a);

        // --- X channel blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        uint16x8_t src_x_lo = vmovl_u8(src_x);
        uint16x8_t dst_x_lo = vmovl_u8(dst_x);
        uint16x8_t src_x_prod = vmulq_u16(src_x_lo, src_a_lo);
        uint16x8_t dst_x_prod = vmulq_u16(dst_x_lo, comp_a_lo);
        uint16x8_t sum_x = vaddq_u16(src_x_prod, dst_x_prod);
        sum_x = vshrq_n_u16(sum_x, 8); // Divide by 256
        uint8x8_t result_x = vqmovn_u16(sum_x);

        // --- Y channel blending: (src_y * src_a + dst_y * (255 - src_a)) / 255 ---
        uint16x8_t src_y_lo = vmovl_u8(src_y);
        uint16x8_t dst_y_lo = vmovl_u8(dst_y);
        uint16x8_t src_y_prod = vmulq_u16(src_y_lo, src_a_lo);
        uint16x8_t dst_y_prod = vmulq_u16(dst_y_lo, comp_a_lo);
        uint16x8_t sum_y = vaddq_u16(src_y_prod, dst_y_prod);
        sum_y = vshrq_n_u16(sum_y, 8);
        uint8x8_t result_y = vqmovn_u16(sum_y);

        // --- Z channel blending: (src_z * src_a + dst_z * (255 - src_a)) / 255 ---
        uint16x8_t src_z_lo = vmovl_u8(src_z);
        uint16x8_t dst_z_lo = vmovl_u8(dst_z);
        uint16x8_t src_z_prod = vmulq_u16(src_z_lo, src_a_lo);
        uint16x8_t dst_z_prod = vmulq_u16(dst_z_lo, comp_a_lo);
        uint16x8_t sum_z = vaddq_u16(src_z_prod, dst_z_prod);
        sum_z = vshrq_n_u16(sum_z, 8);
        uint8x8_t result_z = vqmovn_u16(sum_z);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        uint16x8_t dst_a_lo = vmovl_u8(dst_a);
        uint16x8_t alpha_prod = vmulq_u16(dst_a_lo, comp_a_lo);
        alpha_prod = vshrq_n_u16(alpha_prod, 8);
        uint16x8_t alpha_sum = vaddq_u16(alpha_prod, src_a_lo);
        uint8x8_t result_a = vqmovn_u16(alpha_sum);

        // Store results
        alignas(16) quint8 result_x_data[8], result_y_data[8], result_z_data[8], result_a_data[8];
        vst1_u8(result_x_data, result_x);
        vst1_u8(result_y_data, result_y);
        vst1_u8(result_z_data, result_z);
        vst1_u8(result_a_data, result_a);

        for (int j = 0; j < 8; ++j) {
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

void SimdCoreNEONPrivate::drawFastLc8bits1A(int oWidth,
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
    // Number of pixels processed per iteration (16 pixels with NEON)
    const int pixels_per_iteration = 16;
    int i = *x;

    // Process pixels in chunks of 16 using NEON
    while (i + pixels_per_iteration <= oWidth) {
        // Calculate source and destination offsets for 16 pixels
        alignas(16) uint8_t src_x_data[16], src_a_data[16], dst_x_data[16], dst_a_data[16];

        for (int j = 0; j < pixels_per_iteration; ++j) {
            int xs = ((i + j) * iDiffX + oMultX) / oDiffX;
            src_x_data[j] = src_line_x[(xs >> xiWidthDiv) * xiStep];
            src_a_data[j] = src_line_a[(xs >> aiWidthDiv) * aiStep];
            dst_x_data[j] = dst_line_x[((i + j) >> xiWidthDiv) * xiStep];
            dst_a_data[j] = dst_line_a[((i + j) >> aiWidthDiv) * aiStep];
        }

        // Load 16 pixels into NEON registers
        uint8x16_t src_x = vld1q_u8(src_x_data); // Load grayscale source
        uint8x16_t src_a = vld1q_u8(src_a_data); // Load alpha source
        uint8x16_t dst_x = vld1q_u8(dst_x_data); // Load grayscale destination
        uint8x16_t dst_a = vld1q_u8(dst_a_data); // Load alpha destination

        // Compute complement alpha: (255 - src_a)
        uint8x16_t const_255 = vdupq_n_u8(255);
        uint8x16_t complement_a = vsubq_u8(const_255, src_a);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Widen to 16-bit to avoid overflow (process low and high 8 pixels separately)
        uint16x8_t src_x_lo = vmovl_u8(vget_low_u8(src_x));
        uint16x8_t src_x_hi = vmovl_u8(vget_high_u8(src_x));
        uint16x8_t src_a_lo = vmovl_u8(vget_low_u8(src_a));
        uint16x8_t src_a_hi = vmovl_u8(vget_high_u8(src_a));
        uint16x8_t dst_x_lo = vmovl_u8(vget_low_u8(dst_x));
        uint16x8_t dst_x_hi = vmovl_u8(vget_high_u8(dst_x));
        uint16x8_t comp_a_lo = vmovl_u8(vget_low_u8(complement_a));
        uint16x8_t comp_a_hi = vmovl_u8(vget_high_u8(complement_a));

        // Compute src_x * src_a
        uint16x8_t src_prod_lo = vmulq_u16(src_x_lo, src_a_lo);
        uint16x8_t src_prod_hi = vmulq_u16(src_x_hi, src_a_hi);

        // Compute dst_x * (255 - src_a)
        uint16x8_t dst_prod_lo = vmulq_u16(dst_x_lo, comp_a_lo);
        uint16x8_t dst_prod_hi = vmulq_u16(dst_x_hi, comp_a_hi);

        // Sum products
        uint16x8_t sum_lo = vaddq_u16(src_prod_lo, dst_prod_lo);
        uint16x8_t sum_hi = vaddq_u16(src_prod_hi, dst_prod_hi);

        // Divide by 256 (approximates /255) using right shift
        sum_lo = vshrq_n_u16(sum_lo, 8);
        sum_hi = vshrq_n_u16(sum_hi, 8);

        // Narrow back to 8-bit
        uint8x16_t result_x = vcombine_u8(vmovn_u16(sum_lo), vmovn_u16(sum_hi));

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Widen dst_a
        uint16x8_t dst_a_lo = vmovl_u8(vget_low_u8(dst_a));
        uint16x8_t dst_a_hi = vmovl_u8(vget_high_u8(dst_a));

        // Compute dst_a * (255 - src_a)
        uint16x8_t alpha_prod_lo = vmulq_u16(dst_a_lo, comp_a_lo);
        uint16x8_t alpha_prod_hi = vmulq_u16(dst_a_hi, comp_a_hi);

        // Divide by 256
        alpha_prod_lo = vshrq_n_u16(alpha_prod_lo, 8);
        alpha_prod_hi = vshrq_n_u16(alpha_prod_hi, 8);

        // Add src_a
        alpha_prod_lo = vaddq_u16(alpha_prod_lo, src_a_lo);
        alpha_prod_hi = vaddq_u16(alpha_prod_hi, src_a_hi);

        // Narrow back to 8-bit
        uint8x16_t result_a = vcombine_u8(vmovn_u16(alpha_prod_lo), vmovn_u16(alpha_prod_hi));

        // Store results
        alignas(16) uint8_t result_x_data[16], result_a_data[16];
        vst1q_u8(result_x_data, result_x);
        vst1q_u8(result_a_data, result_a);

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

#include "moc_simdcoreneon.cpp"
