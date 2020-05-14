/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QDataStream>
#include <QDebug>
#include <QMetaEnum>
#include <QSize>
#include <QVector>
#include <QQmlEngine>

#include "akvideocaps.h"
#include "akfrac.h"
#include "akcaps.h"

class VideoFormat
{
    public:
        AkVideoCaps::PixelFormat format;
        int bpp;
        quint32 fourCC;
        QVector<int> planes;
        QVector<int> planes_div;

        static inline const QVector<VideoFormat> &formats()
        {
            static const QVector<VideoFormat> videoFormats = {
                {AkVideoCaps::Format_none          ,   0, AK_FOURCC_NULL           , {}              , {}          },
                {AkVideoCaps::Format_rgb444be      ,  12, AkFourCCS("\xcBGR")      , {16}            , {1}         },
                {AkVideoCaps::Format_rgb444le      ,  12, AkFourCCS("RGB\xc")      , {16}            , {1}         },
                {AkVideoCaps::Format_argb444be     ,  16, AK_FOURCC_NULL           , {16}            , {1}         },
                {AkVideoCaps::Format_argb444le     ,  16, AK_FOURCC_NULL           , {16}            , {1}         },
                {AkVideoCaps::Format_rgb555be      ,  15, AkFourCCS("\xfBGR")      , {16}            , {1}         },
                {AkVideoCaps::Format_rgb555le      ,  15, AkFourCCS("RGB\xf")      , {16}            , {1}         },
                {AkVideoCaps::Format_argb555be     ,  16, AK_FOURCC_NULL           , {16}            , {1}         },
                {AkVideoCaps::Format_argb555le     ,  16, AK_FOURCC_NULL           , {16}            , {1}         },
                {AkVideoCaps::Format_rgb565be      ,  16, AkFourCCS("\x10""BGR")   , {16}            , {1}         },
                {AkVideoCaps::Format_rgb565le      ,  16, AkFourCCS("RGB\x10")     , {16}            , {1}         },
                {AkVideoCaps::Format_rgb0          ,  24, AkFourCCS("RGB\x0")      , {32}            , {1}         },
                {AkVideoCaps::Format_rgb24         ,  24, AkFourCCS("RGB\x18")     , {24}            , {1}         },
                {AkVideoCaps::Format_rgb48be       ,  48, AkFourCCS("0RGB")        , {48}            , {1}         },
                {AkVideoCaps::Format_rgb48le       ,  48, AkFourCCS("RGB0")        , {48}            , {1}         },
                {AkVideoCaps::Format_bgr444be      ,  12, AkFourCCS("\xcRGB")      , {16}            , {1}         },
                {AkVideoCaps::Format_bgr444le      ,  12, AkFourCCS("BGR\xc")      , {16}            , {1}         },
                {AkVideoCaps::Format_bgr555be      ,  15, AkFourCCS("\xfRGB")      , {16}            , {1}         },
                {AkVideoCaps::Format_bgr555le      ,  15, AkFourCCS("BGR\xf")      , {16}            , {1}         },
                {AkVideoCaps::Format_bgr565be      ,  16, AkFourCCS("\x10RGB")     , {16}            , {1}         },
                {AkVideoCaps::Format_bgr565le      ,  16, AkFourCCS("BGR\x10")     , {16}            , {1}         },
                {AkVideoCaps::Format_rgb666        ,  18, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_argb1665      ,  18, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_argb1666      ,  19, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_bgr666        ,  18, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_argb6666      ,  24, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_abgr6666      ,  24, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_0bgr          ,  32, AkFourCCS("\x0BGR")      , {32}            , {1}         },
                {AkVideoCaps::Format_0rgb          ,  32, AkFourCCS("\x0RGB")      , {32}            , {1}         },
                {AkVideoCaps::Format_abgr          ,  32, AkFourCCS("ABGR")        , {32}            , {1}         },
                {AkVideoCaps::Format_argb          ,  32, AkFourCCS("ARGB")        , {32}            , {1}         },
                {AkVideoCaps::Format_bgr0          ,  32, AkFourCCS("BGR\x0")      , {32}            , {1}         },
                {AkVideoCaps::Format_bgr24         ,  32, AkFourCCS("BGR\x18")     , {24}            , {1}         },
                {AkVideoCaps::Format_bgr48be       ,  48, AkFourCCS("0BGR")        , {48}            , {1}         },
                {AkVideoCaps::Format_bgr48le       ,  48, AkFourCCS("BGR0")        , {48}            , {1}         },
                {AkVideoCaps::Format_rgba          ,  32, AkFourCCS("RGBA")        , {32}            , {1}         },
                {AkVideoCaps::Format_rgba64be      ,  64, AkFourCCS("\x40RBA")     , {64}            , {1}         },
                {AkVideoCaps::Format_rgba64le      ,  64, AkFourCCS("RBA\x40")     , {64}            , {1}         },
                {AkVideoCaps::Format_bgra          ,  32, AkFourCCS("BGRA")        , {32}            , {1}         },
                {AkVideoCaps::Format_bgra64be      ,  64, AkFourCCS("\x40""BRA")   , {64}            , {1}         },
                {AkVideoCaps::Format_bgra64le      ,  64, AkFourCCS("BRA\x40")     , {64}            , {1}         },
                {AkVideoCaps::Format_argb1887      ,  24, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_bgra1888      ,  25, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_monob         ,   1, AkFourCCS("B0W1")        , { 1}            , {1}         },
                {AkVideoCaps::Format_monow         ,   1, AkFourCCS("B1W0")        , { 1}            , {1}         },
                {AkVideoCaps::Format_rgb4          ,   4, AkFourCCS("RGB\x4")      , { 4}            , {1}         },
                {AkVideoCaps::Format_rgb4_byte     ,   4, AkFourCCS("B4BY")        , { 8}            , {1}         },
                {AkVideoCaps::Format_rgb8          ,   8, AkFourCCS("RGB\x8")      , { 8}            , {1}         },
                {AkVideoCaps::Format_bgr4          ,   4, AkFourCCS("BGR\x4")      , { 4}            , {1}         },
                {AkVideoCaps::Format_bgr4_byte     ,   4, AkFourCCS("R4BY")        , { 8}            , {1}         },
                {AkVideoCaps::Format_bgr8          ,   8, AkFourCCS("BGR\x8")      , { 8}            , {1}         },
                {AkVideoCaps::Format_gray2         ,   2, AK_FOURCC_NULL           , { 8}            , {1}         },
                {AkVideoCaps::Format_gray4         ,   4, AK_FOURCC_NULL           , { 8}            , {1}         },
                {AkVideoCaps::Format_gray24        ,  24, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_gray32        ,  32, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_gray          ,   8, AkFourCCS("Y800")        , { 8}            , {1}         },
                {AkVideoCaps::Format_gray9be       ,   9, AkFourCCS("\x9\x01Y")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray9le       ,   9, AkFourCCS("Y1\x0\x9")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray10be      ,  10, AkFourCCS("\xa\x01Y")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray10le      ,  10, AkFourCCS("Y1\x0\xa")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray12be      ,  12, AkFourCCS("\xc\x01Y")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray12le      ,  12, AkFourCCS("Y1\x0\xc")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray14be      ,  14, AkFourCCS("\xe\x01Y")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray14le      ,  14, AkFourCCS("Y1\x0\xe")    , {16}            , {1}         },
                {AkVideoCaps::Format_gray16be      ,  16, AkFourCCS("\x10\x01Y")   , {16}            , {1}         },
                {AkVideoCaps::Format_gray16le      ,  16, AkFourCCS("Y1\x0\x10")   , {16}            , {1}         },
                {AkVideoCaps::Format_grayf32be     ,  32, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_grayf32le     ,  32, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_bayer_bggr8   ,   8, AkFourCCS("\xba""BG\x8") , { 8}            , {1}         },
                {AkVideoCaps::Format_bayer_gbrg8   ,   8, AkFourCCS("\xbaGB\x8")   , { 8}            , {1}         },
                {AkVideoCaps::Format_bayer_grbg8   ,   8, AkFourCCS("\xbaGR\x8")   , { 8}            , {1}         },
                {AkVideoCaps::Format_bayer_rggb8   ,   8, AkFourCCS("\xbaRG\x8")   , { 8}            , {1}         },
                {AkVideoCaps::Format_bayer_bggr16be,  16, AkFourCCS("\x10GB\xba")  , {16}            , {1}         },
                {AkVideoCaps::Format_bayer_bggr16le,  16, AkFourCCS("\xba""BG\x10"), {16}            , {1}         },
                {AkVideoCaps::Format_bayer_gbrg16be,  16, AkFourCCS("\x10""BG\xba"), {16}            , {1}         },
                {AkVideoCaps::Format_bayer_gbrg16le,  16, AkFourCCS("\xbaGB\x10")  , {16}            , {1}         },
                {AkVideoCaps::Format_bayer_grbg16be,  16, AkFourCCS("\x10RG\xba")  , {16}            , {1}         },
                {AkVideoCaps::Format_bayer_grbg16le,  16, AkFourCCS("\xbaGR\x10")  , {16}            , {1}         },
                {AkVideoCaps::Format_bayer_rggb16be,  16, AkFourCCS("\x10GR\xba")  , {16}            , {1}         },
                {AkVideoCaps::Format_bayer_rggb16le,  16, AkFourCCS("\xbaRG\x10")  , {16}            , {1}         },
                {AkVideoCaps::Format_ayuv64be      ,  64, AK_FOURCC_NULL           , {64}            , {1}         },
                {AkVideoCaps::Format_ayuv64le      ,  64, AK_FOURCC_NULL           , {64}            , {1}         },
                {AkVideoCaps::Format_uyvy422       ,  16, AkFourCCS("UYVY")        , {16}            , {1}         },
                {AkVideoCaps::Format_vyuy422       ,  16, AK_FOURCC_NULL           , {16}            , {1}         },
                {AkVideoCaps::Format_uyyvyy411     ,  12, AkFourCCS("Y411")        , {12}            , {1}         },
                {AkVideoCaps::Format_ya16be        ,  32, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_ya16le        ,  32, AK_FOURCC_NULL           , {32}            , {1}         },
                {AkVideoCaps::Format_ya8           ,  16, AkFourCCS("Y2\x0\x8")    , {16}            , {1}         },
                {AkVideoCaps::Format_yuyv422       ,  16, AkFourCCS("YUY2")        , {16}            , {1}         },
                {AkVideoCaps::Format_yvyu422       ,  16, AkFourCCS("YVYU")        , {16}            , {1}         },
                {AkVideoCaps::Format_xyz12be       ,  36, AkFourCCS("\x24ZYX")     , {48}            , {1}         },
                {AkVideoCaps::Format_xyz12le       ,  36, AkFourCCS("XYZ\x24")     , {48}            , {1}         },
                {AkVideoCaps::Format_nv12          ,  12, AkFourCCS("NV12")        , { 8,  8}        , {1, 2}      },
                {AkVideoCaps::Format_nv16          ,  16, AK_FOURCC_NULL           , { 8,  8}        , {1, 1}      },
                {AkVideoCaps::Format_nv20be        ,  20, AK_FOURCC_NULL           , {16, 16}        , {1, 1}      },
                {AkVideoCaps::Format_nv20le        ,  20, AK_FOURCC_NULL           , {16, 16}        , {1, 1}      },
                {AkVideoCaps::Format_nv21          ,  12, AkFourCCS("NV21")        , { 8,  8}        , {1, 2}      },
                {AkVideoCaps::Format_p010be        ,  15, AK_FOURCC_NULL           , {16, 16}        , {1, 2}      },
                {AkVideoCaps::Format_p010le        ,  15, AK_FOURCC_NULL           , {16, 16}        , {1, 2}      },
                {AkVideoCaps::Format_p016be        ,  24, AK_FOURCC_NULL           , {16, 16}        , {1, 2}      },
                {AkVideoCaps::Format_p016le        ,  24, AK_FOURCC_NULL           , {16, 16}        , {1, 2}      },
                {AkVideoCaps::Format_yuv410p       ,   9, AkFourCCS("YUV9")        , { 8,  2,  2}    , {1, 4, 4}   },
                {AkVideoCaps::Format_yuv411p       ,  12, AkFourCCS("Y41B")        , { 8,  2,  2}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv420p       ,  12, AkFourCCS("I420")        , { 8,  4,  4}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yvu420p       ,  12, AK_FOURCC_NULL           , { 8,  4,  4}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv422p       ,  16, AkFourCCS("Y42B")        , { 8,  4,  4}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv440p       ,  16, AkFourCCS("440P")        , { 8,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv444p       ,  24, AkFourCCS("444P")        , { 8,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuvj411p      ,  12, AK_FOURCC_NULL           , { 8,  2,  2}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuvj420p      ,  12, AkFourCCS("I420")        , { 8,  4,  4}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuvj422p      ,  16, AkFourCCS("Y42B")        , { 8,  4,  4}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuvj440p      ,  16, AkFourCCS("440P")        , { 8,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuvj444p      ,  24, AkFourCCS("444P")        , { 8,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv420p9be    ,  13, AkFourCCS("\x9\xb3Y")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p9le    ,  13, AkFourCCS("Y3\xb\x9")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p10be   ,  15, AkFourCCS("\xa\xb3Y")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p10le   ,  15, AkFourCCS("Y3\xb\xa")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p12be   ,  18, AkFourCCS("\xc\xb3Y")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p12le   ,  18, AkFourCCS("Y3\xb\xc")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p14be   ,  21, AkFourCCS("\xe\xb3Y")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p14le   ,  21, AkFourCCS("Y3\xb\xe")    , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p16be   ,  24, AkFourCCS("\x10\xb3Y")   , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv420p16le   ,  24, AkFourCCS("Y3\xb\x10")   , {16,  8,  8}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv422p9be    ,  18, AkFourCCS("\x9\xa3Y")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p9le    ,  18, AkFourCCS("Y3\xa\x9")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p10be   ,  20, AkFourCCS("\xa\xa3Y")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p10le   ,  20, AkFourCCS("Y3\xa\xa")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p12be   ,  24, AkFourCCS("\xc\xa3Y")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p12le   ,  24, AkFourCCS("Y3\xa\xc")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p14be   ,  28, AkFourCCS("\xe\xa3Y")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p14le   ,  28, AkFourCCS("Y3\xa\xe")    , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p16be   ,  32, AkFourCCS("\x10\xa3Y")   , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv422p16le   ,  32, AkFourCCS("Y3\xa\x10")   , {16,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv440p10be   ,  20, AK_FOURCC_NULL           , {16, 16, 16}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv440p10le   ,  20, AK_FOURCC_NULL           , {16, 16, 16}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv440p12be   ,  24, AK_FOURCC_NULL           , {16, 16, 16}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv440p12le   ,  24, AK_FOURCC_NULL           , {16, 16, 16}    , {1, 2, 2}   },
                {AkVideoCaps::Format_yuv444p9be    ,  27, AkFourCCS("\x9\x03Y")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p9le    ,  27, AkFourCCS("Y3\x0\x9")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p10be   ,  30, AkFourCCS("\xa\x03Y")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p10le   ,  30, AkFourCCS("Y3\x0\xa")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p12be   ,  36, AkFourCCS("\xc\x03Y")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p12le   ,  36, AkFourCCS("Y3\x0\xc")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p14be   ,  42, AkFourCCS("\xe\x03Y")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p14le   ,  42, AkFourCCS("Y3\x0\xe")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p16be   ,  48, AkFourCCS("\x10\x03Y")   , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuv444p16le   ,  48, AkFourCCS("Y3\x0\x10")   , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_yuva420p      ,  20, AkFourCCS("Y4\xb\x8")    , { 8,  4,  4,  8}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva422p      ,  24, AkFourCCS("Y4\xa\x8")    , { 8,  4,  4,  8}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuv444        ,  24, AK_FOURCC_NULL           , {24}            , {1}         },
                {AkVideoCaps::Format_yuva444p      ,  32, AkFourCCS("Y4\x0\x8")    , { 8,  8,  8,  8}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva420p9be   ,  22, AkFourCCS("\x9\xb4Y")    , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva420p9le   ,  22, AkFourCCS("Y4\xb\x9")    , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva420p10be  ,  25, AkFourCCS("\xa\xb4Y")    , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva420p10le  ,  25, AkFourCCS("Y4\xb\xa")    , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva420p16be  ,  40, AkFourCCS("\x10\xb4Y")   , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva420p16le  ,  40, AkFourCCS("Y4\xb\x10")   , {16,  8,  8, 16}, {1, 2, 2, 1}},
                {AkVideoCaps::Format_yuva422p9be   ,  27, AkFourCCS("\x9\xa4Y")    , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva422p9le   ,  27, AkFourCCS("Y4\xa\x9")    , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva422p10be  ,  30, AkFourCCS("\xa\xa4Y")    , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva422p10le  ,  30, AkFourCCS("Y4\xa\xa")    , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva422p16be  ,  48, AkFourCCS("\x10\xa4Y")   , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva422p16le  ,  48, AkFourCCS("Y4\xa\x10")   , {16,  8,  8, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p9be   ,  36, AkFourCCS("\x9\x04Y")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p9le   ,  36, AkFourCCS("Y4\x0\x9")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p10be  ,  40, AkFourCCS("\xa\x04Y")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p10le  ,  40, AkFourCCS("Y4\x0\xa")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p16be  ,  64, AkFourCCS("\x10\x04Y")   , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_yuva444p16le  ,  64, AkFourCCS("Y4\x0\x10")   , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrp          ,  24, AkFourCCS("G3\x0\x8")    , { 8,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp9be       ,  27, AkFourCCS("\x9\x03G")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp9le       ,  27, AkFourCCS("G3\x0\x9")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp10be      ,  30, AkFourCCS("\xa\x03G")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp10le      ,  30, AkFourCCS("G3\x0\xa")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp12be      ,  36, AkFourCCS("\xc\x03G")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp12le      ,  36, AkFourCCS("G3\x0\xc")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp14be      ,  42, AkFourCCS("\xe\x03G")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp14le      ,  42, AkFourCCS("G3\x0\xe")    , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp16be      ,  48, AkFourCCS("\x10\x03G")   , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrp16le      ,  48, AkFourCCS("G3\x0\x10")   , {16, 16, 16}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrpf32be     ,  96, AK_FOURCC_NULL           , {32, 32, 32}    , {1, 1, 1}   },
                {AkVideoCaps::Format_gbrpf32le     ,  96, AK_FOURCC_NULL           , {32, 32, 32}    , {1, 1, 1}   },
                {AkVideoCaps::Format_rgbp          ,  24, AK_FOURCC_NULL           , { 8,  8,  8}    , {1, 1, 1}   },
                {AkVideoCaps::Format_rgbap         ,  32, AK_FOURCC_NULL           , { 8,  8,  8,  8}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap         ,  32, AkFourCCS("G4\x0\x8")    , { 8,  8,  8,  8}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap10be     ,  40, AkFourCCS("\xa\x04G")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap10le     ,  40, AkFourCCS("G4\x0\xa")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap12be     ,  48, AkFourCCS("\xc\x04G")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap12le     ,  48, AkFourCCS("G4\x0\xc")    , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap16be     ,  64, AkFourCCS("\x10\x04G")   , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrap16le     ,  64, AkFourCCS("G4\x0\x10")   , {16, 16, 16, 16}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrapf32be    , 128, AK_FOURCC_NULL           , {32, 32, 32, 32}, {1, 1, 1, 1}},
                {AkVideoCaps::Format_gbrapf32le    , 128, AK_FOURCC_NULL           , {32, 32, 32, 32}, {1, 1, 1, 1}},
            };

            return videoFormats;
        }

        static inline const VideoFormat *byFormat(AkVideoCaps::PixelFormat format)
        {
            for (auto &format_: formats())
                if (format_.format == format)
                    return &format_;

            return &formats().front();
        }

        static inline const VideoFormat *byBpp(int bpp)
        {
            for (auto &format: formats())
                if (format.bpp == bpp)
                    return &format;

            return &formats().front();
        }

        static inline const VideoFormat *byFourCC(quint32 fourCC)
        {
            for (auto &format: formats())
                if (format.fourCC == fourCC)
                    return &format;

            return &formats().front();
        }

        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

class AkVideoCapsPrivate
{
    public:
        AkVideoCaps::PixelFormat m_format {AkVideoCaps::Format_none};
        int m_width {0};
        int m_height {0};
        int m_align {1};
        AkFrac m_fps;
        const QVector<int> *m_planes_div;
        QVector<size_t> m_bypl;
        QVector<size_t> m_offset;

        void updateParams();
};

AkVideoCaps::AkVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoCapsPrivate();
}

AkVideoCaps::AkVideoCaps(AkVideoCaps::PixelFormat format,
                         int width,
                         int height,
                         const AkFrac &fps,
                         int align)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
    this->d->m_fps = fps;
    this->d->m_align = align;
    this->d->updateParams();
}

AkVideoCaps::AkVideoCaps(AkVideoCaps::PixelFormat format,
                         const QSize &size,
                         const AkFrac &fps,
                         int align)
{
    AkVideoCaps(format, size.width(), size.height(), fps, align);
}


AkVideoCaps::AkVideoCaps(const AkCaps &caps)
{
    this->d = new AkVideoCapsPrivate();

    if (caps.mimeType() == "video/x-raw")
        this->update(caps);
}

AkVideoCaps::AkVideoCaps(const AkVideoCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
    this->d->m_align = other.d->m_align;
    this->d->m_planes_div = other.d->m_planes_div;
    this->d->m_bypl = other.d->m_bypl;
    this->d->m_offset = other.d->m_offset;
    auto properties = other.dynamicPropertyNames();

    for (auto &property: properties)
        this->setProperty(property, other.property(property));
}

AkVideoCaps::~AkVideoCaps()
{
    delete this->d;
}

AkVideoCaps &AkVideoCaps::operator =(const AkVideoCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
        this->d->m_align = other.d->m_align;
        this->d->m_planes_div = other.d->m_planes_div;
        this->d->m_bypl = other.d->m_bypl;
        this->d->m_offset = other.d->m_offset;

        this->clear();
        auto properties = other.dynamicPropertyNames();

        for (auto &property: properties)
            this->setProperty(property, other.property(property));

        this->d->updateParams();
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "video/x-raw") {
        this->update(caps);
    } else {
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = AkFrac();
        this->d->m_align = 1;
    }

    return *this;
}

bool AkVideoCaps::operator ==(const AkVideoCaps &other) const
{
    if (this->dynamicPropertyNames() != other.dynamicPropertyNames())
        return false;

    for (auto &property: this->dynamicPropertyNames())
        if (this->property(property) != other.property(property))
            return false;

    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height
            && this->d->m_fps == other.d->m_fps
            && this->d->m_align == other.d->m_align;
}

bool AkVideoCaps::operator !=(const AkVideoCaps &other) const
{
    return !(*this == other);
}

QObject *AkVideoCaps::create()
{
    return new AkVideoCaps();
}

QObject *AkVideoCaps::create(const AkCaps &caps)
{
    return new AkVideoCaps(caps);
}

QObject *AkVideoCaps::create(const AkVideoCaps &caps)
{
    return new AkVideoCaps(caps);
}

QObject *AkVideoCaps::create(AkVideoCaps::PixelFormat format,
                             int width,
                             int height,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(format, width, height, fps, align);
}

QObject *AkVideoCaps::create(const QString &format,
                             int width,
                             int height,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(AkVideoCaps::pixelFormatFromString(format),
                           width,
                           height,
                           fps,
                           align);
}

QObject *AkVideoCaps::create(AkVideoCaps::PixelFormat format,
                             const QSize &size,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(format, size, fps, align);
}

QObject *AkVideoCaps::create(const QString &format,
                             const QSize &size,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(AkVideoCaps::pixelFormatFromString(format),
                           size,
                           fps,
                           align);
}

QVariant AkVideoCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkVideoCaps::operator AkCaps() const
{
    AkCaps caps("video/x-raw");
    caps.setProperty("format", this->d->m_format);
    caps.setProperty("width" , this->d->m_width);
    caps.setProperty("height", this->d->m_height);
    caps.setProperty("fps"   , QVariant::fromValue(this->d->m_fps));
    caps.setProperty("align" , this->d->m_align);

    return caps;
}

AkVideoCaps::operator bool() const
{
    return this->pictureSize() > 0;
}

AkVideoCaps::PixelFormat AkVideoCaps::format() const
{
    return this->d->m_format;
}

quint32 AkVideoCaps::fourCC() const
{
    return AkVideoCaps::fourCC(this->d->m_format);
}

int AkVideoCaps::bpp() const
{
    return VideoFormat::byFormat(this->d->m_format)->bpp;
}

QSize AkVideoCaps::size() const
{
    return {this->d->m_width, this->d->m_height};
}

int AkVideoCaps::width() const
{
    return this->d->m_width;
}

int AkVideoCaps::height() const
{
    return this->d->m_height;
}

AkFrac AkVideoCaps::fps() const
{
    return this->d->m_fps;
}

AkFrac &AkVideoCaps::fps()
{
    return this->d->m_fps;
}

int AkVideoCaps::align() const
{
    return this->d->m_align;
}

size_t AkVideoCaps::pictureSize() const
{
    auto vf = VideoFormat::byFormat(this->d->m_format);

    if (!vf)
        return 0;

    size_t size = 0;

    for (int i = 0; i < vf->planes.size(); i++)
        size += this->planeSize(i);

    return size;
}

AkVideoCaps AkVideoCaps::fromMap(const QVariantMap &caps)
{
    AkVideoCaps videoCaps;

    if (!caps.contains("mimeType") || caps["mimeType"] != "video/x-raw")
        return videoCaps;

    for (auto it = caps.begin(); it != caps.end(); it++) {
        auto value = it.value();

        if (it.key() == "mimeType")
            continue;

        videoCaps.setProperty(it.key().toStdString().c_str(), value);
    }

    return videoCaps;
}

QVariantMap AkVideoCaps::toMap() const
{
    QVariantMap map {
        {"mimeType", "video/x-raw"                      },
        {"format"  , this->d->m_format                  },
        {"width"   , this->d->m_width                   },
        {"height"  , this->d->m_height                  },
        {"fps"     , QVariant::fromValue(this->d->m_fps)},
        {"align"   , this->d->m_align                   },
    };

    for (auto &property: this->dynamicPropertyNames()) {
        auto key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

AkVideoCaps &AkVideoCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "video/x-raw")
        return *this;

    this->clear();

    for (auto &property: caps.dynamicPropertyNames()) {
        int i = this->metaObject()->indexOfProperty(property);

        if (this->metaObject()->property(i).isWritable())
            this->setProperty(property, caps.property(property));
    }

    this->d->updateParams();

    return *this;
}

size_t AkVideoCaps::planeOffset(int plane) const
{
    return this->d->m_offset[plane];
}

size_t AkVideoCaps::lineOffset(int plane, int y) const
{
    y /= (*this->d->m_planes_div)[plane];

    return this->planeOffset(plane) + this->bytesPerLine(plane) *  size_t(y);
}

size_t AkVideoCaps::bytesPerLine(int plane) const
{
    return this->d->m_bypl.value(plane, 0);
}

int AkVideoCaps::planes() const
{
    auto vf = VideoFormat::byFormat(this->d->m_format);

    return vf? vf->planes.size(): 0;
}

size_t AkVideoCaps::planeSize(int plane) const
{
    auto bypl = this->bytesPerLine(plane);

    if (bypl < 1)
        return 0;

    auto vf = VideoFormat::byFormat(this->d->m_format);

    return bypl
            * size_t(this->d->m_height)
            / size_t(vf->planes_div[plane]);
}

int AkVideoCaps::bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::byFormat(pixelFormat)->bpp;
}

int AkVideoCaps::bitsPerPixel(const QString &pixelFormat)
{
    return AkVideoCaps::bitsPerPixel(AkVideoCaps::pixelFormatFromString(pixelFormat));
}

QString AkVideoCaps::pixelFormatToString(AkVideoCaps::PixelFormat pixelFormat)
{
    AkVideoCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(pixelFormat));
    format.remove("Format_");

    return format;
}

AkVideoCaps::PixelFormat AkVideoCaps::pixelFormatFromString(const QString &pixelFormat)
{
    AkVideoCaps caps;
    QString format = "Format_" + pixelFormat;
    int enumIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum enumType = caps.metaObject()->enumerator(enumIndex);
    int enumValue = enumType.keyToValue(format.toStdString().c_str());

    return static_cast<PixelFormat>(enumValue);
}

quint32 AkVideoCaps::fourCC(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::byFormat(pixelFormat)->fourCC;
}

quint32 AkVideoCaps::fourCC(const QString &pixelFormat)
{
    return AkVideoCaps::fourCC(AkVideoCaps::pixelFormatFromString(pixelFormat));
}

void AkVideoCaps::setFormat(AkVideoCaps::PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    this->d->updateParams();
    emit this->formatChanged(format);
}

void AkVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->d->m_width = size.width();
    this->d->m_height = size.height();
    this->d->updateParams();
    emit this->widthChanged(size.width());
    emit this->heightChanged(size.height());
    emit sizeChanged(size);
}

void AkVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    this->d->updateParams();
    emit this->widthChanged(width);
}

void AkVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    this->d->updateParams();
    emit this->heightChanged(height);
}

