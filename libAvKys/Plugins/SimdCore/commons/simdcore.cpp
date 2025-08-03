/* Webcamoid, camera capture application.
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

#ifdef OPENMP_ENABLED
#include <omp.h>
#endif

#include "simdcore.h"

#ifdef AKSIMD_USE_MMX
        #include <simd/akmmx.h>

        using SimdType = AkSimdMMXI32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDMMXI32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDMMXI32_ALIGN
#elif defined(AKSIMD_USE_SSE)
        #include <simd/aksse.h>

        using SimdType = AkSimdSSEF32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDSSEF32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDSSEF32_ALIGN
#elif defined(AKSIMD_USE_SSE2)
        #include <simd/aksse2.h>

        using SimdType = AkSimdSSE2I32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDSSE2I32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDSSE2I32_ALIGN
#elif defined(AKSIMD_USE_SSE4_1)
        #include <simd/aksse4_1.h>

        using SimdType = AkSimdSSE4_1I32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDSSE4_1I32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDSSE4_1I32_ALIGN
#elif defined(AKSIMD_USE_AVX)
        #include <simd/akavx.h>

        using SimdType = AkSimdAVXF32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDAVXF32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDAVXF32_ALIGN

#elif defined(AKSIMD_USE_AVX2)
        #include <simd/akavx2.h>

        using SimdType = AkSimdAVX2I32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDAVX2I32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDAVX2I32_ALIGN
#elif defined(AKSIMD_USE_NEON)
        #include <simd/akneon.h>
    #if 0
        using SimdType = AkSimdNEONI32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDNEONI32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDNEONI32_ALIGN
    #else
        using SimdType = AkSimdNEONF32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDNEONF32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDNEONF32_ALIGN
    #endif
#elif defined(AKSIMD_USE_SVE)
        #include <simd/aksve.h>

        using SimdType = AkSimdSVEI32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDSVEI32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDSVEI32_ALIGN
#elif defined(AKSIMD_USE_RVV)
        #include <simd/akrvv.h>

        using SimdType = AkSimdRVVI32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDRVVI32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDRVVI32_ALIGN
#else
        #include <simd/akscalar.h>

        using SimdType = AkSimdScalarI32;
        using VectorType = SimdType::VectorType;
        using NativeType = SimdType::NativeType;

        #define SIMD_DEFAULT_SIZE AKSIMDSCALARI32_DEFAULT_SIZE
        #define SIMD_ALIGN        AKSIMDSCALARI32_ALIGN
#endif

class DrawParameters
{
    public:
        SimdType simd;

        DrawParameters()
        {

        }
};

class ConvertParameters
{
    public:
        SimdType simd;
        qint64 m[12];
        qint64 am[9];
        qint64 vmin[3];
        qint64 vmax[3];
        size_t colorShift;
        size_t alphaShift;

        ConvertParameters()
        {

        }

        ConvertParameters(qint64 *colorMatrix,
                          qint64 *alphaMatrix,
                          qint64 *minValues,
                          qint64 *maxValues,
                          qint64 colorShift,
                          qint64 alphaShift)
        {
            memcpy(this->m, colorMatrix, 12 * sizeof(qint64));
            memcpy(this->am, alphaMatrix, 9 * sizeof(qint64));
            memcpy(this->vmin, minValues, 3 * sizeof(qint64));
            memcpy(this->vmax, maxValues, 3 * sizeof(qint64));

            this->colorShift = colorShift;
            this->alphaShift = alphaShift;
        }

#define M(index) \
    s.load(static_cast<NativeType>(this->m[index]))

#define AM(index) \
    s.load(static_cast<NativeType>(this->am[index]))

#define VMIN(index) \
    s.load(static_cast<NativeType>(this->vmin[index]))

#define VMAX(index) \
    s.load(static_cast<NativeType>(this->vmax[index]))

        inline void applyMatrix(VectorType a, VectorType b, VectorType c,
                                VectorType *x, VectorType *y, VectorType *z) const
        {
            auto &s = this->simd;

            // Apply matrix

            *x = s.shr(s.add(s.add(s.add(s.mul(a, M(0)), s.mul(b, M(1))), s.mul(c, M(2 ))), M(3 )), this->colorShift);
            *y = s.shr(s.add(s.add(s.add(s.mul(a, M(4)), s.mul(b, M(5))), s.mul(c, M(6 ))), M(7 )), this->colorShift);
            *z = s.shr(s.add(s.add(s.add(s.mul(a, M(8)), s.mul(b, M(9))), s.mul(c, M(10))), M(11)), this->colorShift);

            // Clamp values

            *x = s.bound(VMIN(0), *x, VMAX(0));
            *y = s.bound(VMIN(1), *y, VMAX(1));
            *z = s.bound(VMIN(2), *z, VMAX(2));
        }

        inline void applyPoint(VectorType p,
                               VectorType *x, VectorType *y, VectorType *z) const
        {
            auto &s = this->simd;

            // Apply point

            *x = s.shr(s.add(s.mul(p, M(0)), M(3 )), this->colorShift);
            *y = s.shr(s.add(s.mul(p, M(4)), M(7 )), this->colorShift);
            *z = s.shr(s.add(s.mul(p, M(8)), M(11)), this->colorShift);
        }

        inline void applyPoint(VectorType a, VectorType b, VectorType c,
                               VectorType *p) const
        {
            auto &s = this->simd;

            // Apply point

            *p = s.shr(s.add(s.add(s.add(s.mul(a, M(0)), s.mul(b, M(1))), s.mul(c, M(2))), M(3)), this->colorShift);

            // Clamp value

            *p = s.bound(VMIN(0), *p, VMAX(0));
        }

        inline void applyAlpha(VectorType x, VectorType y, VectorType z, VectorType a,
                               VectorType *xa, VectorType *ya,VectorType *za) const
        {
            auto &s = this->simd;

            // Apply alpha

            *xa = s.shr(s.add(s.mul(a, s.add(s.mul(x, AM(0)), AM(1))), AM(2)), this->alphaShift);
            *ya = s.shr(s.add(s.mul(a, s.add(s.mul(y, AM(3)), AM(4))), AM(5)), this->alphaShift);
            *za = s.shr(s.add(s.mul(a, s.add(s.mul(z, AM(6)), AM(7))), AM(8)), this->alphaShift);

            // Clamp values

            *xa = s.bound(VMIN(0), *xa, VMAX(0));
            *ya = s.bound(VMIN(1), *ya, VMAX(1));
            *za = s.bound(VMIN(2), *za, VMAX(2));
        }

        inline void applyAlpha(VectorType a,
                               VectorType *x, VectorType *y, VectorType *z) const
        {
            this->applyAlpha(*x, *y, *z, a, x, y, z);
        }

        inline void applyAlpha(VectorType p, VectorType a, VectorType *pa) const
        {
            auto &s = this->simd;

            // Apply alpha

            *pa = s.shr(s.add(s.mul(a, s.add(s.mul(p, AM(0)), AM(1))), AM(2)), this->alphaShift);

            // Clamp value

            *pa = s.bound(VMIN(0), *pa, VMAX(0));
        }

        inline void applyAlpha(VectorType a, VectorType *p) const
        {
            this->applyAlpha(*p, a, p);
        }
};

class SimdCorePrivate
{
    public:
        // Optimized draw functions

        static void *createDrawParameters();
        static void freeDrawParameters(void *drawParameters);
        static void drawFast8bits3A(void *drawParameters,
                                    int oWidth,
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
        static void drawFast8bits1A(void *drawParameters,
                                    int oWidth,
                                    const int *srcWidthOffsetX,
                                    const int *srcWidthOffsetA,
                                    const int *dstWidthOffsetX,
                                    const int *dstWidthOffsetA,
                                    const quint8 *src_line_x,
                                    const quint8 *src_line_a,
                                    quint8 *dst_line_x,
                                    quint8 *dst_line_a,
                                    int *x);
        static void drawFastLc8bits3A(void *drawParameters,
                                      int oWidth,
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
        static void drawFastLc8bits1A(void *drawParameters,
                                      int oWidth,
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

        // Optimized convert functions

        static void *createConvertParameters(qint64 *colorMatrix,
                                             qint64 *alphaMatrix,
                                             qint64 *minValues,
                                             qint64 *maxValues,
                                             qint64 colorShift,
                                             qint64 alphaShift);
        static void freeConvertParameters(void *convertParameters);
        static void convertFast8bits3to3(void *convertParameters,
                                         const int *srcWidthOffsetX,
                                         const int *srcWidthOffsetY,
                                         const int *srcWidthOffsetZ,
                                         const int *dstWidthOffsetX,
                                         const int *dstWidthOffsetY,
                                         const int *dstWidthOffsetZ,
                                         int xmax,
                                         const quint8 *src_line_x,
                                         const quint8 *src_line_y,
                                         const quint8 *src_line_z,
                                         quint8 *dst_line_x,
                                         quint8 *dst_line_y,
                                         quint8 *dst_line_z,
                                         int *x);
        static void convertFast8bits3to3A(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetY,
                                          const int *srcWidthOffsetZ,
                                          const int *dstWidthOffsetX,
                                          const int *dstWidthOffsetY,
                                          const int *dstWidthOffsetZ,
                                          const int *dstWidthOffsetA,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_y,
                                          const quint8 *src_line_z,
                                          quint8 *dst_line_x,
                                          quint8 *dst_line_y,
                                          quint8 *dst_line_z,
                                          quint8 *dst_line_a,
                                          int *x);
        static void convertFast8bits3Ato3(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetY,
                                          const int *srcWidthOffsetZ,
                                          const int *srcWidthOffsetA,
                                          const int *dstWidthOffsetX,
                                          const int *dstWidthOffsetY,
                                          const int *dstWidthOffsetZ,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_y,
                                          const quint8 *src_line_z,
                                          const quint8 *src_line_a,
                                          quint8 *dst_line_x,
                                          quint8 *dst_line_y,
                                          quint8 *dst_line_z,
                                          int *x);
        static void convertFast8bits3Ato3A(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetY,
                                           const int *srcWidthOffsetZ,
                                           const int *srcWidthOffsetA,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetY,
                                           const int *dstWidthOffsetZ,
                                           const int *dstWidthOffsetA,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           quint8 *dst_line_a,
                                           int *x);
        static void convertFast8bitsV3Ato3(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetY,
                                           const int *srcWidthOffsetZ,
                                           const int *srcWidthOffsetA,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetY,
                                           const int *dstWidthOffsetZ,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           int *x);
        static void convertFast8bits3to1(void *convertParameters,
                                         const int *srcWidthOffsetX,
                                         const int *srcWidthOffsetY,
                                         const int *srcWidthOffsetZ,
                                         const int *dstWidthOffsetX,
                                         int xmax,
                                         const quint8 *src_line_x,
                                         const quint8 *src_line_y,
                                         const quint8 *src_line_z,
                                         quint8 *dst_line_x,
                                         int *x);
        static void convertFast8bits3to1A(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetY,
                                          const int *srcWidthOffsetZ,
                                          const int *dstWidthOffsetX,
                                          const int *dstWidthOffsetA,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_y,
                                          const quint8 *src_line_z,
                                          quint8 *dst_line_x,
                                          quint8 *dst_line_a,
                                          int *x);
        static void convertFast8bits3Ato1(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetY,
                                          const int *srcWidthOffsetZ,
                                          const int *srcWidthOffsetA,
                                          const int *dstWidthOffsetX,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_y,
                                          const quint8 *src_line_z,
                                          const quint8 *src_line_a,
                                          quint8 *dst_line_x,
                                          int *x);
        static void convertFast8bits3Ato1A(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetY,
                                           const int *srcWidthOffsetZ,
                                           const int *srcWidthOffsetA,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetA,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_a,
                                           int *x);
        static void convertFast8bits1to3(void *convertParameters,
                                         const int *srcWidthOffsetX,
                                         const int *dstWidthOffsetX,
                                         const int *dstWidthOffsetY,
                                         const int *dstWidthOffsetZ,
                                         int xmax,
                                         const quint8 *src_line_x,
                                         quint8 *dst_line_x,
                                         quint8 *dst_line_y,
                                         quint8 *dst_line_z,
                                         int *x);
        static void convertFast8bits1to3A(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *dstWidthOffsetX,
                                          const int *dstWidthOffsetY,
                                          const int *dstWidthOffsetZ,
                                          const int *dstWidthOffsetA,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          quint8 *dst_line_x,
                                          quint8 *dst_line_y,
                                          quint8 *dst_line_z,
                                          quint8 *dst_line_a,
                                          int *x);
        static void convertFast8bits1Ato3(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetA,
                                          const int *dstWidthOffsetX,
                                          const int *dstWidthOffsetY,
                                          const int *dstWidthOffsetZ,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_a,
                                          quint8 *dst_line_x,
                                          quint8 *dst_line_y,
                                          quint8 *dst_line_z,
                                          int *x);
        static void convertFast8bits1Ato3A(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetA,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetY,
                                           const int *dstWidthOffsetZ,
                                           const int *dstWidthOffsetA,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_a,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           quint8 *dst_line_a,
                                           int *x);
        static void convertFast8bits1Ato1(void *convertParameters,
                                          const int *srcWidthOffsetX,
                                          const int *srcWidthOffsetA,
                                          const int *dstWidthOffsetX,
                                          int xmax,
                                          const quint8 *src_line_x,
                                          const quint8 *src_line_a,
                                          quint8 *dst_line_x,
                                          int *x);
};

SimdCore::SimdCore(QObject *parent):
    AkSimdOptimizations(parent)
{
    this->d = new SimdCorePrivate;
}

SimdCore::~SimdCore()
{
    delete this->d;
}

#define CHECK_FUNCTION(func) \
    if (strncmp(functionName, #func, 1024) == 0) \
        return reinterpret_cast<QFunctionPointer>(SimdCorePrivate::func);

QFunctionPointer SimdCore::resolve(const char *functionName) const
{
    // Optimized draw functions

    CHECK_FUNCTION(createDrawParameters)
    CHECK_FUNCTION(freeDrawParameters)
    CHECK_FUNCTION(drawFast8bits1A)
    CHECK_FUNCTION(drawFast8bits3A)
    CHECK_FUNCTION(drawFastLc8bits1A)
    CHECK_FUNCTION(drawFastLc8bits3A)

    // Optimized convert functions

    CHECK_FUNCTION(createConvertParameters)
    CHECK_FUNCTION(freeConvertParameters)
    CHECK_FUNCTION(convertFast8bits3to3)
    CHECK_FUNCTION(convertFast8bits3to3A)
    CHECK_FUNCTION(convertFast8bits3Ato3)
    CHECK_FUNCTION(convertFast8bits3Ato3A)
    CHECK_FUNCTION(convertFast8bitsV3Ato3)
    CHECK_FUNCTION(convertFast8bits3to1)
    CHECK_FUNCTION(convertFast8bits3to1A)
    CHECK_FUNCTION(convertFast8bits3Ato1)
    CHECK_FUNCTION(convertFast8bits3Ato1A)
    CHECK_FUNCTION(convertFast8bits1to3)
    CHECK_FUNCTION(convertFast8bits1to3A)
    CHECK_FUNCTION(convertFast8bits1Ato3)
    CHECK_FUNCTION(convertFast8bits1Ato3A)
    CHECK_FUNCTION(convertFast8bits1Ato1)

    return nullptr;
}

void *SimdCorePrivate::createDrawParameters()
{

    return new DrawParameters;
}

void SimdCorePrivate::freeDrawParameters(void *drawParameters)
{
    if (drawParameters)
        delete reinterpret_cast<DrawParameters *>(drawParameters);
}

void SimdCorePrivate::drawFast8bits3A(void *drawParameters,
                                      int oWidth,
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
    auto params = reinterpret_cast<DrawParameters *>(drawParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(oWidth - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= oWidth - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ao_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];

            xo_data[i] = dst_line_x[dstWidthOffsetX[xoff]];
            yo_data[i] = dst_line_y[dstWidthOffsetY[xoff]];
            zo_data[i] = dst_line_z[dstWidthOffsetZ[xoff]];
            ao_data[i] = dst_line_a[dstWidthOffsetA[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);
        auto ai = s.load(ai_data);

        auto xo = s.load(xo_data);
        auto yo = s.load(yo_data);
        auto zo = s.load(zo_data);
        auto ao = s.load(ao_data);

        // Alpha blend

        auto aiMult = s.mul(ai, NativeType(255));
        auto aoMult = s.mul(ao, s.sub(s.load(NativeType(255)), ai));

        auto a = s.add(aiMult, aoMult);
        xo = s.sdiv(s.add(s.mul(xi, aiMult), s.mul(xo, aoMult)), a);
        yo = s.sdiv(s.add(s.mul(yi, aiMult), s.mul(yo, aoMult)), a);
        zo = s.sdiv(s.add(s.mul(zi, aiMult), s.mul(zo, aoMult)), a);
        ao = s.div(a, NativeType(255));

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);
        s.store(ao_data, ao);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = static_cast<quint8>(ao_data[i]);
        }
    }

    *x = xStart + ((oWidth - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::drawFast8bits1A(void *drawParameters,
                                      int oWidth,
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
    auto params = reinterpret_cast<DrawParameters *>(drawParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(oWidth - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= oWidth - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ao_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];

            xo_data[i] = dst_line_x[dstWidthOffsetX[xoff]];
            ao_data[i] = dst_line_a[dstWidthOffsetA[xoff]];
        }

        auto xi = s.load(xi_data);
        auto ai = s.load(ai_data);

        auto xo = s.load(xo_data);
        auto ao = s.load(ao_data);

        // Alpha blend

        auto aiMult = s.mul(ai, NativeType(255));
        auto aoMult = s.mul(ao, s.sub(s.load(NativeType(255)), ai));

        auto a = s.add(aiMult, aoMult);
        xo = s.sdiv(s.add(s.mul(xi, aiMult), s.mul(xo, aoMult)), a);
        ao = s.div(a, NativeType(255));

        s.store(xo_data, xo);
        s.store(ao_data, ao);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = static_cast<quint8>(ao_data[i]);
        }
    }

    *x = xStart + ((oWidth - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::drawFastLc8bits3A(void *drawParameters,
                                        int oWidth,
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
    auto params = reinterpret_cast<DrawParameters *>(drawParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(oWidth - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= oWidth - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ao_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            auto xs = (xoff * iDiffX + oMultX) / oDiffX;

            auto srcWidthOffsetX = (xs >> xiWidthDiv) * xiStep;
            auto srcWidthOffsetY = (xs >> yiWidthDiv) * yiStep;
            auto srcWidthOffsetZ = (xs >> ziWidthDiv) * ziStep;
            auto srcWidthOffsetA = (xs >> aiWidthDiv) * aiStep;

            auto dstWidthOffsetX = (xoff >> xiWidthDiv) * xiStep;
            auto dstWidthOffsetY = (xoff >> yiWidthDiv) * yiStep;
            auto dstWidthOffsetZ = (xoff >> ziWidthDiv) * ziStep;
            auto dstWidthOffsetA = (xoff >> aiWidthDiv) * aiStep;

            xi_data[i] = src_line_x[srcWidthOffsetX];
            yi_data[i] = src_line_y[srcWidthOffsetY];
            zi_data[i] = src_line_z[srcWidthOffsetZ];
            ai_data[i] = src_line_a[srcWidthOffsetA];

            xo_data[i] = dst_line_x[dstWidthOffsetX];
            yo_data[i] = dst_line_y[dstWidthOffsetY];
            zo_data[i] = dst_line_z[dstWidthOffsetZ];
            ao_data[i] = dst_line_a[dstWidthOffsetA];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);
        auto ai = s.load(ai_data);

        auto xo = s.load(xo_data);
        auto yo = s.load(yo_data);
        auto zo = s.load(zo_data);
        auto ao = s.load(ao_data);

        // Alpha blend

        auto aiMult = s.mul(ai, NativeType(255));
        auto aoMult = s.mul(ao, s.sub(s.load(NativeType(255)), ai));

        auto a = s.add(aiMult, aoMult);
        xo = s.sdiv(s.add(s.mul(xi, aiMult), s.mul(xo, aoMult)), a);
        yo = s.sdiv(s.add(s.mul(yi, aiMult), s.mul(yo, aoMult)), a);
        zo = s.sdiv(s.add(s.mul(zi, aiMult), s.mul(zo, aoMult)), a);
        ao = s.div(a, NativeType(255));

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);
        s.store(ao_data, ao);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;

            auto dstWidthOffsetX = (xoff >> xiWidthDiv) * xiStep;
            auto dstWidthOffsetY = (xoff >> yiWidthDiv) * yiStep;
            auto dstWidthOffsetZ = (xoff >> ziWidthDiv) * ziStep;
            auto dstWidthOffsetA = (xoff >> aiWidthDiv) * aiStep;

            dst_line_x[dstWidthOffsetX] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA] = static_cast<quint8>(ao_data[i]);
        }
    }

    *x = xStart + ((oWidth - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::drawFastLc8bits1A(void *drawParameters,
                                        int oWidth,
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
    auto params = reinterpret_cast<DrawParameters *>(drawParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(oWidth - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= oWidth - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ao_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            auto xs = (xoff * iDiffX + oMultX) / oDiffX;

            auto srcWidthOffsetX = (xs >> xiWidthDiv) * xiStep;
            auto srcWidthOffsetA = (xs >> aiWidthDiv) * aiStep;

            auto dstWidthOffsetX = (xoff >> xiWidthDiv) * xiStep;
            auto dstWidthOffsetA = (xoff >> aiWidthDiv) * aiStep;

            xi_data[i] = src_line_x[srcWidthOffsetX];
            ai_data[i] = src_line_a[srcWidthOffsetA];

            xo_data[i] = dst_line_x[dstWidthOffsetX];
            ao_data[i] = dst_line_a[dstWidthOffsetA];
        }

        auto xi = s.load(xi_data);
        auto ai = s.load(ai_data);

        auto xo = s.load(xo_data);
        auto ao = s.load(ao_data);

        // Alpha blend

        auto aiMult = s.mul(ai, NativeType(255));
        auto aoMult = s.mul(ao, s.sub(s.load(NativeType(255)), ai));

        auto a = s.add(aiMult, aoMult);
        xo = s.sdiv(s.add(s.mul(xi, aiMult), s.mul(xo, aoMult)), a);
        ao = s.div(a, NativeType(255));

        s.store(xo_data, xo);
        s.store(ao_data, ao);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;

            auto dstWidthOffsetX = (xoff >> xiWidthDiv) * xiStep;
            auto dstWidthOffsetA = (xoff >> aiWidthDiv) * aiStep;

            dst_line_x[dstWidthOffsetX] = static_cast<quint8>(xo_data[i]);
            dst_line_a[dstWidthOffsetA] = static_cast<quint8>(ao_data[i]);
        }
    }

    *x = xStart + ((oWidth - xStart) / vlen) * vlen;
    SimdType::end();
}

void *SimdCorePrivate::createConvertParameters(qint64 *colorMatrix,
                                               qint64 *alphaMatrix,
                                               qint64 *minValues,
                                               qint64 *maxValues,
                                               qint64 colorShift,
                                               qint64 alphaShift)
{
    return new ConvertParameters(colorMatrix,
                                 alphaMatrix,
                                 minValues,
                                 maxValues,
                                 colorShift,
                                 alphaShift);
}

void SimdCorePrivate::freeConvertParameters(void *convertParameters)
{
    if (convertParameters)
        delete reinterpret_cast<ConvertParameters *>(convertParameters);
}

void SimdCorePrivate::convertFast8bits3to3(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetY,
                                           const int *srcWidthOffsetZ,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetY,
                                           const int *dstWidthOffsetZ,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyMatrix(xi, yi, zi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3to3A(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetY,
                                            const int *srcWidthOffsetZ,
                                            const int *dstWidthOffsetX,
                                            const int *dstWidthOffsetY,
                                            const int *dstWidthOffsetZ,
                                            const int *dstWidthOffsetA,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_y,
                                            const quint8 *src_line_z,
                                            quint8 *dst_line_x,
                                            quint8 *dst_line_y,
                                            quint8 *dst_line_z,
                                            quint8 *dst_line_a,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyMatrix(xi, yi, zi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = 0xff;
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3Ato3(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetY,
                                            const int *srcWidthOffsetZ,
                                            const int *srcWidthOffsetA,
                                            const int *dstWidthOffsetX,
                                            const int *dstWidthOffsetY,
                                            const int *dstWidthOffsetZ,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_y,
                                            const quint8 *src_line_z,
                                            const quint8 *src_line_a,
                                            quint8 *dst_line_x,
                                            quint8 *dst_line_y,
                                            quint8 *dst_line_z,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);
        auto ai = s.load(ai_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyMatrix(xi, yi, zi, &xo, &yo, &zo);
        params->applyAlpha(ai, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3Ato3A(void *convertParameters,
                                             const int *srcWidthOffsetX,
                                             const int *srcWidthOffsetY,
                                             const int *srcWidthOffsetZ,
                                             const int *srcWidthOffsetA,
                                             const int *dstWidthOffsetX,
                                             const int *dstWidthOffsetY,
                                             const int *dstWidthOffsetZ,
                                             const int *dstWidthOffsetA,
                                             int xmax,
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
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyMatrix(xi, yi, zi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = src_line_a[srcWidthOffsetA[xoff]];
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bitsV3Ato3(void *convertParameters,
                                             const int *srcWidthOffsetX,
                                             const int *srcWidthOffsetY,
                                             const int *srcWidthOffsetZ,
                                             const int *srcWidthOffsetA,
                                             const int *dstWidthOffsetX,
                                             const int *dstWidthOffsetY,
                                             const int *dstWidthOffsetZ,
                                             int xmax,
                                             const quint8 *src_line_x,
                                             const quint8 *src_line_y,
                                             const quint8 *src_line_z,
                                             const quint8 *src_line_a,
                                             quint8 *dst_line_x,
                                             quint8 *dst_line_y,
                                             quint8 *dst_line_z,
                                             int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];
        }

        auto xo = s.load(xi_data);
        auto yo = s.load(yi_data);
        auto zo = s.load(zi_data);
        auto ai = s.load(ai_data);

        params->applyAlpha(ai, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3to1(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *srcWidthOffsetY,
                                           const int *srcWidthOffsetZ,
                                           const int *dstWidthOffsetX,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           const quint8 *src_line_y,
                                           const quint8 *src_line_z,
                                           quint8 *dst_line_x,
                                           int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        params->applyPoint(xi, yi, zi, &xo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        s.store(xo_data, xo);

        for (int i = 0; i < vlen; ++i)
            dst_line_x[dstWidthOffsetX[xLocal + i]] = static_cast<quint8>(xo_data[i]);
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3to1A(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetY,
                                            const int *srcWidthOffsetZ,
                                            const int *dstWidthOffsetX,
                                            const int *dstWidthOffsetA,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_y,
                                            const quint8 *src_line_z,
                                            quint8 *dst_line_x,
                                            quint8 *dst_line_a,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        params->applyPoint(xi, yi, zi, &xo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        s.store(xo_data, xo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = 0xff;
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3Ato1(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetY,
                                            const int *srcWidthOffsetZ,
                                            const int *srcWidthOffsetA,
                                            const int *dstWidthOffsetX,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_y,
                                            const quint8 *src_line_z,
                                            const quint8 *src_line_a,
                                            quint8 *dst_line_x,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);
        auto ai = s.load(ai_data);

        VectorType xo;
        params->applyPoint(xi, yi, zi, &xo);
        params->applyAlpha(ai, &xo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        s.store(xo_data, xo);

        for (int i = 0; i < vlen; ++i)
            dst_line_x[dstWidthOffsetX[xLocal + i]] = static_cast<quint8>(xo_data[i]);
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits3Ato1A(void *convertParameters,
                                             const int *srcWidthOffsetX,
                                             const int *srcWidthOffsetY,
                                             const int *srcWidthOffsetZ,
                                             const int *srcWidthOffsetA,
                                             const int *dstWidthOffsetX,
                                             const int *dstWidthOffsetA,
                                             int xmax,
                                             const quint8 *src_line_x,
                                             const quint8 *src_line_y,
                                             const quint8 *src_line_z,
                                             const quint8 *src_line_a,
                                             quint8 *dst_line_x,
                                             quint8 *dst_line_a,
                                             int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            yi_data[i] = src_line_y[srcWidthOffsetY[xoff]];
            zi_data[i] = src_line_z[srcWidthOffsetZ[xoff]];
        }

        auto xi = s.load(xi_data);
        auto yi = s.load(yi_data);
        auto zi = s.load(zi_data);

        VectorType xo;
        params->applyPoint(xi, yi, zi, &xo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        s.store(xo_data, xo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = src_line_a[srcWidthOffsetA[xoff]];
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits1to3(void *convertParameters,
                                           const int *srcWidthOffsetX,
                                           const int *dstWidthOffsetX,
                                           const int *dstWidthOffsetY,
                                           const int *dstWidthOffsetZ,
                                           int xmax,
                                           const quint8 *src_line_x,
                                           quint8 *dst_line_x,
                                           quint8 *dst_line_y,
                                           quint8 *dst_line_z,
                                           int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i)
            xi_data[i] = src_line_x[srcWidthOffsetX[xLocal + i]];

        auto xi = s.load(xi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyPoint(xi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits1to3A(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *dstWidthOffsetX,
                                            const int *dstWidthOffsetY,
                                            const int *dstWidthOffsetZ,
                                            const int *dstWidthOffsetA,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            quint8 *dst_line_x,
                                            quint8 *dst_line_y,
                                            quint8 *dst_line_z,
                                            quint8 *dst_line_a,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i)
            xi_data[i] = src_line_x[srcWidthOffsetX[xLocal + i]];

        auto xi = s.load(xi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyPoint(xi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = 0xff;
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits1Ato3(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetA,
                                            const int *dstWidthOffsetX,
                                            const int *dstWidthOffsetY,
                                            const int *dstWidthOffsetZ,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_a,
                                            quint8 *dst_line_x,
                                            quint8 *dst_line_y,
                                            quint8 *dst_line_z,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i <vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];
        }

        auto xi = s.load(xi_data);
        auto ai = s.load(ai_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyPoint(xi, &xo, &yo, &zo);
        params->applyAlpha(ai, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits1Ato3A(void *convertParameters,
                                             const int *srcWidthOffsetX,
                                             const int *srcWidthOffsetA,
                                             const int *dstWidthOffsetX,
                                             const int *dstWidthOffsetY,
                                             const int *dstWidthOffsetZ,
                                             const int *dstWidthOffsetA,
                                             int xmax,
                                             const quint8 *src_line_x,
                                             const quint8 *src_line_a,
                                             quint8 *dst_line_x,
                                             quint8 *dst_line_y,
                                             quint8 *dst_line_z,
                                             quint8 *dst_line_a,
                                             int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i)
            xi_data[i] = src_line_x[srcWidthOffsetX[xLocal + i]];

        auto xi = s.load(xi_data);

        VectorType xo;
        VectorType yo;
        VectorType zo;
        params->applyPoint(xi, &xo, &yo, &zo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType yo_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType zo_data[SIMD_DEFAULT_SIZE];

        s.store(xo_data, xo);
        s.store(yo_data, yo);
        s.store(zo_data, zo);

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            dst_line_x[dstWidthOffsetX[xoff]] = static_cast<quint8>(xo_data[i]);
            dst_line_y[dstWidthOffsetY[xoff]] = static_cast<quint8>(yo_data[i]);
            dst_line_z[dstWidthOffsetZ[xoff]] = static_cast<quint8>(zo_data[i]);
            dst_line_a[dstWidthOffsetA[xoff]] = src_line_a[srcWidthOffsetA[xoff]];
        }
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

void SimdCorePrivate::convertFast8bits1Ato1(void *convertParameters,
                                            const int *srcWidthOffsetX,
                                            const int *srcWidthOffsetA,
                                            const int *dstWidthOffsetX,
                                            int xmax,
                                            const quint8 *src_line_x,
                                            const quint8 *src_line_a,
                                            quint8 *dst_line_x,
                                            int *x)
{
    auto params = reinterpret_cast<ConvertParameters *>(convertParameters);
    auto &s = params->simd;
    auto vlen = s.size();
    int xStart = *x;

    #pragma omp parallel for schedule(dynamic, 1) if(xmax - xStart >= 1024)
    for (int xLocal = xStart; xLocal <= xmax - vlen; xLocal += vlen) {
        alignas(SIMD_ALIGN) NativeType xi_data[SIMD_DEFAULT_SIZE];
        alignas(SIMD_ALIGN) NativeType ai_data[SIMD_DEFAULT_SIZE];

        for (int i = 0; i < vlen; ++i) {
            auto xoff = xLocal + i;
            xi_data[i] = src_line_x[srcWidthOffsetX[xoff]];
            ai_data[i] = src_line_a[srcWidthOffsetA[xoff]];
        }

        auto xo = s.load(xi_data);
        auto ai = s.load(ai_data);

        params->applyAlpha(ai, &xo);

        alignas(SIMD_ALIGN) NativeType xo_data[SIMD_DEFAULT_SIZE];
        s.store(xo_data, xo);

        for (int i = 0; i < vlen; ++i)
            dst_line_x[dstWidthOffsetX[xLocal + i]] = static_cast<quint8>(xo_data[i]);
    }

    *x = xStart + ((xmax - xStart) / vlen) * vlen;
    SimdType::end();
}

#include "moc_simdcore.cpp"
