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
    CHECK_FUNCTION(drawFast8bits1A)
    CHECK_FUNCTION(drawFast8bits3A)
    CHECK_FUNCTION(drawFastLc8bits1A)
    CHECK_FUNCTION(drawFastLc8bits3A)

    return nullptr;
}

void SimdCoreRVVPrivate::drawFast8bits3A(int oWidth,
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
    const int simd_width = 8; // Process 8 pixels per RVV iteration
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

    // Set vector length for 8 uint8_t elements (LMUL=1)
    size_t vl = __riscv_vsetvl_e8m1(simd_width);

    for (; i <= oWidth - simd_width; i += vl) {
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

        // Load data into RVV registers
        vuint8m1_t src_x = __riscv_vle8_v_u8m1(src_x_data, vl);
        vuint8m1_t src_y = __riscv_vle8_v_u8m1(src_y_data, vl);
        vuint8m1_t src_z = __riscv_vle8_v_u8m1(src_z_data, vl);
        vuint8m1_t src_a = __riscv_vle8_v_u8m1(src_a_data, vl);
        vuint8m1_t dst_x = __riscv_vle8_v_u8m1(dst_x_data, vl);
        vuint8m1_t dst_y = __riscv_vle8_v_u8m1(dst_y_data, vl);
        vuint8m1_t dst_z = __riscv_vle8_v_u8m1(dst_z_data, vl);
        vuint8m1_t dst_a = __riscv_vle8_v_u8m1(dst_a_data, vl);

        // Extend to 16-bit for arithmetic
        vuint16m2_t src_a_16 = __riscv_vzext_vf2_u16m2(src_a, vl); // uint8_t to uint16_t
        vuint16m2_t inv_a_16 = __riscv_vsub_vv_u16m2(__riscv_vmv_v_x_u16m2(255, vl), src_a_16, vl); // 255 - src_a

        // Process X channel
        vuint16m2_t src_x_16 = __riscv_vzext_vf2_u16m2(src_x, vl);
        vuint16m2_t dst_x_16 = __riscv_vzext_vf2_u16m2(dst_x, vl);
        vuint16m2_t result_x_16 = __riscv_vmul_vv_u16m2(src_x_16, src_a_16, vl); // src_x * src_a
        result_x_16 = __riscv_vadd_vv_u16m2(result_x_16, __riscv_vmul_vv_u16m2(dst_x_16, inv_a_16, vl), vl); // + dst_x * (255 - src_a)
        result_x_16 = __riscv_vdivu_vx_u16m2(result_x_16, 255, vl); // Divide by 255
        vuint8m1_t result_x = __riscv_vncvt_x_x_w_u8m1(result_x_16, vl);

        // Process Y channel
        vuint16m2_t src_y_16 = __riscv_vzext_vf2_u16m2(src_y, vl);
        vuint16m2_t dst_y_16 = __riscv_vzext_vf2_u16m2(dst_y, vl);
        vuint16m2_t result_y_16 = __riscv_vmul_vv_u16m2(src_y_16, src_a_16, vl);
        result_y_16 = __riscv_vadd_vv_u16m2(result_y_16, __riscv_vmul_vv_u16m2(dst_y_16, inv_a_16, vl), vl);
        result_y_16 = __riscv_vdivu_vx_u16m2(result_y_16, 255, vl);
        vuint8m1_t result_y = __riscv_vncvt_x_x_w_u8m1(result_y_16, vl);

        // Process Z channel
        vuint16m2_t src_z_16 = __riscv_vzext_vf2_u16m2(src_z, vl);
        vuint16m2_t dst_z_16 = __riscv_vzext_vf2_u16m2(dst_z, vl);
        vuint16m2_t result_z_16 = __riscv_vmul_vv_u16m2(src_z_16, src_a_16, vl);
        result_z_16 = __riscv_vadd_vv_u16m2(result_z_16, __riscv_vmul_vv_u16m2(dst_z_16, inv_a_16, vl), vl);
        result_z_16 = __riscv_vdivu_vx_u16m2(result_z_16, 255, vl);
        vuint8m1_t result_z = __riscv_vncvt_x_x_w_u8m1(result_z_16, vl);

        // Process A channel
        vuint16m2_t dst_a_16 = __riscv_vzext_vf2_u16m2(dst_a, vl);
        vuint16m2_t result_a_16 = __riscv_vmul_vv_u16m2(dst_a_16, inv_a_16, vl); // dst_a * (255 - src_a)
        result_a_16 = __riscv_vdivu_vx_u16m2(result_a_16, 255, vl); // Divide by 255
        result_a_16 = __riscv_vadd_vv_u16m2(result_a_16, src_a_16, vl); // + src_a
        vuint8m1_t result_a = __riscv_vncvt_x_x_w_u8m1(result_a_16, vl);

        // Store results to temporary buffers
        __riscv_vse8_v_u8m1(result_x_data, result_x, vl);
        __riscv_vse8_v_u8m1(result_y_data, result_y, vl);
        __riscv_vse8_v_u8m1(result_z_data, result_z, vl);
        __riscv_vse8_v_u8m1(result_a_data, result_a, vl);

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

void SimdCoreRVVPrivate::drawFast8bits1A(int oWidth,
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
    // Process variable-length vectors
    int i = *x;
    size_t vlmax = __riscv_vsetvlmax_e8m1(); // Maximum vector length for 8-bit elements
    alignas(16) uint8_t src_x_data[vlmax], src_a_data[vlmax];
    alignas(16) uint8_t dst_x_data[vlmax], dst_a_data[vlmax];
    alignas(16) uint8_t result_x_data[vlmax], result_a_data[vlmax];

    while (i < oWidth) {
        // Set vector length for remaining pixels
        size_t vl = __riscv_vsetvl_e8m1(oWidth - i);

        // Gather pixels using offset arrays
        for (size_t j = 0; j < vl; ++j) {
            src_x_data[j] = src_line_x[srcWidthOffsetX[i + j]];
            src_a_data[j] = src_line_a[srcWidthOffsetA[i + j]];
            dst_x_data[j] = dst_line_x[dstWidthOffsetX[i + j]];
            dst_a_data[j] = dst_line_a[dstWidthOffsetA[i + j]];
        }

        // Load gathered data into RVV registers
        vuint8m1_t src_x = __riscv_vle8_v_u8m1(src_x_data, vl);
        vuint8m1_t src_a = __riscv_vle8_v_u8m1(src_a_data, vl);
        vuint8m1_t dst_x = __riscv_vle8_v_u8m1(dst_x_data, vl);
        vuint8m1_t dst_a = __riscv_vle8_v_u8m1(dst_a_data, vl);

        // Compute complement alpha: (255 - src_a)
        vuint8m1_t complement_a = __riscv_vsub_vx_u8m1(src_a, 255, vl);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Widen to 16-bit to avoid overflow
        vuint16m2_t src_x_w = __riscv_vzext_vf2_u16m2(src_x, vl);
        vuint16m2_t src_a_w = __riscv_vzext_vf2_u16m2(src_a, vl);
        vuint16m2_t dst_x_w = __riscv_vzext_vf2_u16m2(dst_x, vl);
        vuint16m2_t comp_a_w = __riscv_vzext_vf2_u16m2(complement_a, vl);

        // Compute src_x * src_a
        vuint16m2_t src_prod = __riscv_vmul_vv_u16m2(src_x_w, src_a_w, vl);

        // Compute dst_x * (255 - src_a)
        vuint16m2_t dst_prod = __riscv_vmul_vv_u16m2(dst_x_w, comp_a_w, vl);

        // Sum products
        vuint16m2_t sum = __riscv_vadd_vv_u16m2(src_prod, dst_prod, vl);

        // Divide by 256 (approximates /255)
        sum = __riscv_vsrl_vx_u16m2(sum, 8, vl);

        // Narrow back to 8-bit
        vuint8m1_t result_x = __riscv_vncvt_x_x_w_u8m1(sum, vl); // Corrección aquí

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Widen dst_a and complement_a
        vuint16m2_t dst_a_w = __riscv_vzext_vf2_u16m2(dst_a, vl);

        // Compute dst_a * (255 - src_a)
        vuint16m2_t alpha_prod = __riscv_vmul_vv_u16m2(dst_a_w, comp_a_w, vl);

        // Divide by 256
        alpha_prod = __riscv_vsrl_vx_u16m2(alpha_prod, 8, vl);

        // Add src_a
        vuint16m2_t alpha_sum = __riscv_vadd_vv_u16m2(alpha_prod, src_a_w, vl);

        // Narrow back to 8-bit
        vuint8m1_t result_a = __riscv_vncvt_x_x_w_u8m1(alpha_sum, vl); // Corrección aquí

        // Store results
        __riscv_vse8_v_u8m1(result_x_data, result_x, vl);
        __riscv_vse8_v_u8m1(result_a_data, result_a, vl);

        // Scatter results using offset arrays
        for (size_t j = 0; j < vl; ++j) {
            dst_line_x[dstWidthOffsetX[i + j]] = result_x_data[j];
            dst_line_a[dstWidthOffsetA[i + j]] = result_a_data[j];
        }

        // Advance index
        i += vl;
    }

    // Update x for scalar fallback
    *x = i;
}

void SimdCoreRVVPrivate::drawFastLc8bits3A(int oWidth,
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
    // Process variable-length vectors
    int i = *x;
    size_t vlmax = __riscv_vsetvlmax_e8m1(); // Maximum vector length for 8-bit elements
    alignas(16) uint8_t src_x_data[vlmax], src_y_data[vlmax], src_z_data[vlmax], src_a_data[vlmax];
    alignas(16) uint8_t dst_x_data[vlmax], dst_y_data[vlmax], dst_z_data[vlmax], dst_a_data[vlmax];
    alignas(16) uint8_t result_x_data[vlmax], result_y_data[vlmax], result_z_data[vlmax], result_a_data[vlmax];

    while (i < oWidth) {
        // Set vector length for remaining pixels
        size_t vl = __riscv_vsetvl_e8m1(oWidth - i);

        // Gather pixels using scaling parameters
        for (size_t j = 0; j < vl; ++j) {
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

        // Load gathered data into RVV registers
        vuint8m1_t src_x = __riscv_vle8_v_u8m1(src_x_data, vl);
        vuint8m1_t src_y = __riscv_vle8_v_u8m1(src_y_data, vl);
        vuint8m1_t src_z = __riscv_vle8_v_u8m1(src_z_data, vl);
        vuint8m1_t src_a = __riscv_vle8_v_u8m1(src_a_data, vl);

        vuint8m1_t dst_x = __riscv_vle8_v_u8m1(dst_x_data, vl);
        vuint8m1_t dst_y = __riscv_vle8_v_u8m1(dst_y_data, vl);
        vuint8m1_t dst_z = __riscv_vle8_v_u8m1(dst_z_data, vl);
        vuint8m1_t dst_a = __riscv_vle8_v_u8m1(dst_a_data, vl);

        // Compute complement alpha: (255 - src_a)
        vuint8m1_t complement_a = __riscv_vsub_vx_u8m1(src_a, 255, vl);

        // Widen to 16-bit for multiplication
        vuint16m2_t src_a_w = __riscv_vzext_vf2_u16m2(src_a, vl);
        vuint16m2_t comp_a_w = __riscv_vzext_vf2_u16m2(complement_a, vl);

        // --- X channel blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        vuint16m2_t src_x_w = __riscv_vzext_vf2_u16m2(src_x, vl);
        vuint16m2_t dst_x_w = __riscv_vzext_vf2_u16m2(dst_x, vl);
        vuint16m2_t src_x_prod = __riscv_vmul_vv_u16m2(src_x_w, src_a_w, vl);
        vuint16m2_t dst_x_prod = __riscv_vmul_vv_u16m2(dst_x_w, comp_a_w, vl);
        vuint16m2_t sum_x = __riscv_vadd_vv_u16m2(src_x_prod, dst_x_prod, vl);
        sum_x = __riscv_vsrl_vx_u16m2(sum_x, 8, vl);
        vuint8m1_t result_x = __riscv_vnsrl_wx_u8m1(sum_x, 0, vl); // Narrow with no shift

        // --- Y channel blending: (src_y * src_a + dst_y * (255 - src_a)) / 255 ---
        vuint16m2_t src_y_w = __riscv_vzext_vf2_u16m2(src_y, vl);
        vuint16m2_t dst_y_w = __riscv_vzext_vf2_u16m2(dst_y, vl);
        vuint16m2_t src_y_prod = __riscv_vmul_vv_u16m2(src_y_w, src_a_w, vl);
        vuint16m2_t dst_y_prod = __riscv_vmul_vv_u16m2(dst_y_w, comp_a_w, vl);
        vuint16m2_t sum_y = __riscv_vadd_vv_u16m2(src_y_prod, dst_y_prod, vl);
        sum_y = __riscv_vsrl_vx_u16m2(sum_y, 8, vl);
        vuint8m1_t result_y = __riscv_vnsrl_wx_u8m1(sum_y, 0, vl);

        // --- Z channel blending: (src_z * src_a + dst_z * (255 - src_a)) / 255 ---
        vuint16m2_t src_z_w = __riscv_vzext_vf2_u16m2(src_z, vl);
        vuint16m2_t dst_z_w = __riscv_vzext_vf2_u16m2(dst_z, vl);
        vuint16m2_t src_z_prod = __riscv_vmul_vv_u16m2(src_z_w, src_a_w, vl);
        vuint16m2_t dst_z_prod = __riscv_vmul_vv_u16m2(dst_z_w, comp_a_w, vl);
        vuint16m2_t sum_z = __riscv_vadd_vv_u16m2(src_z_prod, dst_z_prod, vl);
        sum_z = __riscv_vsrl_vx_u16m2(sum_z, 8, vl);
        vuint8m1_t result_z = __riscv_vnsrl_wx_u8m1(sum_z, 0, vl);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        vuint16m2_t dst_a_w = __riscv_vzext_vf2_u16m2(dst_a, vl);
        vuint16m2_t alpha_prod = __riscv_vmul_vv_u16m2(dst_a_w, comp_a_w, vl);
        alpha_prod = __riscv_vsrl_vx_u16m2(alpha_prod, 8, vl);
        vuint16m2_t alpha_sum = __riscv_vadd_vv_u16m2(alpha_prod, src_a_w, vl);
        vuint8m1_t result_a = __riscv_vnsrl_wx_u8m1(alpha_sum, 0, vl);

        // Store results
        __riscv_vse8_v_u8m1(result_x_data, result_x, vl);
        __riscv_vse8_v_u8m1(result_y_data, result_y, vl);
        __riscv_vse8_v_u8m1(result_z_data, result_z, vl);
        __riscv_vse8_v_u8m1(result_a_data, result_a, vl);

        // Scatter results using offsets
        for (size_t j = 0; j < vl; ++j) {
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

        // Advance index
        i += vl;
    }

    // Update x for scalar fallback
    *x = i;
}

void SimdCoreRVVPrivate::drawFastLc8bits1A(int oWidth,
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
    int i = *x;
    // Get maximum vector length for 8-bit elements
    size_t vlmax = __riscv_vsetvlmax_e8m1();
    // Allocate temporary arrays for gather/scatter
    alignas(16) uint8_t src_x_data[vlmax], src_a_data[vlmax];
    alignas(16) uint8_t dst_x_data[vlmax], dst_a_data[vlmax];
    alignas(16) uint8_t result_x_data[vlmax], result_a_data[vlmax];

    // Process pixels in chunks based on vector length
    while (i < oWidth) {
        // Set vector length for remaining pixels
        size_t vl = __riscv_vsetvl_e8m1(oWidth - i);

        // Calculate source and destination offsets and gather data
        for (size_t j = 0; j < vl; ++j) {
            int xs = ((i + j) * iDiffX + oMultX) / oDiffX;
            src_x_data[j] = src_line_x[(xs >> xiWidthDiv) * xiStep];
            src_a_data[j] = src_line_a[(xs >> aiWidthDiv) * aiStep];
            dst_x_data[j] = dst_line_x[((i + j) >> xiWidthDiv) * xiStep];
            dst_a_data[j] = dst_line_a[((i + j) >> aiWidthDiv) * aiStep];
        }

        // Load gathered data into RVV registers
        vuint8m1_t src_x = __riscv_vle8_v_u8m1(src_x_data, vl); // Load grayscale source
        vuint8m1_t src_a = __riscv_vle8_v_u8m1(src_a_data, vl); // Load alpha source
        vuint8m1_t dst_x = __riscv_vle8_v_u8m1(dst_x_data, vl); // Load grayscale destination
        vuint8m1_t dst_a = __riscv_vle8_v_u8m1(dst_a_data, vl); // Load alpha destination

        // Compute complement alpha: (255 - src_a)
        vuint8m1_t complement_a = __riscv_vsub_vx_u8m1(src_a, 255, vl);

        // --- Grayscale blending: (src_x * src_a + dst_x * (255 - src_a)) / 255 ---
        // Widen to 16-bit to avoid overflow
        vuint16m2_t src_x_w = __riscv_vzext_vf2_u16m2(src_x, vl);
        vuint16m2_t src_a_w = __riscv_vzext_vf2_u16m2(src_a, vl);
        vuint16m2_t dst_x_w = __riscv_vzext_vf2_u16m2(dst_x, vl);
        vuint16m2_t comp_a_w = __riscv_vzext_vf2_u16m2(complement_a, vl);

        // Compute src_x * src_a
        vuint16m2_t src_prod = __riscv_vmul_vv_u16m2(src_x_w, src_a_w, vl);

        // Compute dst_x * (255 - src_a)
        vuint16m2_t dst_prod = __riscv_vmul_vv_u16m2(dst_x_w, comp_a_w, vl);

        // Sum products
        vuint16m2_t sum = __riscv_vadd_vv_u16m2(src_prod, dst_prod, vl);

        // Divide by 256 (approximates /255)
        sum = __riscv_vsrl_vx_u16m2(sum, 8, vl);

        // Narrow back to 8-bit
        vuint8m1_t result_x = __riscv_vncvt_x_x_w_u8m1(sum, vl);

        // --- Alpha blending: src_a + dst_a * (255 - src_a) / 255 ---
        // Widen dst_a
        vuint16m2_t dst_a_w = __riscv_vzext_vf2_u16m2(dst_a, vl);

        // Compute dst_a * (255 - src_a)
        vuint16m2_t alpha_prod = __riscv_vmul_vv_u16m2(dst_a_w, comp_a_w, vl);

        // Divide by 256
        alpha_prod = __riscv_vsrl_vx_u16m2(alpha_prod, 8, vl);

        // Add src_a
        vuint16m2_t alpha_sum = __riscv_vadd_vv_u16m2(alpha_prod, src_a_w, vl);

        // Narrow back to 8-bit
        vuint8m1_t result_a = __riscv_vncvt_x_x_w_u8m1(alpha_sum, vl);

        // Store results
        __riscv_vse8_v_u8m1(result_x_data, result_x, vl);
        __riscv_vse8_v_u8m1(result_a_data, result_a, vl);

        // Scatter results back to destination
        for (size_t j = 0; j < vl; ++j) {
            dst_line_x[((i + j) >> xiWidthDiv) * xiStep] = result_x_data[j];
            dst_line_a[((i + j) >> aiWidthDiv) * aiStep] = result_a_data[j];
        }

        // Advance index
        i += vl;
    }

    // Update x for scalar fallback
    *x = i;
}

#include "moc_simdcorervv.cpp"