void AkVideoCaps::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AkVideoCaps::setAlign(int align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    this->d->updateParams();
    emit this->alignChanged(align);
}

void AkVideoCaps::resetFormat()
{
    this->setFormat(AkVideoCaps::Format_none);
}

void AkVideoCaps::resetSize()
{
    this->setSize(QSize());
}

void AkVideoCaps::resetWidth()
{
    this->setWidth(0);
}

void AkVideoCaps::resetHeight()
{
    this->setHeight(0);
}

void AkVideoCaps::resetFps()
{
    this->setFps(AkFrac());
}

void AkVideoCaps::resetAlign()
{
    this->setAlign(1);
}

void AkVideoCaps::clear()
{
    for (auto &property: this->dynamicPropertyNames())
        this->setProperty(property.constData(), {});
}

void AkVideoCaps::registerTypes()
{
    qRegisterMetaType<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<AkVideoCapsList>("AkVideoCapsList");
    qRegisterMetaTypeStreamOperators<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<PixelFormat>("PixelFormat");
    qRegisterMetaType<PixelFormatList>("PixelFormatList");
    QMetaType::registerDebugStreamOperator<AkVideoCaps::PixelFormat>();
    qmlRegisterSingletonType<AkVideoCaps>("Ak", 1, 0, "AkVideoCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoCaps();
    });
}

