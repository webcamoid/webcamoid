/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QGenericMatrix>
#include <QMutex>
#include <QQmlEngine>
#include <QRect>
#include <QtEndian>
#include <QtMath>

#include "akalgorithm.h"
#include "akfrac.h"
#include "aksimd.h"
#include "akvideocaps.h"
#include "akvideoconverter.h"
#include "akvideoformatspec.h"
#include "akvideopacket.h"

#define SCALE_EMULT 8

/*
 * NOTE: Using integer numbers is much faster but can overflow with high
 * resolution and depth frames.
 */

#if 0
using DlSumType = quint64;
#else
using DlSumType = qreal;
#endif

enum ConvertType
{
    ConvertType_Vector,
    ConvertType_1to1,
    ConvertType_1to3,
    ConvertType_3to1,
    ConvertType_3to3,
};

enum ConvertDataTypes
{
    ConvertDataTypes_8_8,
    ConvertDataTypes_8_16,
    ConvertDataTypes_8_32,
    ConvertDataTypes_16_8,
    ConvertDataTypes_16_16,
    ConvertDataTypes_16_32,
    ConvertDataTypes_32_8,
    ConvertDataTypes_32_16,
    ConvertDataTypes_32_32,
};

enum ConvertAlphaMode
{
    ConvertAlphaMode_AI_AO,
    ConvertAlphaMode_AI_O,
    ConvertAlphaMode_I_AO,
    ConvertAlphaMode_I_O,
};

enum ResizeMode
{
    ResizeMode_Keep,
    ResizeMode_Up,
    ResizeMode_Down,
};

using ConvertFast8bits3to3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bits3to3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits3Ato3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bits3Ato3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
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
using ConvertFast8bitsV3to3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bitsV3to3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bitsV3Ato3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bitsV3Ato3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
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
using ConvertFast8bits3to1Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             int *x);
using ConvertFast8bits3to1AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             quint8 *dst_line_x,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits3Ato1Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             int *x);
using ConvertFast8bits3Ato1AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetY,
             const int *srcWidthOffsetZ,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_y,
             const quint8 *src_line_z,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits1to3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bits1to3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits1Ato3Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             int *x);
using ConvertFast8bits1Ato3AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetY,
             const int *dstWidthOffsetZ,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_y,
             quint8 *dst_line_z,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits1to1Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *dstWidthOffsetX,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             quint8 *dst_line_x,
             int *x);
using ConvertFast8bits1to1AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             quint8 *dst_line_x,
             quint8 *dst_line_a,
             int *x);
using ConvertFast8bits1Ato1Type =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             int *x);
using ConvertFast8bits1Ato1AType =
    void (*)(const AkColorConvert &colorConvert,
             const int *srcWidthOffsetX,
             const int *srcWidthOffsetA,
             const int *dstWidthOffsetX,
             const int *dstWidthOffsetA,
             int xmin,
             int xmax,
             const quint8 *src_line_x,
             const quint8 *src_line_a,
             quint8 *dst_line_x,
             quint8 *dst_line_a,
             int *x);

class FrameConvertParameters
{
    public:
        AkColorConvert colorConvert;

        AkVideoCaps inputCaps;
        AkVideoCaps outputCaps;
        AkVideoCaps outputConvertCaps;
        AkVideoPacket outputFrame;
        QRect inputRect;
        AkColorConvert::YuvColorSpace yuvColorSpace {AkColorConvert::YuvColorSpace_ITUR_BT601};
        AkColorConvert::YuvColorSpaceType yuvColorSpaceType {AkColorConvert::YuvColorSpaceType_StudioSwing};
        AkVideoConverter::ScalingMode scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};
        ConvertType convertType {ConvertType_Vector};
        ConvertDataTypes convertDataTypes {ConvertDataTypes_8_8};
        ConvertAlphaMode alphaMode {ConvertAlphaMode_AI_AO};
        ResizeMode resizeMode {ResizeMode_Keep};
        bool fastConvertion {false};

        int fromEndian {Q_BYTE_ORDER};
        int toEndian {Q_BYTE_ORDER};

        int xmin {0};
        int ymin {0};
        int xmax {0};
        int ymax {0};

        int inputWidth {0};
        int inputWidth_1 {0};
        int inputHeight {0};

        int *srcWidth {nullptr};
        int *srcWidth_1 {nullptr};
        int *srcWidthOffsetX {nullptr};
        int *srcWidthOffsetY {nullptr};
        int *srcWidthOffsetZ {nullptr};
        int *srcWidthOffsetA {nullptr};
        int *srcHeight {nullptr};

        int *dlSrcWidthOffsetX {nullptr};
        int *dlSrcWidthOffsetY {nullptr};
        int *dlSrcWidthOffsetZ {nullptr};
        int *dlSrcWidthOffsetA {nullptr};

        int *srcWidthOffsetX_1 {nullptr};
        int *srcWidthOffsetY_1 {nullptr};
        int *srcWidthOffsetZ_1 {nullptr};
        int *srcWidthOffsetA_1 {nullptr};
        int *srcHeight_1 {nullptr};

        int *dstWidthOffsetX {nullptr};
        int *dstWidthOffsetY {nullptr};
        int *dstWidthOffsetZ {nullptr};
        int *dstWidthOffsetA {nullptr};

        size_t *srcHeightDlOffset {nullptr};
        size_t *srcHeightDlOffset_1 {nullptr};

        DlSumType *integralImageDataX {nullptr};
        DlSumType *integralImageDataY {nullptr};
        DlSumType *integralImageDataZ {nullptr};
        DlSumType *integralImageDataA {nullptr};

        qint64 *kx {nullptr};
        qint64 *ky {nullptr};
        DlSumType *kdl {nullptr};

        int planeXi {0};
        int planeYi {0};
        int planeZi {0};
        int planeAi {0};

        AkColorComponent compXi;
        AkColorComponent compYi;
        AkColorComponent compZi;
        AkColorComponent compAi;

        int planeXo {0};
        int planeYo {0};
        int planeZo {0};
        int planeAo {0};

        AkColorComponent compXo;
        AkColorComponent compYo;
        AkColorComponent compZo;
        AkColorComponent compAo;

        size_t xiOffset {0};
        size_t yiOffset {0};
        size_t ziOffset {0};
        size_t aiOffset {0};

        size_t xoOffset {0};
        size_t yoOffset {0};
        size_t zoOffset {0};
        size_t aoOffset {0};

        size_t xiShift {0};
        size_t yiShift {0};
        size_t ziShift {0};
        size_t aiShift {0};

        size_t xoShift {0};
        size_t yoShift {0};
        size_t zoShift {0};
        size_t aoShift {0};

        quint64 maxXi {0};
        quint64 maxYi {0};
        quint64 maxZi {0};
        quint64 maxAi {0};

        quint64 maskXo {0};
        quint64 maskYo {0};
        quint64 maskZo {0};
        quint64 maskAo {0};

        quint64 alphaMask {0};

        ConvertFast8bits3to3Type    convertSIMDFast8bits3to3    {nullptr};
        ConvertFast8bits3to3AType   convertSIMDFast8bits3to3A   {nullptr};
        ConvertFast8bits3Ato3Type   convertSIMDFast8bits3Ato3   {nullptr};
        ConvertFast8bits3Ato3AType  convertSIMDFast8bits3Ato3A  {nullptr};
        ConvertFast8bitsV3to3Type   convertSIMDFast8bitsV3to3   {nullptr};
        ConvertFast8bitsV3to3AType  convertSIMDFast8bitsV3to3A  {nullptr};
        ConvertFast8bitsV3Ato3Type  convertSIMDFast8bitsV3Ato3  {nullptr};
        ConvertFast8bitsV3Ato3AType convertSIMDFast8bitsV3Ato3A {nullptr};
        ConvertFast8bits3to1Type    convertSIMDFast8bits3to1    {nullptr};
        ConvertFast8bits3to1AType   convertSIMDFast8bits3to1A   {nullptr};
        ConvertFast8bits3Ato1Type   convertSIMDFast8bits3Ato1   {nullptr};
        ConvertFast8bits3Ato1AType  convertSIMDFast8bits3Ato1A  {nullptr};
        ConvertFast8bits1to3Type    convertSIMDFast8bits1to3    {nullptr};
        ConvertFast8bits1to3AType   convertSIMDFast8bits1to3A   {nullptr};
        ConvertFast8bits1Ato3Type   convertSIMDFast8bits1Ato3   {nullptr};
        ConvertFast8bits1Ato3AType  convertSIMDFast8bits1Ato3A  {nullptr};
        ConvertFast8bits1to1Type    convertSIMDFast8bits1to1    {nullptr};
        ConvertFast8bits1to1AType   convertSIMDFast8bits1to1A   {nullptr};
        ConvertFast8bits1Ato1Type   convertSIMDFast8bits1Ato1   {nullptr};
        ConvertFast8bits1Ato1AType  convertSIMDFast8bits1Ato1A  {nullptr};

        FrameConvertParameters();
        FrameConvertParameters(const FrameConvertParameters &other);
        ~FrameConvertParameters();
        FrameConvertParameters &operator =(const FrameConvertParameters &other);
        inline void clearBuffers();
        inline void clearDlBuffers();
        inline void allocateBuffers(const AkVideoCaps &ocaps);
        inline void allocateDlBuffers(const AkVideoCaps &icaps,
                                      const AkVideoCaps &ocaps);
        void configure(const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps,
                       AkColorConvert &colorConvert,
                       AkColorConvert::YuvColorSpace yuvColorSpace,
                       AkColorConvert::YuvColorSpaceType yuvColorSpaceType);
        void configureScaling(const AkVideoCaps &icaps,
                              const AkVideoCaps &ocaps,
                              const QRect &inputRect,
                              AkVideoConverter::AspectRatioMode aspectRatioMode);
        void reset();
};