void AkVideoCapsPrivate::updateParams()
{
    auto vf = VideoFormat::byFormat(this->m_format);

    if (!vf) {
        this->m_planes_div = nullptr;
        this->m_offset.clear();
        this->m_bypl.clear();

        return;
    }

    this->m_planes_div = &vf->planes_div;
    this->m_offset.clear();
    this->m_bypl.clear();
    size_t offset = 0;

    for (int i = 0; i < vf->planes_div.size(); i++) {
        this->m_offset << offset;
        auto bypl = VideoFormat::alignUp(size_t(vf->planes[i]
                                                * this->m_width
                                                / 8),
                                         size_t(this->m_align));
        this->m_bypl << bypl;
        offset += bypl * size_t(this->m_height) / size_t(vf->planes_div[i]);
    }
}

QDebug operator <<(QDebug debug, const AkVideoCaps &caps)
{
    debug.nospace() << "AkVideoCaps("
                    << "format="
                    << caps.format()
                    << ",width="
                    << caps.width()
                    << ",height="
                    << caps.height()
                    << ",fps="
                    << caps.fps()
                    << ",align="
                    << caps.align();

    QStringList properties;

    for (auto &property: caps.dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (auto &property: properties)
        debug.nospace() << ","
                        << property.toStdString().c_str()
                        << "="
                        << caps.property(property.toStdString().c_str());

    debug.nospace() << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkVideoCaps::PixelFormat format)
{
    debug.nospace() << AkVideoCaps::pixelFormatToString(format).toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps)
{
    int nProperties;
    istream >> nProperties;

    for (int i = 0; i < nProperties; i++) {
        QByteArray key;
        QVariant value;
        istream >> key;
        istream >> value;

        caps.setProperty(key.toStdString().c_str(), value);
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps)
{
    QVariantMap staticProperties {
        {"format", caps.format()                  },
        {"width" , caps.width()                   },
        {"height", caps.height()                  },
        {"fps"   , QVariant::fromValue(caps.fps())},
        {"align" , caps.align()                   },
    };

    int nProperties =
            staticProperties.size() + caps.dynamicPropertyNames().size();
    ostream << nProperties;

    for (auto &key: caps.dynamicPropertyNames()) {
        ostream << key;
        ostream << caps.property(key);
    }

    return ostream;
}

#include "moc_akvideocaps.cpp"