class AkVideoConverterPrivate
{
    public:
        QMutex m_mutex;
        AkVideoCaps m_outputCaps;
        FrameConvertParameters *m_fc {nullptr};
        size_t m_fcSize {0};
        int m_cacheIndex {0};
        AkColorConvert::YuvColorSpace m_yuvColorSpace {AkColorConvert::YuvColorSpace_ITUR_BT601};
        AkColorConvert::YuvColorSpaceType m_yuvColorSpaceType {AkColorConvert::YuvColorSpaceType_StudioSwing};
        AkVideoConverter::ScalingMode m_scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode m_aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};
        QRect m_inputRect;

        /* Color blendig functions
         *
         * kx and ky must be in the range of [0, 2^N]
         */

        template <int N>
        inline void blend(qint64 a,
                          qint64 bx, qint64 by,
                          qint64 kx, qint64 ky,
                          qint64 *c) const
        {
            *c = (kx * (bx - a) + ky * (by - a) + (a << (N + 1))) >> (N + 1);
        }

        template <int N>
        inline void blend2(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
        }

        template <int N>
        inline void blend3(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
            this->blend<N>(ax[2],
                           bx[2], by[2],
                           kx, ky,
                           c + 2);
        }

        template <int N>
        inline void blend4(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
            this->blend<N>(ax[2],
                           bx[2], by[2],
                           kx, ky,
                           c + 2);
            this->blend<N>(ax[3],
                           bx[3], by[3],
                           kx, ky,
                           c + 3);
        }

        /* Component reading functions */

        template <typename InputType>
        inline void read1(const FrameConvertParameters &fc,
                          const quint8 *src_line_x,
                          int x,
                          InputType *xi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            *xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

            if (fc.fromEndian != Q_BYTE_ORDER)
                *xi = AkAlgorithm::swapBytes(InputType(*xi));

            *xi = (*xi >> fc.xiShift) & fc.maxXi;
        }

        template <typename InputType>
        inline void read1A(const FrameConvertParameters &fc,
                           const quint8 *src_line_x,
                           const quint8 *src_line_a,
                           int x,
                           InputType *xi,
                           InputType *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto ait = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xit = AkAlgorithm::swapBytes(InputType(xit));
                ait = AkAlgorithm::swapBytes(InputType(ait));
            }

            *xi = (xit >> fc.xiShift) & fc.maxXi;
            *ai = (ait >> fc.aiShift) & fc.maxAi;
        }

        template <typename InputType>
        inline void readDL1(const FrameConvertParameters &fc,
                            const DlSumType *src_line_x,
                            const DlSumType *src_line_x_1,
                            int x,
                            const DlSumType *kdl,
                            InputType *xi) const
        {
            auto &xs = fc.srcWidth[x];
            auto &xs_1 = fc.srcWidth_1[x];
            auto &k = kdl[x];

            *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
        }

        template <typename InputType>
        inline void readDL1A(const FrameConvertParameters &fc,
                             const DlSumType *src_line_x,
                             const DlSumType *src_line_a,
                             const DlSumType *src_line_x_1,
                             const DlSumType *src_line_a_1,
                             int x,
                             const DlSumType *kdl,
                             InputType *xi,
                             InputType *ai) const
        {
            auto &xs = fc.srcWidth[x];
            auto &xs_1 = fc.srcWidth_1[x];
            auto &k = kdl[x];

            *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
            *ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;
        }

        template <typename InputType>
        inline void readUL1(const FrameConvertParameters &fc,
                            const quint8 *src_line_x,
                            const quint8 *src_line_x_1,
                            int x,
                            qint64 ky,
                            InputType *xi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_x_1 = fc.srcWidthOffsetX_1[x];

            auto xi_ = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
            auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xi_ = AkAlgorithm::swapBytes(InputType(xi_));
                xi_x = AkAlgorithm::swapBytes(InputType(xi_x));
                xi_y = AkAlgorithm::swapBytes(InputType(xi_y));
            }

            xi_ = (xi_ >> fc.xiShift) & fc.maxXi;
            xi_x = (xi_x >> fc.xiShift) & fc.maxXi;
            xi_y = (xi_y >> fc.xiShift) & fc.maxXi;

            qint64 xib = 0;
            this->blend<SCALE_EMULT>(xi_,
                                     xi_x, xi_y,
                                     fc.kx[x], ky,
                                     &xib);
            *xi = xib;
        }

        inline void readF8UL1(const FrameConvertParameters &fc,
                              const quint8 *src_line_x,
                              const quint8 *src_line_x_1,
                              int x,
                              qint64 ky,
                              quint8 *xi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_x_1 = fc.srcWidthOffsetX_1[x];

            auto xi_ = src_line_x[xs_x];
            auto xi_x = src_line_x[xs_x_1];
            auto xi_y = src_line_x_1[xs_x];

            qint64 xib = 0;
            this->blend<SCALE_EMULT>(xi_,
                                     xi_x, xi_y,
                                     fc.kx[x], ky,
                                     &xib);
            *xi = quint8(xib);
        }

        template <typename InputType>
        inline void readUL1A(const FrameConvertParameters &fc,
                             const quint8 *src_line_x,
                             const quint8 *src_line_a,
                             const quint8 *src_line_x_1,
                             const quint8 *src_line_a_1,
                             int x,
                             qint64 ky,
                             InputType *xi,
                             InputType *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_a_1 = fc.srcWidthOffsetA_1[x];

            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];

            auto xai0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto xai1 = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
            auto xai_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
            auto xai_x1 = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
            auto xai_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
            auto xai_y1 = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xai0 = AkAlgorithm::swapBytes(InputType(xai0));
                xai1 = AkAlgorithm::swapBytes(InputType(xai1));
                xai_x0 = AkAlgorithm::swapBytes(InputType(xai_x0));
                xai_x1 = AkAlgorithm::swapBytes(InputType(xai_x1));
                xai_y0 = AkAlgorithm::swapBytes(InputType(xai_y0));
                xai_y1 = AkAlgorithm::swapBytes(InputType(xai_y1));
            }

            xai[0] = (xai0 >> fc.xiShift) & fc.maxXi;
            xai[1] = (xai1 >> fc.aiShift) & fc.maxAi;
            xai_x[0] = (xai_x0 >> fc.xiShift) & fc.maxXi;
            xai_x[1] = (xai_x1 >> fc.aiShift) & fc.maxAi;
            xai_y[0] = (xai_y0 >> fc.xiShift) & fc.maxXi;
            xai_y[1] = (xai_y1 >> fc.aiShift) & fc.maxAi;

            qint64 xaib[2];
            this->blend2<SCALE_EMULT>(xai,
                                      xai_x, xai_y,
                                      fc.kx[x], ky,
                                      xaib);

            *xi = xaib[0];
            *ai = xaib[1];
        }

        inline void readF8UL1A(const FrameConvertParameters &fc,
                               const quint8 *src_line_x,
                               const quint8 *src_line_a,
                               const quint8 *src_line_x_1,
                               const quint8 *src_line_a_1,
                               int x,
                               qint64 ky,
                               quint8 *xi,
                               quint8 *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_a_1 = fc.srcWidthOffsetA_1[x];

            qint64 xai[] = {
                src_line_x[xs_x],
                src_line_a[xs_a]
            };
            qint64 xai_x[] = {
                src_line_x[xs_x_1],
                src_line_a[xs_a_1]
            };
            qint64 xai_y[] = {
                src_line_x_1[xs_x],
                src_line_a_1[xs_a]
            };

            qint64 xaib[2];
            this->blend2<SCALE_EMULT>(xai,
                                      xai_x, xai_y,
                                      fc.kx[x], ky,
                                      xaib);

            *xi = quint8(xaib[0]);
            *ai = quint8(xaib[1]);
        }

        template <typename InputType>
        inline void read3(const FrameConvertParameters &fc,
                          const quint8 *src_line_x,
                          const quint8 *src_line_y,
                          const quint8 *src_line_z,
                          int x,
                          InputType *xi,
                          InputType *yi,
                          InputType *zi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];

            auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto yit = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
            auto zit = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xit = AkAlgorithm::swapBytes(InputType(xit));
                yit = AkAlgorithm::swapBytes(InputType(yit));
                zit = AkAlgorithm::swapBytes(InputType(zit));
            }

            *xi = (xit >> fc.xiShift) & fc.maxXi;
            *yi = (yit >> fc.yiShift) & fc.maxYi;
            *zi = (zit >> fc.ziShift) & fc.maxZi;
        }

        template <typename InputType>
        inline void read3A(const FrameConvertParameters &fc,
                           const quint8 *src_line_x,
                           const quint8 *src_line_y,
                           const quint8 *src_line_z,
                           const quint8 *src_line_a,
                           int x,
                           InputType *xi,
                           InputType *yi,
                           InputType *zi,
                           InputType *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto yit = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
            auto zit = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
            auto ait = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xit = AkAlgorithm::swapBytes(InputType(xit));
                yit = AkAlgorithm::swapBytes(InputType(yit));
                zit = AkAlgorithm::swapBytes(InputType(zit));
                ait = AkAlgorithm::swapBytes(InputType(ait));
            }

            *xi = (xit >> fc.xiShift) & fc.maxXi;
            *yi = (yit >> fc.yiShift) & fc.maxYi;
            *zi = (zit >> fc.ziShift) & fc.maxZi;
            *ai = (ait >> fc.aiShift) & fc.maxAi;
        }

        template <typename InputType>
        inline void readDL3(const FrameConvertParameters &fc,
                            const DlSumType *src_line_x,
                            const DlSumType *src_line_y,
                            const DlSumType *src_line_z,
                            const DlSumType *src_line_x_1,
                            const DlSumType *src_line_y_1,
                            const DlSumType *src_line_z_1,
                            int x,
                            const DlSumType *kdl,
                            InputType *xi,
                            InputType *yi,
                            InputType *zi) const
        {
            auto &xs = fc.srcWidth[x];
            auto &xs_1 = fc.srcWidth_1[x];
            auto &k = kdl[x];

            *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
            *yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
            *zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
        }

        template <typename InputType>
        inline void readDL3A(const FrameConvertParameters &fc,
                             const DlSumType *src_line_x,
                             const DlSumType *src_line_y,
                             const DlSumType *src_line_z,
                             const DlSumType *src_line_a,
                             const DlSumType *src_line_x_1,
                             const DlSumType *src_line_y_1,
                             const DlSumType *src_line_z_1,
                             const DlSumType *src_line_a_1,
                             int x,
                             const DlSumType *kdl,
                             InputType *xi,
                             InputType *yi,
                             InputType *zi,
                             InputType *ai) const
        {
            auto &xs = fc.srcWidth[x];
            auto &xs_1 = fc.srcWidth_1[x];
            auto &k = kdl[x];

            *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
            *yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
            *zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
            *ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;
        }

        template <typename InputType>
        inline void readUL3(const FrameConvertParameters &fc,
                            const quint8 *src_line_x,
                            const quint8 *src_line_y,
                            const quint8 *src_line_z,
                            const quint8 *src_line_x_1,
                            const quint8 *src_line_y_1,
                            const quint8 *src_line_z_1,
                            int x,
                            qint64 ky,
                            InputType *xi,
                            InputType *yi,
                            InputType *zi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_y_1 = fc.srcWidthOffsetY_1[x];
            int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];

            auto xyzi0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto xyzi1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
            auto xyzi2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
            auto xyzi_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
            auto xyzi_x1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
            auto xyzi_x2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
            auto xyzi_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
            auto xyzi_y1 = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
            auto xyzi_y2 = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xyzi0 = AkAlgorithm::swapBytes(InputType(xyzi0));
                xyzi1 = AkAlgorithm::swapBytes(InputType(xyzi1));
                xyzi2 = AkAlgorithm::swapBytes(InputType(xyzi2));
                xyzi_x0 = AkAlgorithm::swapBytes(InputType(xyzi_x0));
                xyzi_x1 = AkAlgorithm::swapBytes(InputType(xyzi_x1));
                xyzi_x2 = AkAlgorithm::swapBytes(InputType(xyzi_x2));
                xyzi_y0 = AkAlgorithm::swapBytes(InputType(xyzi_y0));
                xyzi_y1 = AkAlgorithm::swapBytes(InputType(xyzi_y1));
                xyzi_y2 = AkAlgorithm::swapBytes(InputType(xyzi_y2));
            }

            xyzi[0] = (xyzi0 >> fc.xiShift) & fc.maxXi;
            xyzi[1] = (xyzi1 >> fc.yiShift) & fc.maxYi;
            xyzi[2] = (xyzi2 >> fc.ziShift) & fc.maxZi;
            xyzi_x[0] = (xyzi_x0 >> fc.xiShift) & fc.maxXi;
            xyzi_x[1] = (xyzi_x1 >> fc.yiShift) & fc.maxYi;
            xyzi_x[2] = (xyzi_x2 >> fc.ziShift) & fc.maxZi;
            xyzi_y[0] = (xyzi_y0 >> fc.xiShift) & fc.maxXi;
            xyzi_y[1] = (xyzi_y1 >> fc.yiShift) & fc.maxYi;
            xyzi_y[2] = (xyzi_y2 >> fc.ziShift) & fc.maxZi;

            qint64 xyzib[3];
            this->blend3<SCALE_EMULT>(xyzi,
                                      xyzi_x, xyzi_y,
                                      fc.kx[x], ky,
                                      xyzib);

            *xi = xyzib[0];
            *yi = xyzib[1];
            *zi = xyzib[2];
        }

        inline void readF8UL3(const FrameConvertParameters &fc,
                              const quint8 *src_line_x,
                              const quint8 *src_line_y,
                              const quint8 *src_line_z,
                              const quint8 *src_line_x_1,
                              const quint8 *src_line_y_1,
                              const quint8 *src_line_z_1,
                              int x,
                              qint64 ky,
                              quint8 *xi,
                              quint8 *yi,
                              quint8 *zi) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_y_1 = fc.srcWidthOffsetY_1[x];
            int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

            qint64 xyzi[] = {
                src_line_x[xs_x],
                src_line_y[xs_y],
                src_line_z[xs_z]
            };
            qint64 xyzi_x[] = {
                src_line_x[xs_x_1],
                src_line_y[xs_y_1],
                src_line_z[xs_z_1]
            };
            qint64 xyzi_y[] = {
                src_line_x_1[xs_x],
                src_line_y_1[xs_y],
                src_line_z_1[xs_z]
            };

            qint64 xyzib[3];
            this->blend3<SCALE_EMULT>(xyzi,
                                      xyzi_x, xyzi_y,
                                      fc.kx[x], ky,
                                      xyzib);

            *xi = quint8(xyzib[0]);
            *yi = quint8(xyzib[1]);
            *zi = quint8(xyzib[2]);
        }

        template <typename InputType>
        inline void readUL3A(const FrameConvertParameters &fc,
                             const quint8 *src_line_x,
                             const quint8 *src_line_y,
                             const quint8 *src_line_z,
                             const quint8 *src_line_a,
                             const quint8 *src_line_x_1,
                             const quint8 *src_line_y_1,
                             const quint8 *src_line_z_1,
                             const quint8 *src_line_a_1,
                             int x,
                             qint64 ky,
                             InputType *xi,
                             InputType *yi,
                             InputType *zi,
                             InputType *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_y_1 = fc.srcWidthOffsetY_1[x];
            int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
            int &xs_a_1 = fc.srcWidthOffsetA_1[x];

            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];

            auto xyzai0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
            auto xyzai1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
            auto xyzai2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
            auto xyzai3 = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
            auto xyzai_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
            auto xyzai_x1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
            auto xyzai_x2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
            auto xyzai_x3 = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
            auto xyzai_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
            auto xyzai_y1 = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
            auto xyzai_y2 = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
            auto xyzai_y3 = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

            if (fc.fromEndian != Q_BYTE_ORDER) {
                xyzai0 = AkAlgorithm::swapBytes(InputType(xyzai0));
                xyzai1 = AkAlgorithm::swapBytes(InputType(xyzai1));
                xyzai2 = AkAlgorithm::swapBytes(InputType(xyzai2));
                xyzai3 = AkAlgorithm::swapBytes(InputType(xyzai3));
                xyzai_x0 = AkAlgorithm::swapBytes(InputType(xyzai_x0));
                xyzai_x1 = AkAlgorithm::swapBytes(InputType(xyzai_x1));
                xyzai_x2 = AkAlgorithm::swapBytes(InputType(xyzai_x2));
                xyzai_x3 = AkAlgorithm::swapBytes(InputType(xyzai_x3));
                xyzai_y0 = AkAlgorithm::swapBytes(InputType(xyzai_y0));
                xyzai_y1 = AkAlgorithm::swapBytes(InputType(xyzai_y1));
                xyzai_y2 = AkAlgorithm::swapBytes(InputType(xyzai_y2));
                xyzai_y3 = AkAlgorithm::swapBytes(InputType(xyzai_y3));
            }

            xyzai[0] = (xyzai0 >> fc.xiShift) & fc.maxXi;
            xyzai[1] = (xyzai1 >> fc.yiShift) & fc.maxYi;
            xyzai[2] = (xyzai2 >> fc.ziShift) & fc.maxZi;
            xyzai[3] = (xyzai3 >> fc.aiShift) & fc.maxAi;
            xyzai_x[0] = (xyzai_x0 >> fc.xiShift) & fc.maxXi;
            xyzai_x[1] = (xyzai_x1 >> fc.yiShift) & fc.maxYi;
            xyzai_x[2] = (xyzai_x2 >> fc.ziShift) & fc.maxZi;
            xyzai_x[3] = (xyzai_x3 >> fc.aiShift) & fc.maxAi;
            xyzai_y[0] = (xyzai_y0 >> fc.xiShift) & fc.maxXi;
            xyzai_y[1] = (xyzai_y1 >> fc.yiShift) & fc.maxYi;
            xyzai_y[2] = (xyzai_y2 >> fc.ziShift) & fc.maxZi;
            xyzai_y[3] = (xyzai_y3 >> fc.aiShift) & fc.maxAi;

            qint64 xyzaib[4];
            this->blend4<SCALE_EMULT>(xyzai,
                                      xyzai_x, xyzai_y,
                                      fc.kx[x], ky,
                                      xyzaib);

            *xi = xyzaib[0];
            *yi = xyzaib[1];
            *zi = xyzaib[2];
            *ai = xyzaib[3];
        }

        inline void readF8UL3A(const FrameConvertParameters &fc,
                               const quint8 *src_line_x,
                               const quint8 *src_line_y,
                               const quint8 *src_line_z,
                               const quint8 *src_line_a,
                               const quint8 *src_line_x_1,
                               const quint8 *src_line_y_1,
                               const quint8 *src_line_z_1,
                               const quint8 *src_line_a_1,
                               int x,
                               qint64 ky,
                               quint8 *xi,
                               quint8 *yi,
                               quint8 *zi,
                               quint8 *ai) const
        {
            int &xs_x = fc.srcWidthOffsetX[x];
            int &xs_y = fc.srcWidthOffsetY[x];
            int &xs_z = fc.srcWidthOffsetZ[x];
            int &xs_a = fc.srcWidthOffsetA[x];

            int &xs_x_1 = fc.srcWidthOffsetX_1[x];
            int &xs_y_1 = fc.srcWidthOffsetY_1[x];
            int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
            int &xs_a_1 = fc.srcWidthOffsetA_1[x];

            qint64 xyzai[] = {
                src_line_x[xs_x],
                src_line_y[xs_y],
                src_line_z[xs_z],
                src_line_a[xs_a]
            };
            qint64 xyzai_x[] = {
                src_line_x[xs_x_1],
                src_line_y[xs_y_1],
                src_line_z[xs_z_1],
                src_line_a[xs_a_1]
            };
            qint64 xyzai_y[] = {
                src_line_x_1[xs_x],
                src_line_y_1[xs_y],
                src_line_z_1[xs_z],
                src_line_a_1[xs_a]
            };

            qint64 xyzaib[4];
            this->blend4<SCALE_EMULT>(xyzai,
                                      xyzai_x, xyzai_y,
                                      fc.kx[x], ky,
                                      xyzaib);

            *xi = quint8(xyzaib[0]);
            *yi = quint8(xyzaib[1]);
            *zi = quint8(xyzaib[2]);
            *ai = quint8(xyzaib[3]);
        }

        /* Component writing functions */

        template <typename OutputType>
        inline void write1(const FrameConvertParameters &fc,
                           quint8 *dst_line_x,
                           int x,
                           OutputType xo) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
        }

        template <typename OutputType>
        inline void write1A(const FrameConvertParameters &fc,
                            quint8 *dst_line_x,
                            quint8 *dst_line_a,
                            int x,
                            OutputType xo,
                            OutputType ao) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            int &xd_a = fc.dstWidthOffsetA[x];

            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            *ao_ = (*ao_ & OutputType(fc.maskAo)) | (OutputType(ao) << fc.aoShift);
        }

        template <typename OutputType>
        inline void write1A(const FrameConvertParameters &fc,
                            quint8 *dst_line_x,
                            quint8 *dst_line_a,
                            int x,
                            OutputType xo) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            int &xd_a = fc.dstWidthOffsetA[x];

            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            *ao_ = *ao_ | OutputType(fc.alphaMask);
        }

        template <typename OutputType>
        inline void write3(const FrameConvertParameters &fc,
                           quint8 *dst_line_x,
                           quint8 *dst_line_y,
                           quint8 *dst_line_z,
                           int x,
                           OutputType xo,
                           OutputType yo,
                           OutputType zo) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            int &xd_y = fc.dstWidthOffsetY[x];
            int &xd_z = fc.dstWidthOffsetZ[x];

            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
            auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
            *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
        }

        template <typename OutputType>
        inline void write3A(const FrameConvertParameters &fc,
                            quint8 *dst_line_x,
                            quint8 *dst_line_y,
                            quint8 *dst_line_z,
                            quint8 *dst_line_a,
                            int x,
                            OutputType xo,
                            OutputType yo,
                            OutputType zo,
                            OutputType ao) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            int &xd_y = fc.dstWidthOffsetY[x];
            int &xd_z = fc.dstWidthOffsetZ[x];
            int &xd_a = fc.dstWidthOffsetA[x];

            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
            auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
            auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
            *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
            *ao_ = (*ao_ & OutputType(fc.maskAo)) | (OutputType(ao) << fc.aoShift);
        }

        template <typename OutputType>
        inline void write3A(const FrameConvertParameters &fc,
                            quint8 *dst_line_x,
                            quint8 *dst_line_y,
                            quint8 *dst_line_z,
                            quint8 *dst_line_a,
                            int x,
                            OutputType xo,
                            OutputType yo,
                            OutputType zo) const
        {
            int &xd_x = fc.dstWidthOffsetX[x];
            int &xd_y = fc.dstWidthOffsetY[x];
            int &xd_z = fc.dstWidthOffsetZ[x];
            int &xd_a = fc.dstWidthOffsetA[x];

            auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
            auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
            auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
            auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

            *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
            *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
            *ao_ = *ao_ | OutputType(fc.alphaMask);
        }

        /* Integral image functions */

        template <typename InputType>
        inline void integralImage1(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

                    if (fc.fromEndian != Q_BYTE_ORDER)
                        xi = AkAlgorithm::swapBytes(InputType(xi));

                    // Accumulate pixels in current line.

                    sumX += (xi >> fc.xiShift) & fc.maxXi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage1A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_a = fc.integralImageDataA;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_a = fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    if (fc.fromEndian != Q_BYTE_ORDER) {
                        xi = AkAlgorithm::swapBytes(InputType(xi));
                        ai = AkAlgorithm::swapBytes(InputType(ai));
                    }

                    // Accumulate pixels in current line.

                    sumX += (xi >> fc.xiShift) & fc.maxXi;
                    sumA += (ai >> fc.aiShift) & fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_a += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_a_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage3(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_y = fc.integralImageDataY;
            auto dst_line_z = fc.integralImageDataZ;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
            auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_y = fc.dlSrcWidthOffsetY[x];
                    int &xs_z = fc.dlSrcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    if (fc.fromEndian != Q_BYTE_ORDER) {
                        xi = AkAlgorithm::swapBytes(InputType(xi));
                        yi = AkAlgorithm::swapBytes(InputType(yi));
                        zi = AkAlgorithm::swapBytes(InputType(zi));
                    }

                    // Accumulate pixels in current line.

                    sumX += (xi >> fc.xiShift) & fc.maxXi;
                    sumY += (yi >> fc.yiShift) & fc.maxYi;
                    sumZ += (zi >> fc.ziShift) & fc.maxZi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_y += fc.inputWidth_1;
                dst_line_z += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_y_1 += fc.inputWidth_1;
                dst_line_z_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage3A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_y = fc.integralImageDataY;
            auto dst_line_z = fc.integralImageDataZ;
            auto dst_line_a = fc.integralImageDataA;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
            auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;
            auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_y = fc.dlSrcWidthOffsetY[x];
                    int &xs_z = fc.dlSrcWidthOffsetZ[x];
                    int &xs_a = fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    if (fc.fromEndian != Q_BYTE_ORDER) {
                        xi = AkAlgorithm::swapBytes(InputType(xi));
                        yi = AkAlgorithm::swapBytes(InputType(yi));
                        zi = AkAlgorithm::swapBytes(InputType(zi));
                        ai = AkAlgorithm::swapBytes(InputType(ai));
                    }

                    // Accumulate pixels in current line.

                    sumX += (xi >> fc.xiShift) & fc.maxXi;
                    sumY += (yi >> fc.yiShift) & fc.maxYi;
                    sumZ += (zi >> fc.ziShift) & fc.maxZi;
                    sumA += (ai >> fc.aiShift) & fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_y += fc.inputWidth_1;
                dst_line_z += fc.inputWidth_1;
                dst_line_a += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_y_1 += fc.inputWidth_1;
                dst_line_z_1 += fc.inputWidth_1;
                dst_line_a_1 += fc.inputWidth_1;
            }
        }

        /* Fast conversion functions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert3to3(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bits3to3(const FrameConvertParameters &fc,
                                  const AkVideoPacket &src,
                                  AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3to3)
                    fc.convertSIMDFast8bits3to3(fc.colorConvert,
                                                fc.srcWidthOffsetX,
                                                fc.srcWidthOffsetY,
                                                fc.srcWidthOffsetZ,
                                                fc.dstWidthOffsetX,
                                                fc.dstWidthOffsetY,
                                                fc.dstWidthOffsetZ,
                                                fc.xmin,
                                                fc.xmax,
                                                src_line_x,
                                                src_line_y,
                                                src_line_z,
                                                dst_line_x,
                                                dst_line_y,
                                                dst_line_z,
                                                &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to3A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bits3to3A(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3to3A)
                    fc.convertSIMDFast8bits3to3A(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetY,
                                                 fc.srcWidthOffsetZ,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetY,
                                                 fc.dstWidthOffsetZ,
                                                 fc.dstWidthOffsetA,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_y,
                                                 src_line_z,
                                                 dst_line_x,
                                                 dst_line_y,
                                                 dst_line_z,
                                                 dst_line_a,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bits3Ato3(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3Ato3)
                    fc.convertSIMDFast8bits3Ato3(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetY,
                                                 fc.srcWidthOffsetZ,
                                                 fc.srcWidthOffsetA,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetY,
                                                 fc.dstWidthOffsetZ,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_y,
                                                 src_line_z,
                                                 src_line_a,
                                                 dst_line_x,
                                                 dst_line_y,
                                                 dst_line_z,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bits3Ato3A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3Ato3A)
                    fc.convertSIMDFast8bits3Ato3A(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetY,
                                                  fc.srcWidthOffsetZ,
                                                  fc.srcWidthOffsetA,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetY,
                                                  fc.dstWidthOffsetZ,
                                                  fc.dstWidthOffsetA,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_y,
                                                  src_line_z,
                                                  src_line_a,
                                                  dst_line_x,
                                                  dst_line_y,
                                                  dst_line_z,
                                                  dst_line_a,
                                                  &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertV3to3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsV3to3(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bitsV3to3)
                    fc.convertSIMDFast8bitsV3to3(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetY,
                                                 fc.srcWidthOffsetZ,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetY,
                                                 fc.dstWidthOffsetZ,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_y,
                                                 src_line_z,
                                                 dst_line_x,
                                                 dst_line_y,
                                                 dst_line_z,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3to3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bitsV3to3A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bitsV3to3A)
                    fc.convertSIMDFast8bitsV3to3A(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetY,
                                                  fc.srcWidthOffsetZ,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetY,
                                                  fc.dstWidthOffsetZ,
                                                  fc.dstWidthOffsetA,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_y,
                                                  src_line_z,
                                                  dst_line_x,
                                                  dst_line_y,
                                                  dst_line_z,
                                                  dst_line_a,
                                                  &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsV3Ato3(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bitsV3Ato3)
                    fc.convertSIMDFast8bitsV3Ato3(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetY,
                                                  fc.srcWidthOffsetZ,
                                                  fc.srcWidthOffsetA,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetY,
                                                  fc.dstWidthOffsetZ,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_y,
                                                  src_line_z,
                                                  src_line_a,
                                                  dst_line_x,
                                                  dst_line_y,
                                                  dst_line_z,
                                                   &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsV3Ato3A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bitsV3Ato3A)
                    fc.convertSIMDFast8bitsV3Ato3A(fc.colorConvert,
                                                   fc.srcWidthOffsetX,
                                                   fc.srcWidthOffsetY,
                                                   fc.srcWidthOffsetZ,
                                                   fc.srcWidthOffsetA,
                                                   fc.dstWidthOffsetX,
                                                   fc.dstWidthOffsetY,
                                                   fc.dstWidthOffsetZ,
                                                   fc.dstWidthOffsetA,
                                                   fc.xmin,
                                                   fc.xmax,
                                                   src_line_x,
                                                   src_line_y,
                                                   src_line_z,
                                                   src_line_a,
                                                   dst_line_x,
                                                   dst_line_y,
                                                   dst_line_z,
                                                   dst_line_a,
                                                   &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert3to1(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bits3to1(const FrameConvertParameters &fc,
                                  const AkVideoPacket &src,
                                  AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3to1)
                    fc.convertSIMDFast8bits3to1(fc.colorConvert,
                                                fc.srcWidthOffsetX,
                                                fc.srcWidthOffsetY,
                                                fc.srcWidthOffsetZ,
                                                fc.dstWidthOffsetX,
                                                fc.xmin,
                                                fc.xmax,
                                                src_line_x,
                                                src_line_y,
                                                src_line_z,
                                                dst_line_x,
                                                &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to1A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->read3(fc,
                                src_line_x,
                                src_line_y,
                                src_line_z,
                                x,
                                &xi,
                                &yi,
                                &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }
            }
        }

        void convertFast8bits3to1A(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3to1A)
                    fc.convertSIMDFast8bits3to1A(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetY,
                                                 fc.srcWidthOffsetZ,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetA,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_y,
                                                 src_line_z,
                                                 dst_line_x,
                                                 dst_line_a,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bits3Ato1(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3Ato1)
                    fc.convertSIMDFast8bits3Ato1(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetY,
                                                 fc.srcWidthOffsetZ,
                                                 fc.srcWidthOffsetA,
                                                 fc.dstWidthOffsetX,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_y,
                                                 src_line_z,
                                                 src_line_a,
                                                 dst_line_x,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->read3A(fc,
                                 src_line_x,
                                 src_line_y,
                                 src_line_z,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &yi,
                                 &zi,
                                 &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bits3Ato1A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits3Ato1A)
                    fc.convertSIMDFast8bits3Ato1A(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetY,
                                                  fc.srcWidthOffsetZ,
                                                  fc.srcWidthOffsetA,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetA,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_y,
                                                  src_line_z,
                                                  src_line_a,
                                                  dst_line_x,
                                                  dst_line_a,
                                                  &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto yi = src_line_y[fc.srcWidthOffsetY[x]];
                    auto zi = src_line_z[fc.srcWidthOffsetZ[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert1to3(const FrameConvertParameters &fc,
                         const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->read1(fc,
                                src_line_x,
                                x,
                                &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bits1to3(const FrameConvertParameters &fc,
                                  const AkVideoPacket &src,
                                  AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1to3)
                    fc.convertSIMDFast8bits1to3(fc.colorConvert,
                                                fc.srcWidthOffsetX,
                                                fc.dstWidthOffsetX,
                                                fc.dstWidthOffsetY,
                                                fc.dstWidthOffsetZ,
                                                fc.xmin,
                                                fc.xmax,
                                                src_line_x,
                                                dst_line_x,
                                                dst_line_y,
                                                dst_line_z,
                                                &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to3A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->read1(fc,
                                src_line_x,
                                x,
                                &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bits1to3A(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1to3A)
                    fc.convertSIMDFast8bits1to3A(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetY,
                                                 fc.dstWidthOffsetZ,
                                                 fc.dstWidthOffsetA,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 dst_line_x,
                                                 dst_line_y,
                                                 dst_line_z,
                                                 dst_line_a,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->read1A(fc,
                                 src_line_x,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bits1Ato3(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1Ato3)
                    fc.convertSIMDFast8bits1Ato3(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetA,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetY,
                                                 fc.dstWidthOffsetZ,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_a,
                                                 dst_line_x,
                                                 dst_line_y,
                                                 dst_line_z,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->read1A(fc,
                                 src_line_x,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bits1Ato3A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1Ato3A)
                    fc.convertSIMDFast8bits1Ato3A(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetA,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetY,
                                                  fc.dstWidthOffsetZ,
                                                  fc.dstWidthOffsetA,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_a,
                                                  dst_line_x,
                                                  dst_line_y,
                                                  dst_line_z,
                                                  dst_line_a,
                                                  &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert1to1(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->read1(fc,
                                src_line_x,
                                x,
                                &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1(fc,
                                  dst_line_x,
                                  x,
                                  OutputType(xo));
                }
            }
        }

        void convertFast8bits1to1(const FrameConvertParameters &fc,
                                  const AkVideoPacket &src,
                                  AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1to1)
                    fc.convertSIMDFast8bits1to1(fc.colorConvert,
                                                fc.srcWidthOffsetX,
                                                fc.dstWidthOffsetX,
                                                fc.xmin,
                                                fc.xmax,
                                                src_line_x,
                                                dst_line_x,
                                                &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to1A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->read1(fc,
                                src_line_x,
                                x,
                                &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }
            }
        }

        void convertFast8bits1to1A(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1to1A)
                    fc.convertSIMDFast8bits1to1A(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.dstWidthOffsetX,
                                                 fc.dstWidthOffsetA,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 dst_line_x,
                                                 dst_line_a,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->read1A(fc,
                                 src_line_x,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bits1Ato1(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src,
                                   AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1Ato1)
                    fc.convertSIMDFast8bits1Ato1(fc.colorConvert,
                                                 fc.srcWidthOffsetX,
                                                 fc.srcWidthOffsetA,
                                                 fc.dstWidthOffsetX,
                                                 fc.xmin,
                                                 fc.xmax,
                                                 src_line_x,
                                                 src_line_a,
                                                 dst_line_x,
                                                 &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->read1A(fc,
                                 src_line_x,
                                 src_line_a,
                                 x,
                                 &xi,
                                 &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bits1Ato1A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                int x = fc.xmin;

                if (fc.convertSIMDFast8bits1Ato1A)
                    fc.convertSIMDFast8bits1Ato1A(fc.colorConvert,
                                                  fc.srcWidthOffsetX,
                                                  fc.srcWidthOffsetA,
                                                  fc.dstWidthOffsetX,
                                                  fc.dstWidthOffsetA,
                                                  fc.xmin,
                                                  fc.xmax,
                                                  src_line_x,
                                                  src_line_a,
                                                  dst_line_x,
                                                  dst_line_a,
                                                  &x);

                for (; x < fc.xmax; ++x) {
                    auto xi = src_line_x[fc.srcWidthOffsetX[x]];
                    auto ai = src_line_a[fc.srcWidthOffsetA[x]];

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        /* Linear downscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3to3(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3to3A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3Ato3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3Ato3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertDLV3to3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDLV3to3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3to3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDLV3to3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDLV3Ato3(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3A(const FrameConvertParameters &fc,
                              const AkVideoPacket &src,
                              AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDLV3Ato3A(const FrameConvertParameters &fc,
                                       const AkVideoPacket &src,
                                       AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3to1(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3to1A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readDL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  kdl,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3Ato1(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL3Ato1A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readDL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1to3(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1to3A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1Ato3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1Ato3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1to1(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1to1A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readDL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  kdl,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1Ato1(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }

                kdl += fc.inputWidth;
            }
        }

        void convertFast8bitsDL1Ato1A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            Q_UNUSED(src)
            auto kdl = fc.kdl;

            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readDL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   kdl,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }

                kdl += fc.inputWidth;
            }
        }

        /* Linear upscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL3to3(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL3to3A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL3Ato3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsUL3Ato3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertULV3to3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsULV3to3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3to3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bitsULV3to3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsULV3Ato3(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo,
                                               &yo,
                                               &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3A(const FrameConvertParameters &fc,
                              const AkVideoPacket &src,
                              AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsULV3Ato3A(const FrameConvertParameters &fc,
                                       const AkVideoPacket &src,
                                       AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo,
                                                &yo,
                                                &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL3to1(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    this->readUL3(fc,
                                  src_line_x,
                                  src_line_y,
                                  src_line_z,
                                  src_line_x_1,
                                  src_line_y_1,
                                  src_line_z_1,
                                  x,
                                  ky,
                                  &xi,
                                  &yi,
                                  &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL3to1A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    this->readF8UL3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    src_line_x_1,
                                    src_line_y_1,
                                    src_line_z_1,
                                    x,
                                    ky,
                                    &xi,
                                    &yi,
                                    &zi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL3Ato1(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType yi;
                    InputType zi;
                    InputType ai;
                    this->readUL3A(fc,
                                   src_line_x,
                                   src_line_y,
                                   src_line_z,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_y_1,
                                   src_line_z_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &yi,
                                   &zi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsUL3Ato1A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 yi;
                    quint8 zi;
                    quint8 ai;
                    this->readF8UL3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_y_1,
                                     src_line_z_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi,
                                               yi,
                                               zi,
                                               &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readUL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  ky,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL1to3(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readF8UL1(fc,
                                    src_line_x,
                                    src_line_x_1,
                                    x,
                                    ky,
                                    &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readUL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  ky,
                                  &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL1to3A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readF8UL1(fc,
                                    src_line_x,
                                    src_line_x_1,
                                    x,
                                    ky,
                                    &xi);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readUL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(xi, &xo, &yo, &zo);

                    this->write3(fc,
                                 dst_line_x,
                                 dst_line_y,
                                 dst_line_z,
                                 x,
                                 OutputType(xo),
                                 OutputType(yo),
                                 OutputType(zo));
                }
            }
        }

        void convertFast8bitsUL1Ato3(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readF8UL1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                    fc.colorConvert.applyAlpha(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readUL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    this->write3A(fc,
                                  dst_line_x,
                                  dst_line_y,
                                  dst_line_z,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(yo),
                                  OutputType(zo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsUL1Ato3A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readF8UL1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &ai);

                    qint64 xo = 0;
                    qint64 yo = 0;
                    qint64 zo = 0;
                    fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_y[fc.dstWidthOffsetY[x]] = quint8(yo);
                    dst_line_z[fc.dstWidthOffsetZ[x]] = quint8(zo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readUL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  ky,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL1to1(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src,
                                    AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readF8UL1(fc,
                                    src_line_x,
                                    src_line_x_1,
                                    x,
                                    ky,
                                    &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    this->readUL1(fc,
                                  src_line_x,
                                  src_line_x_1,
                                  x,
                                  ky,
                                  &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL1to1A(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    this->readF8UL1(fc,
                                    src_line_x,
                                    src_line_x_1,
                                    x,
                                    ky,
                                    &xi);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readUL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    this->write1(fc,
                                 dst_line_x,
                                 x,
                                 OutputType(xo));
                }
            }
        }

        void convertFast8bitsUL1Ato1(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readF8UL1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);
                    fc.colorConvert.applyAlpha(ai, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    InputType xi;
                    InputType ai;
                    this->readUL1A(fc,
                                   src_line_x,
                                   src_line_a,
                                   src_line_x_1,
                                   src_line_a_1,
                                   x,
                                   ky,
                                   &xi,
                                   &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    this->write1A(fc,
                                  dst_line_x,
                                  dst_line_a,
                                  x,
                                  OutputType(xo),
                                  OutputType(ai));
                }
            }
        }

        void convertFast8bitsUL1Ato1A(const FrameConvertParameters &fc,
                                      const AkVideoPacket &src,
                                      AkVideoPacket &dst) const
        {
            for (int y = fc.ymin; y < fc.ymax; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = fc.xmin; x < fc.xmax; ++x) {
                    quint8 xi;
                    quint8 ai;
                    this->readF8UL1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     src_line_x_1,
                                     src_line_a_1,
                                     x,
                                     ky,
                                     &xi,
                                     &ai);

                    qint64 xo = 0;
                    fc.colorConvert.applyPoint(xi, &xo);

                    dst_line_x[fc.dstWidthOffsetX[x]] = quint8(xo);
                    dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                }
            }
        }

#define CONVERT_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                const AkVideoPacket &src, \
                                                                AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convert##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convert##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convert##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convert##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FAST_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bits##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                         const AkVideoPacket &src, \
                                                                         AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bits##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bits##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bits##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bits##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertVFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                 const AkVideoPacket &src, \
                                                                 AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FASTV_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bitsV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                          const AkVideoPacket &src, \
                                                                          AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bitsV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bitsV##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bitsV##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bitsV##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTDL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormatDL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                  const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
            case ConvertAlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertDL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertDL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertDL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertDL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FASTDL_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bitsDL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                           const AkVideoPacket &src, \
                                                                           AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
            case ConvertAlphaMode_AI_O: \
                this->integralImage##icomponents##A<quint8>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<quint8>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bitsDL##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bitsDL##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bitsDL##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bitsDL##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTDLV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormatDLV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                   const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
            case ConvertAlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertDLV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertDLV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertDLV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertDLV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FASTDLV_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bitsDLV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                            const AkVideoPacket &src, \
                                                                            AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
            case ConvertAlphaMode_AI_O: \
                this->integralImage##icomponents##A<quint8>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<quint8>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bitsDLV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bitsDLV##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bitsDLV##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bitsDLV##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTUL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormatUL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                  const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertUL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertUL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertUL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertUL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FASTUL_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bitsUL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                           const AkVideoPacket &src, \
                                                                           AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bitsUL##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bitsUL##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bitsUL##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bitsUL##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTULV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormatULV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                   const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertULV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertULV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertULV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertULV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERT_FASTULV_FUNC(icomponents, ocomponents) \
        inline void convertFormatFast8bitsULV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                            const AkVideoPacket &src, \
                                                                            AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case ConvertAlphaMode_AI_AO: \
                this->convertFast8bitsULV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_AI_O: \
                this->convertFast8bitsULV##icomponents##Ato##ocomponents(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_AO: \
                this->convertFast8bitsULV##icomponents##to##ocomponents##A(fc, src, dst); \
                break; \
            case ConvertAlphaMode_I_O: \
                this->convertFast8bitsULV##icomponents##to##ocomponents(fc, src, dst); \
                break; \
            }; \
        }

        CONVERT_FUNC(3, 3)
        CONVERT_FUNC(3, 1)
        CONVERT_FUNC(1, 3)
        CONVERT_FUNC(1, 1)
        CONVERTV_FUNC(3, 3)
        CONVERTDL_FUNC(3, 3)
        CONVERTDL_FUNC(3, 1)
        CONVERTDL_FUNC(1, 3)
        CONVERTDL_FUNC(1, 1)
        CONVERTDLV_FUNC(3, 3)
        CONVERTUL_FUNC(3, 3)
        CONVERTUL_FUNC(3, 1)
        CONVERTUL_FUNC(1, 3)
        CONVERTUL_FUNC(1, 1)
        CONVERTULV_FUNC(3, 3)

        CONVERT_FAST_FUNC(3, 3)
        CONVERT_FAST_FUNC(3, 1)
        CONVERT_FAST_FUNC(1, 3)
        CONVERT_FAST_FUNC(1, 1)
        CONVERT_FASTV_FUNC(3, 3)
        CONVERT_FASTDL_FUNC(3, 3)
        CONVERT_FASTDL_FUNC(3, 1)
        CONVERT_FASTDL_FUNC(1, 3)
        CONVERT_FASTDL_FUNC(1, 1)
        CONVERT_FASTDLV_FUNC(3, 3)
        CONVERT_FASTUL_FUNC(3, 3)
        CONVERT_FASTUL_FUNC(3, 1)
        CONVERT_FASTUL_FUNC(1, 3)
        CONVERT_FASTUL_FUNC(1, 1)
        CONVERT_FASTULV_FUNC(3, 3)

        template <typename InputType, typename OutputType>
        inline void convert(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst)
        {
            if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                && fc.resizeMode == ResizeMode_Up) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertFormatULV3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormatUL3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormatUL3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormatUL1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormatUL1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            } else if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                       && fc.resizeMode == ResizeMode_Down) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertFormatDLV3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormatDL3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormatDL3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormatDL1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormatDL1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            } else {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertVFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormat3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormat1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormat1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            }
        }

        inline void convertFast8bits(const FrameConvertParameters &fc,
                                     const AkVideoPacket &src,
                                     AkVideoPacket &dst)
        {
            if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                && fc.resizeMode == ResizeMode_Up) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertFormatFast8bitsULV3to3(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormatFast8bitsUL3to3(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormatFast8bitsUL3to1(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormatFast8bitsUL1to3(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormatFast8bitsUL1to1(fc, src, dst);
                    break;
                }
            } else if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                       && fc.resizeMode == ResizeMode_Down) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertFormatFast8bitsDLV3to3(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormatFast8bitsDL3to3(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormatFast8bitsDL3to1(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormatFast8bitsDL1to3(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormatFast8bitsDL1to1(fc, src, dst);
                    break;
                }
            } else {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertFormatFast8bitsV3to3(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormatFast8bits3to3(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormatFast8bits3to1(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormatFast8bits1to3(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormatFast8bits1to1(fc, src, dst);
                    break;
                }
            }
        }

        inline AkVideoPacket convert(const AkVideoPacket &packet,
                                     const AkVideoCaps &ocaps);
};

AkVideoConverter::AkVideoConverter(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoConverterPrivate();
}

AkVideoConverter::AkVideoConverter(const AkVideoCaps &outputCaps,
                                   QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = outputCaps;
}

AkVideoConverter::AkVideoConverter(const AkVideoConverter &other):
    QObject()
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = other.d->m_outputCaps;
    this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
    this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
    this->d->m_scalingMode = other.d->m_scalingMode;
    this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
    this->d->m_inputRect = other.d->m_inputRect;
}

AkVideoConverter::~AkVideoConverter()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    delete this->d;
}

AkVideoConverter &AkVideoConverter::operator =(const AkVideoConverter &other)
{
    if (this != &other) {
        this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
        this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
        this->d->m_outputCaps = other.d->m_outputCaps;
        this->d->m_scalingMode = other.d->m_scalingMode;
        this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
        this->d->m_inputRect = other.d->m_inputRect;
    }

    return *this;
}

QObject *AkVideoConverter::create()
{
    return new AkVideoConverter();
}

AkVideoCaps AkVideoConverter::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkColorConvert::YuvColorSpace AkVideoConverter::yuvColorSpace() const
{
    return this->d->m_yuvColorSpace;
}

AkColorConvert::YuvColorSpaceType AkVideoConverter::yuvColorSpaceType() const
{
    return this->d->m_yuvColorSpaceType;
}

AkVideoConverter::ScalingMode AkVideoConverter::scalingMode() const
{
    return this->d->m_scalingMode;
}

AkVideoConverter::AspectRatioMode AkVideoConverter::aspectRatioMode() const
{
    return this->d->m_aspectRatioMode;
}

QRect AkVideoConverter::inputRect() const
{
    return this->d->m_inputRect;
}

bool AkVideoConverter::begin()
{
    this->d->m_cacheIndex = 0;

    return true;
}

void AkVideoConverter::end()
{
    this->d->m_cacheIndex = 0;
}

AkVideoPacket AkVideoConverter::convert(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    auto &caps = packet.caps();

    if (caps.format() == this->d->m_outputCaps.format()
        && caps.width() == this->d->m_outputCaps.width()
        && caps.height() == this->d->m_outputCaps.height()
        && this->d->m_inputRect.isEmpty())
        return packet;

    return this->d->convert(packet, this->d->m_outputCaps);
}

void AkVideoConverter::setCacheIndex(int index)
{
    this->d->m_cacheIndex = index;
}

void AkVideoConverter::setOutputCaps(const AkVideoCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_mutex.lock();
    this->d->m_outputCaps = outputCaps;
    this->d->m_mutex.unlock();
    emit this->outputCapsChanged(outputCaps);
}

void AkVideoConverter::setYuvColorSpace(AkColorConvert::YuvColorSpace yuvColorSpace)
{
    if (this->d->m_yuvColorSpace == yuvColorSpace)
        return;

    this->d->m_yuvColorSpace = yuvColorSpace;
    emit this->yuvColorSpaceChanged(yuvColorSpace);
}

void AkVideoConverter::setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    if (this->d->m_yuvColorSpaceType == yuvColorSpaceType)
        return;

    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
    emit this->yuvColorSpaceTypeChanged(yuvColorSpaceType);
}

void AkVideoConverter::setScalingMode(AkVideoConverter::ScalingMode scalingMode)
{
    if (this->d->m_scalingMode == scalingMode)
        return;

    this->d->m_scalingMode = scalingMode;
    emit this->scalingModeChanged(scalingMode);
}

void AkVideoConverter::setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    if (this->d->m_aspectRatioMode == aspectRatioMode)
        return;

    this->d->m_aspectRatioMode = aspectRatioMode;
    emit this->aspectRatioModeChanged(aspectRatioMode);
}

void AkVideoConverter::setInputRect(const QRect &inputRect)
{
    if (this->d->m_inputRect == inputRect)
        return;

    this->d->m_inputRect = inputRect;
    emit this->inputRectChanged(inputRect);
}

void AkVideoConverter::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkVideoConverter::resetYuvColorSpace()
{
    this->setYuvColorSpace(AkColorConvert::YuvColorSpace_ITUR_BT601);
}

void AkVideoConverter::resetYuvColorSpaceType()
{
    this->setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType_StudioSwing);
}

void AkVideoConverter::resetScalingMode()
{
    this->setScalingMode(ScalingMode_Fast);
}

void AkVideoConverter::resetAspectRatioMode()
{
    this->setAspectRatioMode(AspectRatioMode_Ignore);
}

void AkVideoConverter::resetInputRect()
{
    this->setInputRect({});
}

void AkVideoConverter::reset()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    this->d->m_fcSize = 0;
}

void AkVideoConverter::registerTypes()
{
    qRegisterMetaType<AkVideoConverter>("AkVideoConverter");
    qRegisterMetaType<ScalingMode>("ScalingMode");
    qRegisterMetaType<AspectRatioMode>("AspectRatioMode");
    qmlRegisterSingletonType<AkVideoConverter>("Ak", 1, 0, "AkVideoConverter",
                                               [] (QQmlEngine *qmlEngine,
                                                   QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoConverter();
    });
}

QDebug operator <<(QDebug debug, AkVideoConverter::ScalingMode mode)
{
    AkVideoConverter converter;
    int scalingModeIndex = converter.metaObject()->indexOfEnumerator("ScalingMode");
    QMetaEnum scalingModeEnum = converter.metaObject()->enumerator(scalingModeIndex);
    QString scalingModeStr(scalingModeEnum.valueToKey(mode));
    scalingModeStr.remove("ScalingMode_");
    QDebugStateSaver saver(debug);
    debug.nospace() << scalingModeStr.toStdString().c_str();

    return debug;
}

QDebug operator <<(QDebug debug, AkVideoConverter::AspectRatioMode mode)
{
    AkVideoConverter converter;
    int aspectRatioModeIndex = converter.metaObject()->indexOfEnumerator("AspectRatioMode");
    QMetaEnum aspectRatioModeEnum = converter.metaObject()->enumerator(aspectRatioModeIndex);
    QString aspectRatioModeStr(aspectRatioModeEnum.valueToKey(mode));
    aspectRatioModeStr.remove("AspectRatioMode_");
    QDebugStateSaver saver(debug);
    debug.nospace() << aspectRatioModeStr.toStdString().c_str();

    return debug;
}

#define DEFINE_CONVERT_FUNC(isize, osize) \
    case ConvertDataTypes_##isize##_##osize: \
        this->convert<quint##isize, quint##osize>(fc, \
                                                  packet, \
                                                  fc.outputFrame); \
        \
        if (fc.toEndian != Q_BYTE_ORDER) \
            AkAlgorithm::swapDataBytes(reinterpret_cast<quint##osize *>(fc.outputFrame.data()), fc.outputFrame.size()); \
        \
        break;

AkVideoPacket AkVideoConverterPrivate::convert(const AkVideoPacket &packet,
                                               const AkVideoCaps &ocaps)
{
    static const int maxCacheAlloc = 1 << 16;

    if (this->m_cacheIndex >= this->m_fcSize) {
        static const int cacheBlockSize = 8;
        auto newSize = qBound(cacheBlockSize, this->m_cacheIndex + cacheBlockSize, maxCacheAlloc);
        auto fc = new FrameConvertParameters[newSize];

        if (this->m_fc) {
            for (int i = 0; i < this->m_fcSize; ++i)
                fc[i] = this->m_fc[i];

            delete [] this->m_fc;
        }

        this->m_fc = fc;
        this->m_fcSize = newSize;
    }

    if (this->m_cacheIndex >= maxCacheAlloc)
        return {};

    auto &fc = this->m_fc[this->m_cacheIndex];

    if (packet.caps() != fc.inputCaps
        || ocaps != fc.outputCaps
        || this->m_yuvColorSpace != fc.yuvColorSpace
        || this->m_yuvColorSpaceType != fc.yuvColorSpaceType
        || this->m_scalingMode != fc.scalingMode
        || this->m_aspectRatioMode != fc.aspectRatioMode
        || this->m_inputRect != fc.inputRect) {
        fc.configure(packet.caps(),
                     ocaps,
                     fc.colorConvert,
                     this->m_yuvColorSpace,
                     this->m_yuvColorSpaceType);
        fc.configureScaling(packet.caps(),
                            ocaps,
                            this->m_inputRect,
                            this->m_aspectRatioMode);
        fc.inputCaps = packet.caps();
        fc.outputCaps = ocaps;
        fc.yuvColorSpace = this->m_yuvColorSpace;
        fc.yuvColorSpaceType = this->m_yuvColorSpaceType;
        fc.scalingMode = this->m_scalingMode;
        fc.aspectRatioMode = this->m_aspectRatioMode;
        fc.inputRect = this->m_inputRect;
    }

    if (fc.outputConvertCaps.isSameFormat(packet.caps())) {
        this->m_cacheIndex++;

        return packet;
    }

    if (fc.fastConvertion) {
        this->convertFast8bits(fc, packet, fc.outputFrame);
    } else {
        switch (fc.convertDataTypes) {
        DEFINE_CONVERT_FUNC(8 , 8 )
        DEFINE_CONVERT_FUNC(8 , 16)
        DEFINE_CONVERT_FUNC(8 , 32)
        DEFINE_CONVERT_FUNC(16, 8 )
        DEFINE_CONVERT_FUNC(16, 16)
        DEFINE_CONVERT_FUNC(16, 32)
        DEFINE_CONVERT_FUNC(32, 8 )
        DEFINE_CONVERT_FUNC(32, 16)
        DEFINE_CONVERT_FUNC(32, 32)
        }
    }

    fc.outputFrame.copyMetadata(packet);
    this->m_cacheIndex++;

    return fc.outputFrame;
}

FrameConvertParameters::FrameConvertParameters()
{
}

FrameConvertParameters::FrameConvertParameters(const FrameConvertParameters &other):
    inputCaps(other.inputCaps),
    outputCaps(other.outputCaps),
    outputConvertCaps(other.outputConvertCaps),
    outputFrame(other.outputFrame),
    scalingMode(other.scalingMode),
    aspectRatioMode(other.aspectRatioMode),
    convertType(other.convertType),
    convertDataTypes(other.convertDataTypes),
    alphaMode(other.alphaMode),
    resizeMode(other.resizeMode),
    fastConvertion(other.fastConvertion),
    fromEndian(other.fromEndian),
    toEndian(other.toEndian),
    xmin(other.xmin),
    ymin(other.ymin),
    xmax(other.xmax),
    ymax(other.ymax),
    inputWidth(other.inputWidth),
    inputWidth_1(other.inputWidth_1),
    inputHeight(other.inputHeight),
    planeXi(other.planeXi),
    planeYi(other.planeYi),
    planeZi(other.planeZi),
    planeAi(other.planeAi),
    compXi(other.compXi),
    compYi(other.compYi),
    compZi(other.compZi),
    compAi(other.compAi),
    planeXo(other.planeXo),
    planeYo(other.planeYo),
    planeZo(other.planeZo),
    planeAo(other.planeAo),
    compXo(other.compXo),
    compYo(other.compYo),
    compZo(other.compZo),
    compAo(other.compAo),
    xiOffset(other.xiOffset),
    yiOffset(other.yiOffset),
    ziOffset(other.ziOffset),
    aiOffset(other.aiOffset),
    xoOffset(other.xoOffset),
    yoOffset(other.yoOffset),
    zoOffset(other.zoOffset),
    aoOffset(other.aoOffset),
    xiShift(other.xiShift),
    yiShift(other.yiShift),
    ziShift(other.ziShift),
    aiShift(other.aiShift),
    xoShift(other.xoShift),
    yoShift(other.yoShift),
    zoShift(other.zoShift),
    aoShift(other.aoShift),
    maxXi(other.maxXi),
    maxYi(other.maxYi),
    maxZi(other.maxZi),
    maxAi(other.maxAi),
    maskXo(other.maskXo),
    maskYo(other.maskYo),
    maskZo(other.maskZo),
    maskAo(other.maskAo),
    alphaMask(other.alphaMask)
{
    auto oWidth = this->outputCaps.width();
    auto oHeight = this->outputCaps.height();

    size_t oWidthDataSize = sizeof(int) * oWidth;
    size_t oHeightDataSize = sizeof(int) * oHeight;

    if (other.srcWidth) {
        this->srcWidth = new int [oWidth];
        memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
    }

    if (other.srcWidth_1) {
        this->srcWidth_1 = new int [oWidth];
        memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetX) {
        this->srcWidthOffsetX = new int [oWidth];
        memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
    }

    if (other.srcWidthOffsetY) {
        this->srcWidthOffsetY = new int [oWidth];
        memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ) {
        this->srcWidthOffsetZ = new int [oWidth];
        memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
    }

    if (other.srcWidthOffsetA) {
        this->srcWidthOffsetA = new int [oWidth];
        memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
    }

    if (other.srcHeight) {
        this->srcHeight = new int [oHeight];
        memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
    }

    auto iWidth = this->inputCaps.width();
    size_t iWidthDataSize = sizeof(int) * iWidth;

    if (other.dlSrcWidthOffsetX) {
        this->dlSrcWidthOffsetX = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetY) {
        this->dlSrcWidthOffsetY = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetZ) {
        this->dlSrcWidthOffsetZ = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetA) {
        this->dlSrcWidthOffsetA = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
    }

    if (other.srcWidthOffsetX_1) {
        this->srcWidthOffsetX_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetY_1) {
        this->srcWidthOffsetY_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ_1) {
        this->srcWidthOffsetZ_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetA_1) {
        this->srcWidthOffsetA_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
    }

    if (other.srcHeight_1) {
        this->srcHeight_1 = new int [oHeight];
        memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
    }

    if (other.dstWidthOffsetX) {
        this->dstWidthOffsetX = new int [oWidth];
        memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
    }

    if (other.dstWidthOffsetY) {
        this->dstWidthOffsetY = new int [oWidth];
        memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
    }

    if (other.dstWidthOffsetZ) {
        this->dstWidthOffsetZ = new int [oWidth];
        memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
    }

    if (other.dstWidthOffsetA) {
        this->dstWidthOffsetA = new int [oWidth];
        memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
    }

    size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

    if (other.srcHeightDlOffset) {
        this->srcHeightDlOffset = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
    }

    if (other.srcHeightDlOffset_1) {
        this->srcHeightDlOffset_1 = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
    }

    size_t iWidth_1 = this->inputCaps.width() + 1;
    size_t iHeight_1 = this->inputCaps.height() + 1;
    size_t integralImageSize = iWidth_1 * iHeight_1;
    size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

    if (other.integralImageDataX) {
        this->integralImageDataX = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
    }

    if (other.integralImageDataY) {
        this->integralImageDataY = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
    }

    if (other.integralImageDataZ) {
        this->integralImageDataZ = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
    }

    if (other.integralImageDataA) {
        this->integralImageDataA = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
    }

    if (other.kx) {
        this->kx = new qint64 [oWidth];
        memcpy(this->kx, other.kx, sizeof(qint64) * oWidth);
    }

    if (other.ky) {
        this->ky = new qint64 [oHeight];
        memcpy(this->ky, other.ky, sizeof(qint64) * oHeight);
    }

    auto kdlSize = size_t(this->inputCaps.width()) * this->inputCaps.height();
    auto kdlDataSize = sizeof(DlSumType) * kdlSize;

    if (other.kdl) {
        this->kdl = new DlSumType [kdlSize];
        memcpy(this->kdl, other.kdl, kdlDataSize);
    }
}

FrameConvertParameters::~FrameConvertParameters()
{
    this->clearBuffers();
    this->clearDlBuffers();
}

FrameConvertParameters &FrameConvertParameters::operator =(const FrameConvertParameters &other)
{
    if (this != &other) {
        this->inputCaps = other.inputCaps;
        this->outputCaps = other.outputCaps;
        this->outputConvertCaps = other.outputConvertCaps;
        this->outputFrame = other.outputFrame;
        this->scalingMode = other.scalingMode;
        this->aspectRatioMode = other.aspectRatioMode;
        this->convertType = other.convertType;
        this->convertDataTypes = other.convertDataTypes;
        this->alphaMode = other.alphaMode;
        this->resizeMode = other.resizeMode;
        this->fastConvertion = other.fastConvertion;
        this->fromEndian = other.fromEndian;
        this->toEndian = other.toEndian;
        this->xmin = other.xmin;
        this->ymin = other.ymin;
        this->xmax = other.xmax;
        this->ymax = other.ymax;
        this->inputWidth = other.inputWidth;
        this->inputWidth_1 = other.inputWidth_1;
        this->inputHeight = other.inputHeight;
        this->planeXi = other.planeXi;
        this->planeYi = other.planeYi;
        this->planeZi = other.planeZi;
        this->planeAi = other.planeAi;
        this->compXi = other.compXi;
        this->compYi = other.compYi;
        this->compZi = other.compZi;
        this->compAi = other.compAi;
        this->planeXo = other.planeXo;
        this->planeYo = other.planeYo;
        this->planeZo = other.planeZo;
        this->planeAo = other.planeAo;
        this->compXo = other.compXo;
        this->compYo = other.compYo;
        this->compZo = other.compZo;
        this->compAo = other.compAo;
        this->xiOffset = other.xiOffset;
        this->yiOffset = other.yiOffset;
        this->ziOffset = other.ziOffset;
        this->aiOffset = other.aiOffset;
        this->xoOffset = other.xoOffset;
        this->yoOffset = other.yoOffset;
        this->zoOffset = other.zoOffset;
        this->aoOffset = other.aoOffset;
        this->xiShift = other.xiShift;
        this->yiShift = other.yiShift;
        this->ziShift = other.ziShift;
        this->aiShift = other.aiShift;
        this->xoShift = other.xoShift;
        this->yoShift = other.yoShift;
        this->zoShift = other.zoShift;
        this->aoShift = other.aoShift;
        this->maxXi = other.maxXi;
        this->maxYi = other.maxYi;
        this->maxZi = other.maxZi;
        this->maxAi = other.maxAi;
        this->maskXo = other.maskXo;
        this->maskYo = other.maskYo;
        this->maskZo = other.maskZo;
        this->maskAo = other.maskAo;
        this->alphaMask = other.alphaMask;

        this->clearBuffers();
        this->clearDlBuffers();

        auto oWidth = this->outputCaps.width();
        auto oHeight = this->outputCaps.height();

        size_t oWidthDataSize = sizeof(int) * oWidth;
        size_t oHeightDataSize = sizeof(int) * oHeight;

        if (other.srcWidth) {
            this->srcWidth = new int [oWidth];
            memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
        }

        if (other.srcWidth_1) {
            this->srcWidth_1 = new int [oWidth];
            memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetX) {
            this->srcWidthOffsetX = new int [oWidth];
            memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
        }

        if (other.srcWidthOffsetY) {
            this->srcWidthOffsetY = new int [oWidth];
            memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ) {
            this->srcWidthOffsetZ = new int [oWidth];
            memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
        }

        if (other.srcWidthOffsetA) {
            this->srcWidthOffsetA = new int [oWidth];
            memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
        }

        if (other.srcHeight) {
            this->srcHeight = new int [oHeight];
            memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
        }

        auto iWidth = this->inputCaps.width();
        size_t iWidthDataSize = sizeof(int) * iWidth;

        if (other.dlSrcWidthOffsetX) {
            this->dlSrcWidthOffsetX = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetY) {
            this->dlSrcWidthOffsetY = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetZ) {
            this->dlSrcWidthOffsetZ = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetA) {
            this->dlSrcWidthOffsetA = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
        }

        if (other.srcWidthOffsetX_1) {
            this->srcWidthOffsetX_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetY_1) {
            this->srcWidthOffsetY_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ_1) {
            this->srcWidthOffsetZ_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetA_1) {
            this->srcWidthOffsetA_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
        }

        if (other.srcHeight_1) {
            this->srcHeight_1 = new int [oHeight];
            memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
        }

        if (other.dstWidthOffsetX) {
            this->dstWidthOffsetX = new int [oWidth];
            memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
        }

        if (other.dstWidthOffsetY) {
            this->dstWidthOffsetY = new int [oWidth];
            memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
        }

        if (other.dstWidthOffsetZ) {
            this->dstWidthOffsetZ = new int [oWidth];
            memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
        }

        if (other.dstWidthOffsetA) {
            this->dstWidthOffsetA = new int [oWidth];
            memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
        }

        size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

        if (other.srcHeightDlOffset) {
            this->srcHeightDlOffset = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
        }

        if (other.srcHeightDlOffset_1) {
            this->srcHeightDlOffset_1 = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
        }

        size_t iWidth_1 = this->inputCaps.width() + 1;
        size_t iHeight_1 = this->inputCaps.height() + 1;
        size_t integralImageSize = iWidth_1 * iHeight_1;
        size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

        if (other.integralImageDataX) {
            this->integralImageDataX = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
        }

        if (other.integralImageDataY) {
            this->integralImageDataY = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
        }

        if (other.integralImageDataZ) {
            this->integralImageDataZ = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
        }

        if (other.integralImageDataA) {
            this->integralImageDataA = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
        }

        if (other.kx) {
            this->kx = new qint64 [oWidth];
            memcpy(this->kx, other.kx, sizeof(qint64) * oWidth);
        }

        if (other.ky) {
            this->ky = new qint64 [oHeight];
            memcpy(this->ky, other.ky, sizeof(qint64) * oHeight);
        }

        auto kdlSize = size_t(this->inputCaps.width()) * this->inputCaps.height();
        auto kdlDataSize = sizeof(DlSumType) * kdlSize;

        if (other.kdl) {
            this->kdl = new DlSumType [kdlSize];
            memcpy(this->kdl, other.kdl, kdlDataSize);
        }
    }

    return *this;
}

void FrameConvertParameters::clearBuffers()
{
    if (this->srcWidth) {
        delete [] this->srcWidth;
        this->srcWidth = nullptr;
    }

    if (this->srcWidth_1) {
        delete [] this->srcWidth_1;
        this->srcWidth_1 = nullptr;
    }

    if (this->srcWidthOffsetX) {
        delete [] this->srcWidthOffsetX;
        this->srcWidthOffsetX = nullptr;
    }

    if (this->srcWidthOffsetY) {
        delete [] this->srcWidthOffsetY;
        this->srcWidthOffsetY = nullptr;
    }

    if (this->srcWidthOffsetZ) {
        delete [] this->srcWidthOffsetZ;
        this->srcWidthOffsetZ = nullptr;
    }

    if (this->srcWidthOffsetA) {
        delete [] this->srcWidthOffsetA;
        this->srcWidthOffsetA = nullptr;
    }

    if (this->srcHeight) {
        delete [] this->srcHeight;
        this->srcHeight = nullptr;
    }

    if (this->srcWidthOffsetX_1) {
        delete [] this->srcWidthOffsetX_1;
        this->srcWidthOffsetX_1 = nullptr;
    }

    if (this->srcWidthOffsetY_1) {
        delete [] this->srcWidthOffsetY_1;
        this->srcWidthOffsetY_1 = nullptr;
    }

    if (this->srcWidthOffsetZ_1) {
        delete [] this->srcWidthOffsetZ_1;
        this->srcWidthOffsetZ_1 = nullptr;
    }

    if (this->srcWidthOffsetA_1) {
        delete [] this->srcWidthOffsetA_1;
        this->srcWidthOffsetA_1 = nullptr;
    }

    if (this->srcHeight_1) {
        delete [] this->srcHeight_1;
        this->srcHeight_1 = nullptr;
    }

    if (this->dstWidthOffsetX) {
        delete [] this->dstWidthOffsetX;
        this->dstWidthOffsetX = nullptr;
    }

    if (this->dstWidthOffsetY) {
        delete [] this->dstWidthOffsetY;
        this->dstWidthOffsetY = nullptr;
    }

    if (this->dstWidthOffsetZ) {
        delete [] this->dstWidthOffsetZ;
        this->dstWidthOffsetZ = nullptr;
    }

    if (this->dstWidthOffsetA) {
        delete [] this->dstWidthOffsetA;
        this->dstWidthOffsetA = nullptr;
    }

    if (this->kx) {
        delete [] this->kx;
        this->kx = nullptr;
    }

    if (this->ky) {
        delete [] this->ky;
        this->ky = nullptr;
    }
}

void FrameConvertParameters::clearDlBuffers()
{
    if (this->integralImageDataX) {
        delete [] this->integralImageDataX;
        this->integralImageDataX = nullptr;
    }

    if (this->integralImageDataY) {
        delete [] this->integralImageDataY;
        this->integralImageDataY = nullptr;
    }

    if (this->integralImageDataZ) {
        delete [] this->integralImageDataZ;
        this->integralImageDataZ = nullptr;
    }

    if (this->integralImageDataA) {
        delete [] this->integralImageDataA;
        this->integralImageDataA = nullptr;
    }

    if (this->kdl) {
        delete [] this->kdl;
        this->kdl = nullptr;
    }

    if (this->srcHeightDlOffset) {
        delete [] this->srcHeightDlOffset;
        this->srcHeightDlOffset = nullptr;
    }

    if (this->srcHeightDlOffset_1) {
        delete [] this->srcHeightDlOffset_1;
        this->srcHeightDlOffset_1 = nullptr;
    }

    if (this->dlSrcWidthOffsetX) {
        delete [] this->dlSrcWidthOffsetX;
        this->dlSrcWidthOffsetX = nullptr;
    }

    if (this->dlSrcWidthOffsetY) {
        delete [] this->dlSrcWidthOffsetY;
        this->dlSrcWidthOffsetY = nullptr;
    }

    if (this->dlSrcWidthOffsetZ) {
        delete [] this->dlSrcWidthOffsetZ;
        this->dlSrcWidthOffsetZ = nullptr;
    }

    if (this->dlSrcWidthOffsetA) {
        delete [] this->dlSrcWidthOffsetA;
        this->dlSrcWidthOffsetA = nullptr;
    }
}

void FrameConvertParameters::allocateBuffers(const AkVideoCaps &ocaps)
{
    this->clearBuffers();

    this->srcWidth = new int [ocaps.width()];
    this->srcWidth_1 = new int [ocaps.width()];
    this->srcWidthOffsetX = new int [ocaps.width()];
    this->srcWidthOffsetY = new int [ocaps.width()];
    this->srcWidthOffsetZ = new int [ocaps.width()];
    this->srcWidthOffsetA = new int [ocaps.width()];
    this->srcHeight = new int [ocaps.height()];

    this->srcWidthOffsetX_1 = new int [ocaps.width()];
    this->srcWidthOffsetY_1 = new int [ocaps.width()];
    this->srcWidthOffsetZ_1 = new int [ocaps.width()];
    this->srcWidthOffsetA_1 = new int [ocaps.width()];
    this->srcHeight_1 = new int [ocaps.height()];

    this->dstWidthOffsetX = new int [ocaps.width()];
    this->dstWidthOffsetY = new int [ocaps.width()];
    this->dstWidthOffsetZ = new int [ocaps.width()];
    this->dstWidthOffsetA = new int [ocaps.width()];

    this->kx = new qint64 [ocaps.width()];
    this->ky = new qint64 [ocaps.height()];
}

void FrameConvertParameters::allocateDlBuffers(const AkVideoCaps &icaps,
                                               const AkVideoCaps &ocaps)
{
    size_t width_1 = icaps.width() + 1;
    size_t height_1 = icaps.height() + 1;
    auto integralImageSize = width_1 * height_1;

    this->integralImageDataX = new DlSumType [integralImageSize];
    this->integralImageDataY = new DlSumType [integralImageSize];
    this->integralImageDataZ = new DlSumType [integralImageSize];
    this->integralImageDataA = new DlSumType [integralImageSize];
    memset(this->integralImageDataX, 0, integralImageSize);
    memset(this->integralImageDataY, 0, integralImageSize);
    memset(this->integralImageDataZ, 0, integralImageSize);
    memset(this->integralImageDataA, 0, integralImageSize);

    auto kdlSize = size_t(icaps.width()) * icaps.height();
    this->kdl = new DlSumType [kdlSize];
    memset(this->kdl, 0, kdlSize);

    this->srcHeightDlOffset = new size_t [ocaps.height()];
    this->srcHeightDlOffset_1 = new size_t [ocaps.height()];

    this->dlSrcWidthOffsetX = new int [icaps.width()];
    this->dlSrcWidthOffsetY = new int [icaps.width()];
    this->dlSrcWidthOffsetZ = new int [icaps.width()];
    this->dlSrcWidthOffsetA = new int [icaps.width()];
}

#define DEFINE_CONVERT_TYPES(isize, osize) \
    if (ispecs.depth() == isize && ospecs.depth() == osize) \
        this->convertDataTypes = ConvertDataTypes_##isize##_##osize;

void FrameConvertParameters::configure(const AkVideoCaps &icaps,
                                       const AkVideoCaps &ocaps,
                                       AkColorConvert &colorConvert,
                                       AkColorConvert::YuvColorSpace yuvColorSpace,
                                       AkColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    auto ispecs = AkVideoCaps::formatSpecs(icaps.format());
    auto oFormat = ocaps.format();

    if (oFormat == AkVideoCaps::Format_none)
        oFormat = icaps.format();

    auto ospecs = AkVideoCaps::formatSpecs(oFormat);

    DEFINE_CONVERT_TYPES(8, 8);
    DEFINE_CONVERT_TYPES(8, 16);
    DEFINE_CONVERT_TYPES(8, 32);
    DEFINE_CONVERT_TYPES(16, 8);
    DEFINE_CONVERT_TYPES(16, 16);
    DEFINE_CONVERT_TYPES(16, 32);
    DEFINE_CONVERT_TYPES(32, 8);
    DEFINE_CONVERT_TYPES(32, 16);
    DEFINE_CONVERT_TYPES(32, 32);

    auto icomponents = ispecs.mainComponents();
    auto ocomponents = ospecs.mainComponents();

    if (icomponents == 3 && ispecs.type() == ospecs.type())
        this->convertType = ConvertType_Vector;
    else if (icomponents == 3 && ocomponents == 3)
        this->convertType = ConvertType_3to3;
    else if (icomponents == 3 && ocomponents == 1)
        this->convertType = ConvertType_3to1;
    else if (icomponents == 1 && ocomponents == 3)
        this->convertType = ConvertType_1to3;
    else if (icomponents == 1 && ocomponents == 1)
        this->convertType = ConvertType_1to1;

    this->fromEndian = ispecs.endianness();
    this->toEndian = ospecs.endianness();
    colorConvert.setYuvColorSpace(yuvColorSpace);
    colorConvert.setYuvColorSpaceType(yuvColorSpaceType);
    colorConvert.loadMatrix(ispecs, ospecs);

    switch (ispecs.type()) {
    case AkVideoFormatSpec::VFT_RGB:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_R);
        this->planeYi = ispecs.componentPlane(AkColorComponent::CT_G);
        this->planeZi = ispecs.componentPlane(AkColorComponent::CT_B);

        this->compXi = ispecs.component(AkColorComponent::CT_R);
        this->compYi = ispecs.component(AkColorComponent::CT_G);
        this->compZi = ispecs.component(AkColorComponent::CT_B);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_Y);
        this->planeYi = ispecs.componentPlane(AkColorComponent::CT_U);
        this->planeZi = ispecs.componentPlane(AkColorComponent::CT_V);

        this->compXi = ispecs.component(AkColorComponent::CT_Y);
        this->compYi = ispecs.component(AkColorComponent::CT_U);
        this->compZi = ispecs.component(AkColorComponent::CT_V);

        break;

    case AkVideoFormatSpec::VFT_Gray:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_Y);
        this->compXi = ispecs.component(AkColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAi = ispecs.componentPlane(AkColorComponent::CT_A);
    this->compAi = ispecs.component(AkColorComponent::CT_A);

    switch (ospecs.type()) {
    case AkVideoFormatSpec::VFT_RGB:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_R);
        this->planeYo = ospecs.componentPlane(AkColorComponent::CT_G);
        this->planeZo = ospecs.componentPlane(AkColorComponent::CT_B);

        this->compXo = ospecs.component(AkColorComponent::CT_R);
        this->compYo = ospecs.component(AkColorComponent::CT_G);
        this->compZo = ospecs.component(AkColorComponent::CT_B);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_Y);
        this->planeYo = ospecs.componentPlane(AkColorComponent::CT_U);
        this->planeZo = ospecs.componentPlane(AkColorComponent::CT_V);

        this->compXo = ospecs.component(AkColorComponent::CT_Y);
        this->compYo = ospecs.component(AkColorComponent::CT_U);
        this->compZo = ospecs.component(AkColorComponent::CT_V);

        break;

    case AkVideoFormatSpec::VFT_Gray:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_Y);
        this->compXo = ospecs.component(AkColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAo = ospecs.componentPlane(AkColorComponent::CT_A);
    this->compAo = ospecs.component(AkColorComponent::CT_A);

    this->xiOffset = this->compXi.offset();
    this->yiOffset = this->compYi.offset();
    this->ziOffset = this->compZi.offset();
    this->aiOffset = this->compAi.offset();

    this->xoOffset = this->compXo.offset();
    this->yoOffset = this->compYo.offset();
    this->zoOffset = this->compZo.offset();
    this->aoOffset = this->compAo.offset();

    this->xiShift = this->compXi.shift();
    this->yiShift = this->compYi.shift();
    this->ziShift = this->compZi.shift();
    this->aiShift = this->compAi.shift();

    this->xoShift = this->compXo.shift();
    this->yoShift = this->compYo.shift();
    this->zoShift = this->compZo.shift();
    this->aoShift = this->compAo.shift();

    this->maxXi = this->compXi.max<quint64>();
    this->maxYi = this->compYi.max<quint64>();
    this->maxZi = this->compZi.max<quint64>();
    this->maxAi = this->compAi.max<quint64>();

    this->maskXo = ~(this->compXo.max<quint64>() << this->compXo.shift());
    this->maskYo = ~(this->compYo.max<quint64>() << this->compYo.shift());
    this->maskZo = ~(this->compZo.max<quint64>() << this->compZo.shift());
    this->alphaMask = this->compAo.max<quint64>() << this->compAo.shift();
    this->maskAo = ~this->alphaMask;

    bool hasAlphaIn = ispecs.contains(AkColorComponent::CT_A);
    bool hasAlphaOut = ospecs.contains(AkColorComponent::CT_A);

    if (hasAlphaIn && hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_AI_AO;
    else if (hasAlphaIn && !hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_AI_O;
    else if (!hasAlphaIn && hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_I_AO;
    else if (!hasAlphaIn && !hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_I_O;

    this->fastConvertion = ispecs.isFast() && ospecs.isFast();

    AkSimd simd("Core");

    this->convertSIMDFast8bits3to3    = reinterpret_cast<ConvertFast8bits3to3Type>   (simd.resolve("convertFast8bits3to3"));
    this->convertSIMDFast8bits3to3A   = reinterpret_cast<ConvertFast8bits3to3AType>  (simd.resolve("convertFast8bits3to3A"));
    this->convertSIMDFast8bits3Ato3   = reinterpret_cast<ConvertFast8bits3Ato3Type>  (simd.resolve("convertFast8bits3Ato3"));
    this->convertSIMDFast8bits3Ato3A  = reinterpret_cast<ConvertFast8bits3Ato3AType> (simd.resolve("convertFast8bits3Ato3A"));
    this->convertSIMDFast8bitsV3to3   = reinterpret_cast<ConvertFast8bitsV3to3Type>  (simd.resolve("convertFast8bitsV3to3"));
    this->convertSIMDFast8bitsV3to3A  = reinterpret_cast<ConvertFast8bitsV3to3AType> (simd.resolve("convertFast8bitsV3to3A"));
    this->convertSIMDFast8bitsV3Ato3  = reinterpret_cast<ConvertFast8bitsV3Ato3Type> (simd.resolve("convertFast8bitsV3Ato3"));
    this->convertSIMDFast8bitsV3Ato3A = reinterpret_cast<ConvertFast8bitsV3Ato3AType>(simd.resolve("convertFast8bitsV3Ato3"));
    this->convertSIMDFast8bits3to1    = reinterpret_cast<ConvertFast8bits3to1Type>   (simd.resolve("convertFast8bits3to1"));
    this->convertSIMDFast8bits3to1A   = reinterpret_cast<ConvertFast8bits3to1AType>  (simd.resolve("convertFast8bits3to1A"));
    this->convertSIMDFast8bits3Ato1   = reinterpret_cast<ConvertFast8bits3Ato1Type>  (simd.resolve("convertFast8bits3Ato1"));
    this->convertSIMDFast8bits3Ato1A  = reinterpret_cast<ConvertFast8bits3Ato1AType> (simd.resolve("convertFast8bits3AtoA"));
    this->convertSIMDFast8bits1to3    = reinterpret_cast<ConvertFast8bits1to3Type>   (simd.resolve("convertFast8bits1to3"));
    this->convertSIMDFast8bits1to3A   = reinterpret_cast<ConvertFast8bits1to3AType>  (simd.resolve("convertFast8bits1to3A"));
    this->convertSIMDFast8bits1Ato3   = reinterpret_cast<ConvertFast8bits1Ato3Type>  (simd.resolve("convertFast8bits1Ato3"));
    this->convertSIMDFast8bits1Ato3A  = reinterpret_cast<ConvertFast8bits1Ato3AType> (simd.resolve("convertFast8bits1Ato3A"));
    this->convertSIMDFast8bits1to1    = reinterpret_cast<ConvertFast8bits1to1Type>   (simd.resolve("convertFast8bits1to1"));
    this->convertSIMDFast8bits1to1A   = reinterpret_cast<ConvertFast8bits1to1AType>  (simd.resolve("convertFast8bits1to1A"));
    this->convertSIMDFast8bits1Ato1   = reinterpret_cast<ConvertFast8bits1Ato1Type>  (simd.resolve("convertFast8bits1Ato1"));
    this->convertSIMDFast8bits1Ato1A  = reinterpret_cast<ConvertFast8bits1Ato1AType> (simd.resolve("convertFast8bits1Ato1A"));
}

void FrameConvertParameters::configureScaling(const AkVideoCaps &icaps,
                                              const AkVideoCaps &ocaps,
                                              const QRect &inputRect,
                                              AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    QRect irect(0, 0, icaps.width(), icaps.height());

    if (!inputRect.isEmpty())
        irect = irect.intersected(inputRect);

    this->outputConvertCaps = ocaps;

    if (this->outputConvertCaps.format() == AkVideoCaps::Format_none)
        this->outputConvertCaps.setFormat(icaps.format());

    int width = this->outputConvertCaps.width() > 1?
                    this->outputConvertCaps.width():
                    irect.width();
    int height = this->outputConvertCaps.height() > 1?
                     this->outputConvertCaps.height():
                     irect.height();
    int owidth = width;
    int oheight = height;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep
        || aspectRatioMode == AkVideoConverter::AspectRatioMode_Fit) {
        auto w = height * irect.width() / irect.height();
        auto h = width * irect.height() / irect.width();

        if (w > width)
            w = width;
        else if (h > height)
            h = height;

        owidth = w;
        oheight = h;

        if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep) {
            width = owidth;
            height = oheight;
        }
    }

    this->outputConvertCaps.setWidth(width);
    this->outputConvertCaps.setHeight(height);
    this->outputConvertCaps.setFps(icaps.fps());

    this->xmin = (width - owidth) / 2;
    this->ymin = (height - oheight) / 2;
    this->xmax = (width + owidth) / 2;
    this->ymax = (height + oheight) / 2;

    if (owidth > irect.width()
        || oheight > irect.height())
        this->resizeMode = ResizeMode_Up;
    else if (owidth < irect.width()
             || oheight < irect.height())
        this->resizeMode = ResizeMode_Down;
    else
        this->resizeMode = ResizeMode_Keep;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Expanding) {
        auto w = irect.height() * owidth / oheight;
        auto h = irect.width() * oheight / owidth;

        if (w > irect.width())
            w = irect.width();

        if (h > irect.height())
            h = irect.height();

        auto x = (irect.x() + irect.width() - w) / 2;
        auto y = (irect.y() + irect.height() - h) / 2;
        irect = {x, y, w, h};
    }

    this->allocateBuffers(this->outputConvertCaps);

    auto &xomin = this->xmin;

    int wi_1 = qMax(1, irect.width() - 1);
    int wo_1 = qMax(1, owidth - 1);

    auto xSrcToDst = [&irect, &xomin, &wi_1, &wo_1] (int x) -> int {
        return ((x - irect.x()) * wo_1 + xomin * wi_1) / wi_1;
    };

    auto xDstToSrc = [&irect, &xomin, &wi_1, &wo_1] (int x) -> int {
        return ((x - xomin) * wi_1 + irect.x() * wo_1) / wo_1;
    };

    for (int x = 0; x < this->outputConvertCaps.width(); ++x) {
        auto xs = xDstToSrc(x);
        auto xs_1 = xDstToSrc(qMin(x + 1, this->outputConvertCaps.width() - 1));
        auto xmin = xSrcToDst(xs);
        auto xmax = xSrcToDst(xs + 1);

        this->srcWidth[x]   = xs;
        this->srcWidth_1[x] = qMin(xDstToSrc(x + 1), icaps.width());
        this->srcWidthOffsetX[x] = (xs >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY[x] = (xs >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ[x] = (xs >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA[x] = (xs >> this->compAi.widthDiv()) * this->compAi.step();

        this->srcWidthOffsetX_1[x] = (xs_1 >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY_1[x] = (xs_1 >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ_1[x] = (xs_1 >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA_1[x] = (xs_1 >> this->compAi.widthDiv()) * this->compAi.step();

        this->dstWidthOffsetX[x] = (x >> this->compXo.widthDiv()) * this->compXo.step();
        this->dstWidthOffsetY[x] = (x >> this->compYo.widthDiv()) * this->compYo.step();
        this->dstWidthOffsetZ[x] = (x >> this->compZo.widthDiv()) * this->compZo.step();
        this->dstWidthOffsetA[x] = (x >> this->compAo.widthDiv()) * this->compAo.step();

        if (xmax > xmin)
            this->kx[x] = SCALE_EMULT * (x - xmin) / (xmax - xmin);
        else
            this->kx[x] = 0;
    }

    auto &yomin = this->ymin;

    int hi_1 = qMax(1, irect.height() - 1);
    int ho_1 = qMax(1, oheight - 1);

    auto ySrcToDst = [&irect, &yomin, &hi_1, &ho_1] (int y) -> int {
        return ((y - irect.y()) * ho_1 + yomin * hi_1) / hi_1;
    };

    auto yDstToSrc = [&irect, &yomin, &hi_1, &ho_1] (int y) -> int {
        return ((y - yomin) * hi_1 + irect.y() * ho_1) / ho_1;
    };

    for (int y = 0; y < this->outputConvertCaps.height(); ++y) {
        if (this->resizeMode == ResizeMode_Down) {
            this->srcHeight[y] = yDstToSrc(y);
            this->srcHeight_1[y] = qMin(yDstToSrc(y + 1), icaps.height());
        } else {
            auto ys = yDstToSrc(y);
            auto ys_1 = yDstToSrc(qMin(y + 1, this->outputConvertCaps.height() - 1));
            auto ymin = ySrcToDst(ys);
            auto ymax = ySrcToDst(ys + 1);

            this->srcHeight[y] = ys;
            this->srcHeight_1[y] = ys_1;

            if (ymax > ymin)
                this->ky[y] = SCALE_EMULT * (y - ymin) / (ymax - ymin);
            else
                this->ky[y] = 0;
        }
    }

    this->inputWidth = icaps.width();
    this->inputWidth_1 = icaps.width() + 1;
    this->inputHeight = icaps.height();

    this->clearDlBuffers();

    if (this->resizeMode == ResizeMode_Down) {
        this->allocateDlBuffers(icaps, this->outputConvertCaps);

        for (int x = 0; x < icaps.width(); ++x) {
            this->dlSrcWidthOffsetX[x] = (x >> this->compXi.widthDiv()) * this->compXi.step();
            this->dlSrcWidthOffsetY[x] = (x >> this->compYi.widthDiv()) * this->compYi.step();
            this->dlSrcWidthOffsetZ[x] = (x >> this->compZi.widthDiv()) * this->compZi.step();
            this->dlSrcWidthOffsetA[x] = (x >> this->compAi.widthDiv()) * this->compAi.step();
        }

        for (int y = 0; y < this->outputConvertCaps.height(); ++y) {
            auto &ys = this->srcHeight[y];
            auto &ys_1 = this->srcHeight_1[y];

            this->srcHeightDlOffset[y] = size_t(ys) * this->inputWidth_1;
            this->srcHeightDlOffset_1[y] = size_t(ys_1) * this->inputWidth_1;

            auto diffY = ys_1 - ys;
            auto line = this->kdl + size_t(y) * icaps.width();

            for (int x = 0; x < this->outputConvertCaps.width(); ++x) {
                auto diffX = this->srcWidth_1[x] - this->srcWidth[x];
                line[x] = diffX * diffY;
            }
        }
    }

    this->outputFrame = {this->outputConvertCaps};

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Fit)
        this->outputFrame.fillRgb(qRgba(0, 0, 0, 0));
}

void FrameConvertParameters::reset()
{
    this->inputCaps = AkVideoCaps();
    this->outputCaps = AkVideoCaps();
    this->outputConvertCaps = AkVideoCaps();
    this->outputFrame = AkVideoPacket();
    this->scalingMode = AkVideoConverter::ScalingMode_Fast;
    this->aspectRatioMode = AkVideoConverter::AspectRatioMode_Ignore;
    this->convertType = ConvertType_Vector;
    this->convertDataTypes = ConvertDataTypes_8_8;
    this->alphaMode = ConvertAlphaMode_AI_AO;
    this->resizeMode = ResizeMode_Keep;
    this->fastConvertion = false;

    this->fromEndian = Q_BYTE_ORDER;
    this->toEndian = Q_BYTE_ORDER;

    this->clearBuffers();
    this->clearDlBuffers();

    this->xmin = 0;
    this->ymin = 0;
    this->xmax = 0;
    this->ymax = 0;

    this->inputWidth = 0;
    this->inputWidth_1 = 0;
    this->inputHeight = 0;

    this->planeXi = 0;
    this->planeYi = 0;
    this->planeZi = 0;
    this->planeAi = 0;

    this->compXi = {};
    this->compYi = {};
    this->compZi = {};
    this->compAi = {};

    this->planeXo = 0;
    this->planeYo = 0;
    this->planeZo = 0;
    this->planeAo = 0;

    this->compXo = {};
    this->compYo = {};
    this->compZo = {};
    this->compAo = {};

    this->xiOffset = 0;
    this->yiOffset = 0;
    this->ziOffset = 0;
    this->aiOffset = 0;

    this->xoOffset = 0;
    this->yoOffset = 0;
    this->zoOffset = 0;
    this->aoOffset = 0;

    this->xiShift = 0;
    this->yiShift = 0;
    this->ziShift = 0;
    this->aiShift = 0;

    this->xoShift = 0;
    this->yoShift = 0;
    this->zoShift = 0;
    this->aoShift = 0;

    this->maxXi = 0;
    this->maxYi = 0;
    this->maxZi = 0;
    this->maxAi = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;

    this->alphaMask = 0;
}

#include "moc_akvideoconverter.cpp"
