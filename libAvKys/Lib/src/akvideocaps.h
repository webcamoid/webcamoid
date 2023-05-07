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

#ifndef AKVIDEOCAPS_H
#define AKVIDEOCAPS_H

#include <QObject>

#include "akcommons.h"

#define AK_MAKE_FOURCC(a, b, c, d) \
    (((quint32(a) & 0xff) << 24) \
   | ((quint32(b) & 0xff) << 16) \
   | ((quint32(c) & 0xff) <<  8) \
   |  (quint32(d) & 0xff))

class AkVideoCaps;
class AkVideoCapsPrivate;
class AkCaps;
class AkFrac;
class AkVideoFormatSpec;

using AkVideoCapsList = QList<AkVideoCaps>;

class AKCOMMONS_EXPORT AkVideoCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(PixelFormat format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
    Q_PROPERTY(int bpp
               READ bpp
               CONSTANT)
    Q_PROPERTY(QSize size
               READ size
               WRITE setSize
               RESET resetSize
               NOTIFY sizeChanged)
    Q_PROPERTY(int width
               READ width
               WRITE setWidth
               RESET resetWidth
               NOTIFY widthChanged)
    Q_PROPERTY(int height
               READ height
               WRITE setHeight
               RESET resetHeight
               NOTIFY heightChanged)
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        enum PixelFormat
        {
            Format_none            = AK_MAKE_FOURCC(0, 0, 0, 0),
            Format_0bgr            = AK_MAKE_FOURCC(0, 'B', 'G', 'R'),
            Format_0bgr444be       = AK_MAKE_FOURCC(44, 'G', 'B', 0),
            Format_0bgr444le       = AK_MAKE_FOURCC(0, 'B', 'G', 44),
            Format_0bgrpackbe      = AK_MAKE_FOURCC('Q', 'G', 'B', 0),
            Format_0bgrpackle      = AK_MAKE_FOURCC(0, 'B', 'G', 'Q'),
            Format_0rgb            = AK_MAKE_FOURCC(0, 'R', 'G', 'B'),
            Format_0rgbpackbe      = AK_MAKE_FOURCC('Q', 'G', 'R', 0),
            Format_0rgbpackle      = AK_MAKE_FOURCC(0, 'R', 'G', 'Q'),
            Format_0yuv            = AK_MAKE_FOURCC(0, 'Y', 'U', 'V'),
            Format_abgr            = AK_MAKE_FOURCC('A', 'B', 'G', 'R'),
            Format_abgr1555be      = AK_MAKE_FOURCC(55, 51, 'B', 'A'),
            Format_abgr1555le      = AK_MAKE_FOURCC('A', 'B', 15, 55),
            Format_abgr4444be      = AK_MAKE_FOURCC(44, 44, 'B', 'A'),
            Format_abgr4444le      = AK_MAKE_FOURCC('A', 'B', 44, 44),
            Format_abgrpackbe      = AK_MAKE_FOURCC('Q', 'G', 'B', 'A'),
            Format_abgrpackle      = AK_MAKE_FOURCC('A', 'B', 'G', 'Q'),
            Format_argb            = AK_MAKE_FOURCC('A', 'R', 'G', 'B'),
            Format_argb1555be      = AK_MAKE_FOURCC(55, 51, 'R', 'A'),
            Format_argb1555le      = AK_MAKE_FOURCC('A', 'R', 15, 55),
            Format_argb2101010be   = AK_MAKE_FOURCC(10, 10, 12, 'A'),
            Format_argb2101010le   = AK_MAKE_FOURCC('A', 210, 10, 10),
            Format_argb4444be      = AK_MAKE_FOURCC(44, 44, 'R', 'A'),
            Format_argb4444le      = AK_MAKE_FOURCC('A', 'R', 44, 44),
            Format_argb64be        = AK_MAKE_FOURCC(64, 'G', 'R', 'A'),
            Format_argb64le        = AK_MAKE_FOURCC('A', 'R', 'G', 64),
            Format_argbpackbe      = AK_MAKE_FOURCC('Q', 'G', 'R', 'A'),
            Format_argbpackle      = AK_MAKE_FOURCC('A', 'R', 'G', 'Q'),
            Format_ayuv            = AK_MAKE_FOURCC('A', 'Y', 'U', 'V'),
            Format_ayuv64be        = AK_MAKE_FOURCC(64, 'U', 'Y', 'A'),
            Format_ayuv64le        = AK_MAKE_FOURCC('A', 'Y', 'U', 64),
            Format_ayuvpackbe      = AK_MAKE_FOURCC('Q', 'U', 'Y', 'A'),
            Format_ayuvpackle      = AK_MAKE_FOURCC('A', 'Y', 'U', 'Q'),
            Format_bgr0            = AK_MAKE_FOURCC('B', 'G', 'R', 0),
            Format_bgr0packbe      = AK_MAKE_FOURCC('Q', 0, 'G', 'B'),
            Format_bgr0packle      = AK_MAKE_FOURCC('B', 'G', 0, 'Q'),
            Format_bgr0444be       = AK_MAKE_FOURCC(44, 4, 0, 'B'),
            Format_bgr0444le       = AK_MAKE_FOURCC('B', 0, 4, 44),
            Format_bgr0555be       = AK_MAKE_FOURCC(55, 5, 0, 'B'),
            Format_bgr0555le       = AK_MAKE_FOURCC('B', 0, 5, 55),
            Format_bgr233          = AK_MAKE_FOURCC('B', 'G', 2, 33),
            Format_bgr24           = AK_MAKE_FOURCC('B', 'G', 'R', 24),
            Format_bgr30be         = AK_MAKE_FOURCC(30, 'R', 'G', 'B'),
            Format_bgr30le         = AK_MAKE_FOURCC('B', 'G', 'R', 30),
            Format_bgr332          = AK_MAKE_FOURCC('B', 'G', 3, 32),
            Format_bgr444be        = AK_MAKE_FOURCC(44, 4, 'G', 'B'),
            Format_bgr444le        = AK_MAKE_FOURCC('B', 'G', 4, 44),
            Format_bgr48be         = AK_MAKE_FOURCC(48, 'R', 'G', 'B'),
            Format_bgr48le         = AK_MAKE_FOURCC('B', 'G', 'R', 48),
            Format_bgr555be        = AK_MAKE_FOURCC(55, 5, 'G', 'B'),
            Format_bgr555le        = AK_MAKE_FOURCC('B', 'G', 5, 55),
            Format_bgr565be        = AK_MAKE_FOURCC(56, 5, 'G', 'B'),
            Format_bgr565le        = AK_MAKE_FOURCC('B', 'G', 5, 65),
            Format_bgra            = AK_MAKE_FOURCC('B', 'G', 'R', 'A'),
            Format_bgrapackbe      = AK_MAKE_FOURCC('Q', 'A', 'G', 'B'),
            Format_bgrapackle      = AK_MAKE_FOURCC('B', 'G', 'A', 'Q'),
            Format_bgra4444be      = AK_MAKE_FOURCC(44, 44, 'A', 'B'),
            Format_bgra4444le      = AK_MAKE_FOURCC('B', 'A', 44, 44),
            Format_bgra5551be      = AK_MAKE_FOURCC(15, 55, 'A', 'B'),
            Format_bgra5551le      = AK_MAKE_FOURCC('B', 'A', 55, 51),
            Format_bgra64be        = AK_MAKE_FOURCC(64, 'A', 'G', 'B'),
            Format_bgra64le        = AK_MAKE_FOURCC('B', 'G', 'A', 64),
            Format_gbr24p          = AK_MAKE_FOURCC('G', 'B', 24, 'P'),
            Format_gbrap           = AK_MAKE_FOURCC('G', 'B', 'A', 'P'),
            Format_gbrap10be       = AK_MAKE_FOURCC('P', 10, 'A', 'G'),
            Format_gbrap10le       = AK_MAKE_FOURCC('G', 'A', 10, 'P'),
            Format_gbrap12be       = AK_MAKE_FOURCC('P', 12, 'A', 'G'),
            Format_gbrap12le       = AK_MAKE_FOURCC('G', 'A', 12, 'P'),
            Format_gbrap16be       = AK_MAKE_FOURCC('P', 16, 'A', 'G'),
            Format_gbrap16le       = AK_MAKE_FOURCC('G', 'A', 16, 'P'),
            Format_gbrp            = AK_MAKE_FOURCC('G', 'B', 'R', 'P'),
            Format_gbrp10be        = AK_MAKE_FOURCC('P', 10, 'B', 'G'),
            Format_gbrp10le        = AK_MAKE_FOURCC('G', 'B', 10, 'P'),
            Format_gbrp12be        = AK_MAKE_FOURCC('P', 12, 'B', 'G'),
            Format_gbrp12le        = AK_MAKE_FOURCC('G', 'B', 12, 'P'),
            Format_gbrp14be        = AK_MAKE_FOURCC('P', 14, 'B', 'G'),
            Format_gbrp14le        = AK_MAKE_FOURCC('G', 'B', 14, 'P'),
            Format_gbrp16be        = AK_MAKE_FOURCC('P', 16, 'B', 'G'),
            Format_gbrp16le        = AK_MAKE_FOURCC('G', 'B', 16, 'P'),
            Format_gbrp9be         = AK_MAKE_FOURCC('P', 9, 'B', 'G'),
            Format_gbrp9le         = AK_MAKE_FOURCC('G', 'B', 9, 'P'),
            Format_gray10be        = AK_MAKE_FOURCC(0, 0, 10, 'Y'),
            Format_gray10le        = AK_MAKE_FOURCC('Y', 10, 0, 0),
            Format_gray12be        = AK_MAKE_FOURCC(0, 0, 12, 'Y'),
            Format_gray12le        = AK_MAKE_FOURCC('Y', 12, 0, 0),
            Format_gray14be        = AK_MAKE_FOURCC(0, 0, 14, 'Y'),
            Format_gray14le        = AK_MAKE_FOURCC('Y', 14, 0, 0),
            Format_gray16be        = AK_MAKE_FOURCC(0, 0, 16, 'Y'),
            Format_gray16le        = AK_MAKE_FOURCC('Y', 16, 0, 0),
            Format_gray32be        = AK_MAKE_FOURCC(0, 0, 32, 'Y'),
            Format_gray32le        = AK_MAKE_FOURCC('Y', 32, 0, 0),
            Format_gray4           = AK_MAKE_FOURCC('Y', 4, 0, 0),
            Format_gray6           = AK_MAKE_FOURCC('Y', 6, 0, 0),
            Format_gray8           = AK_MAKE_FOURCC('Y', 8, 0, 0),
            Format_gray9be         = AK_MAKE_FOURCC(0, 0, 9, 'Y'),
            Format_gray9le         = AK_MAKE_FOURCC('Y', 9, 0, 0),
            Format_graya16be       = AK_MAKE_FOURCC(0, 16, 'A', 'Y'),
            Format_graya16le       = AK_MAKE_FOURCC('Y', 'A', 16, 0),
            Format_graya8          = AK_MAKE_FOURCC('Y', 'A', 8, 0),
            Format_graya8packbe    = AK_MAKE_FOURCC('Q', 8, 'A', 'Y'),
            Format_graya8packle    = AK_MAKE_FOURCC('Y', 'A', 8, 'Q'),
            Format_nv12            = AK_MAKE_FOURCC('N', 'V', 12, 0),
            Format_nv12a           = AK_MAKE_FOURCC('N', 'V', 12, 'A'),
            Format_nv16            = AK_MAKE_FOURCC('N', 'V', 16, 0),
            Format_nv20be          = AK_MAKE_FOURCC(0, 20, 'V', 'N'),
            Format_nv20le          = AK_MAKE_FOURCC('N', 'V', 20, 0),
            Format_nv21            = AK_MAKE_FOURCC('N', 'V', 21, 0),
            Format_nv24            = AK_MAKE_FOURCC('N', 'V', 24, 0),
            Format_nv42            = AK_MAKE_FOURCC('N', 'V', 42, 0),
            Format_nv61            = AK_MAKE_FOURCC('N', 'V', 61, 0),
            Format_p010be          = AK_MAKE_FOURCC(0, 1, 0, 'p'),
            Format_p010le          = AK_MAKE_FOURCC('p', 0, 1, 0),
            Format_p016be          = AK_MAKE_FOURCC(6, 1, 0, 'p'),
            Format_p016le          = AK_MAKE_FOURCC('p', 0, 1, 6),
            Format_p210be          = AK_MAKE_FOURCC(0, 1, 2, 'p'),
            Format_p210le          = AK_MAKE_FOURCC('p', 2, 1, 0),
            Format_p216be          = AK_MAKE_FOURCC(6, 1, 2, 'p'),
            Format_p216le          = AK_MAKE_FOURCC('p', 2, 1, 6),
            Format_p410be          = AK_MAKE_FOURCC(0, 1, 4, 'p'),
            Format_p410le          = AK_MAKE_FOURCC('p', 4, 1, 0),
            Format_p416be          = AK_MAKE_FOURCC(6, 1, 4, 'p'),
            Format_p416le          = AK_MAKE_FOURCC('p', 4, 1, 6),
            Format_rgb0            = AK_MAKE_FOURCC('R', 'G', 'B', 0),
            Format_rgb0444be       = AK_MAKE_FOURCC(44, 4, 0, 'R'),
            Format_rgb0444le       = AK_MAKE_FOURCC('R', 0, 4, 44),
            Format_rgb0555be       = AK_MAKE_FOURCC(55, 5, 0, 'R'),
            Format_rgb0555le       = AK_MAKE_FOURCC('R', 0, 5, 55),
            Format_rgb0packbe      = AK_MAKE_FOURCC('Q', 0, 'G', 'R'),
            Format_rgb0packle      = AK_MAKE_FOURCC('R', 'G', 0, 'Q'),
            Format_rgb233          = AK_MAKE_FOURCC('R', 'G', 2, 33),
            Format_rgb24           = AK_MAKE_FOURCC('R', 'G', 'B', 24),
            Format_rgb24p          = AK_MAKE_FOURCC('R', 'G', 24, 'P'),
            Format_rgb30be         = AK_MAKE_FOURCC(30, 'B', 'G', 'R'),
            Format_rgb30le         = AK_MAKE_FOURCC('R', 'G', 'B', 30),
            Format_rgb332          = AK_MAKE_FOURCC('R', 'G', 3, 32),
            Format_rgb444be        = AK_MAKE_FOURCC(44, 4, 'G', 'R'),
            Format_rgb444le        = AK_MAKE_FOURCC('R', 'G', 4, 44),
            Format_rgb48be         = AK_MAKE_FOURCC(48, 'B', 'G', 'R'),
            Format_rgb48le         = AK_MAKE_FOURCC('R', 'G', 'B', 48),
            Format_rgb5550be       = AK_MAKE_FOURCC(5, 55, 'G', 'R'),
            Format_rgb5550le       = AK_MAKE_FOURCC('R', 'G', 55, 50),
            Format_rgb555be        = AK_MAKE_FOURCC(55, 5, 'G', 'R'),
            Format_rgb555le        = AK_MAKE_FOURCC('R', 'G', 5, 55),
            Format_rgb565be        = AK_MAKE_FOURCC(5, 65, 'G', 'R'),
            Format_rgb565le        = AK_MAKE_FOURCC('R', 'G', 5, 65),
            Format_rgba            = AK_MAKE_FOURCC('R', 'G', 'B', 'A'),
            Format_rgba1010102be   = AK_MAKE_FOURCC('A', 10, 10, 12),
            Format_rgba1010102le   = AK_MAKE_FOURCC(210, 10, 10, 'A'),
            Format_rgba4444be      = AK_MAKE_FOURCC(44, 44, 'A', 'R'),
            Format_rgba4444le      = AK_MAKE_FOURCC('R', 'A', 44, 44),
            Format_rgba5551be      = AK_MAKE_FOURCC(55, 51, 'A', 'R'),
            Format_rgba5551le      = AK_MAKE_FOURCC('R', 'A', 15, 55),
            Format_rgba64be        = AK_MAKE_FOURCC(64, 'A', 'G', 'R'),
            Format_rgba64le        = AK_MAKE_FOURCC('R', 'G', 'A', 64),
            Format_rgbap           = AK_MAKE_FOURCC('R', 'G', 'A', 'P'),
            Format_rgbapackbe      = AK_MAKE_FOURCC('Q', 'A', 'G', 'R'),
            Format_rgbapackle      = AK_MAKE_FOURCC('R', 'G', 'A', 'Q'),
            Format_uyva            = AK_MAKE_FOURCC('U', 'Y', 'V', 'A'),
            Format_uyvy411         = AK_MAKE_FOURCC('U', 'Y', 4, 11),
            Format_uyvy422         = AK_MAKE_FOURCC('U', 'Y', 'V', 'Y'),
            Format_uyvy422a        = AK_MAKE_FOURCC('U', 'A', 4, 11),
            Format_vuy0            = AK_MAKE_FOURCC('V', 'U', 'Y', 0),
            Format_vuya            = AK_MAKE_FOURCC('V', 'U', 'Y', 'A'),
            Format_vyuy422         = AK_MAKE_FOURCC('V', 'Y', 'U', 'Y'),
            Format_y210be          = AK_MAKE_FOURCC(0, 1, 2, 'Y'),
            Format_y210le          = AK_MAKE_FOURCC('Y', 2, 1, 0),
            Format_yuv24           = AK_MAKE_FOURCC('Y', 'U', 'V', 24),
            Format_yuv30be         = AK_MAKE_FOURCC(30, 'V', 'U', 'Y'),
            Format_yuv30le         = AK_MAKE_FOURCC('Y', 'U', 'V', 30),
            Format_yuv410p         = AK_MAKE_FOURCC('Y', 4, 1, 0),
            Format_yuv411p         = AK_MAKE_FOURCC('Y', 4, 1, 1),
            Format_yuv420p         = AK_MAKE_FOURCC('Y', 4, 2, 0),
            Format_yuv420p10be     = AK_MAKE_FOURCC(10, 0, 2, 'Y'),
            Format_yuv420p10le     = AK_MAKE_FOURCC('Y', 2, 0, 10),
            Format_yuv420p12be     = AK_MAKE_FOURCC(12, 0, 2, 'Y'),
            Format_yuv420p12le     = AK_MAKE_FOURCC('Y', 2, 0, 12),
            Format_yuv420p14be     = AK_MAKE_FOURCC(14, 0, 2, 'Y'),
            Format_yuv420p14le     = AK_MAKE_FOURCC('Y', 2, 0, 14),
            Format_yuv420p16be     = AK_MAKE_FOURCC(16, 0, 2, 'Y'),
            Format_yuv420p16le     = AK_MAKE_FOURCC('Y', 2, 0, 16),
            Format_yuv420p9be      = AK_MAKE_FOURCC(9, 0, 2, 'Y'),
            Format_yuv420p9le      = AK_MAKE_FOURCC('Y', 2, 0, 9),
            Format_yuv422p         = AK_MAKE_FOURCC('Y', 4, 2, 2),
            Format_yuv422p10be     = AK_MAKE_FOURCC(10, 2, 2, 'Y'),
            Format_yuv422p10le     = AK_MAKE_FOURCC('Y', 2, 2, 10),
            Format_yuv422p12be     = AK_MAKE_FOURCC(12, 2, 2, 'Y'),
            Format_yuv422p12le     = AK_MAKE_FOURCC('Y', 2, 2, 12),
            Format_yuv422p14be     = AK_MAKE_FOURCC(14, 2, 2, 'Y'),
            Format_yuv422p14le     = AK_MAKE_FOURCC('Y', 2, 2, 14),
            Format_yuv422p16be     = AK_MAKE_FOURCC(16, 2, 2, 'Y'),
            Format_yuv422p16le     = AK_MAKE_FOURCC('Y', 2, 2, 16),
            Format_yuv422p9be      = AK_MAKE_FOURCC(9, 2, 2, 'Y'),
            Format_yuv422p9le      = AK_MAKE_FOURCC('Y', 2, 2, 9),
            Format_yuv440p         = AK_MAKE_FOURCC('Y', 4, 4, 0),
            Format_yuv440p10be     = AK_MAKE_FOURCC(10, 0, 4, 'Y'),
            Format_yuv440p10le     = AK_MAKE_FOURCC('Y', 4, 0, 10),
            Format_yuv440p12be     = AK_MAKE_FOURCC(12, 0, 4, 'Y'),
            Format_yuv440p12le     = AK_MAKE_FOURCC('Y', 4, 0, 12),
            Format_yuv444          = AK_MAKE_FOURCC('Y', 'U', 'V', 44),
            Format_yuv444p         = AK_MAKE_FOURCC('Y', 4, 4, 4),
            Format_yuv444p10be     = AK_MAKE_FOURCC(10, 4, 4, 'Y'),
            Format_yuv444p10le     = AK_MAKE_FOURCC('Y', 4, 4, 10),
            Format_yuv444p12be     = AK_MAKE_FOURCC(12, 4, 4, 'Y'),
            Format_yuv444p12le     = AK_MAKE_FOURCC('Y', 4, 4, 12),
            Format_yuv444p14be     = AK_MAKE_FOURCC(14, 4, 4, 'Y'),
            Format_yuv444p14le     = AK_MAKE_FOURCC('Y', 4, 4, 14),
            Format_yuv444p16be     = AK_MAKE_FOURCC(16, 4, 4, 'Y'),
            Format_yuv444p16le     = AK_MAKE_FOURCC('Y', 4, 4, 16),
            Format_yuv444p9be      = AK_MAKE_FOURCC(9, 4, 4, 'Y'),
            Format_yuv444p9le      = AK_MAKE_FOURCC('Y', 4, 4, 9),
            Format_yuv444packbe    = AK_MAKE_FOURCC(44, 4, 'U', 'Y'),
            Format_yuv444packle    = AK_MAKE_FOURCC('Y', 'U', 4, 44),
            Format_yuv555packbe    = AK_MAKE_FOURCC(55, 5, 'U', 'Y'),
            Format_yuv555packle    = AK_MAKE_FOURCC('Y', 'U', 5, 55),
            Format_yuv565packbe    = AK_MAKE_FOURCC(56, 5, 'U', 'Y'),
            Format_yuv565packle    = AK_MAKE_FOURCC('Y', 'U', 5, 65),
            Format_yuva420p        = AK_MAKE_FOURCC('A', 4, 2, 0),
            Format_yuva420p10be    = AK_MAKE_FOURCC(10, 0, 2, 'A'),
            Format_yuva420p10le    = AK_MAKE_FOURCC('A', 2, 0, 10),
            Format_yuva420p16be    = AK_MAKE_FOURCC(16, 0, 2, 'A'),
            Format_yuva420p16le    = AK_MAKE_FOURCC('A', 2, 0, 16),
            Format_yuva420p9be     = AK_MAKE_FOURCC(9, 0, 2, 'A'),
            Format_yuva420p9le     = AK_MAKE_FOURCC('A', 2, 0, 9),
            Format_yuva422p        = AK_MAKE_FOURCC('A', 4, 2, 2),
            Format_yuva422p10be    = AK_MAKE_FOURCC(10, 2, 2, 'A'),
            Format_yuva422p10le    = AK_MAKE_FOURCC('A', 2, 2, 10),
            Format_yuva422p12be    = AK_MAKE_FOURCC(12, 2, 2, 'A'),
            Format_yuva422p12le    = AK_MAKE_FOURCC('A', 2, 2, 12),
            Format_yuva422p16be    = AK_MAKE_FOURCC(16, 2, 2, 'A'),
            Format_yuva422p16le    = AK_MAKE_FOURCC('A', 2, 2, 16),
            Format_yuva422p9be     = AK_MAKE_FOURCC(9, 2, 2, 'A'),
            Format_yuva422p9le     = AK_MAKE_FOURCC('A', 2, 2, 9),
            Format_yuva444p        = AK_MAKE_FOURCC('A', 4, 4, 4),
            Format_yuva444p10be    = AK_MAKE_FOURCC(10, 4, 4, 'A'),
            Format_yuva444p10le    = AK_MAKE_FOURCC('A', 4, 4, 10),
            Format_yuva444p12be    = AK_MAKE_FOURCC(12, 4, 4, 'A'),
            Format_yuva444p12le    = AK_MAKE_FOURCC('A', 4, 4, 12),
            Format_yuva444p16be    = AK_MAKE_FOURCC(16, 4, 4, 'A'),
            Format_yuva444p16le    = AK_MAKE_FOURCC('A', 4, 4, 16),
            Format_yuva444p9be     = AK_MAKE_FOURCC(9, 4, 4, 'A'),
            Format_yuva444p9le     = AK_MAKE_FOURCC('A', 4, 4, 9),
            Format_yuyv211         = AK_MAKE_FOURCC('Y', 'U', 'Y', 1),
            Format_yuyv422         = AK_MAKE_FOURCC('Y', 'U', 'Y', 2),
            Format_yuyv422_32_10be = AK_MAKE_FOURCC('Y', 'U', 32, 10),
            Format_yuyv422_32_10le = AK_MAKE_FOURCC('Y', 'U', 32, 10),
            Format_yuyv422_32be    = AK_MAKE_FOURCC('Y', 'U', 'Y', 32),
            Format_yuyv422_32le    = AK_MAKE_FOURCC('Y', 'U', 'Y', 32),
            Format_yvu410p         = AK_MAKE_FOURCC('Y', 'V', 10, 'P'),
            Format_yvu420p         = AK_MAKE_FOURCC('Y', 'V', 20, 'P'),
            Format_yvu422p         = AK_MAKE_FOURCC('Y', 'V', 22, 'P'),
            Format_yvu444p         = AK_MAKE_FOURCC('Y', 'V', 44, 'P'),
            Format_yvyu422         = AK_MAKE_FOURCC('Y', 'V', 4, 22),

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            Format_0bgr444       = Format_0bgr444le,
            Format_0bgrpack      = Format_0bgrpackle,
            Format_0rgbpack      = Format_0rgbpackle,
            Format_abgr1555      = Format_abgr1555le,
            Format_abgr4444      = Format_abgr4444le,
            Format_abgrpack      = Format_abgrpackle,
            Format_argb1555      = Format_argb1555le,
            Format_argb2101010   = Format_argb2101010le,
            Format_argb4444      = Format_argb4444le,
            Format_argb64        = Format_argb64le,
            Format_argbpack      = Format_argbpackle,
            Format_ayuv64        = Format_ayuv64le,
            Format_ayuvpack      = Format_ayuvpackle,
            Format_bgr0pack      = Format_bgr0packle,
            Format_bgr0444       = Format_bgr0444le,
            Format_bgr0555       = Format_bgr0555le,
            Format_bgr30         = Format_bgr30le,
            Format_bgr444        = Format_bgr444le,
            Format_bgr48         = Format_bgr48le,
            Format_bgr555        = Format_bgr555le,
            Format_bgr565        = Format_bgr565le,
            Format_bgrapack      = Format_bgrapackle,
            Format_bgra4444      = Format_bgra4444le,
            Format_bgra5551      = Format_bgra5551le,
            Format_bgra64        = Format_bgra64le,
            Format_gbrap10       = Format_gbrap10le,
            Format_gbrap12       = Format_gbrap12le,
            Format_gbrap16       = Format_gbrap16le,
            Format_gbrp10        = Format_gbrp10le,
            Format_gbrp12        = Format_gbrp12le,
            Format_gbrp14        = Format_gbrp14le,
            Format_gbrp16        = Format_gbrp16le,
            Format_gbrp9         = Format_gbrp9le,
            Format_gray10        = Format_gray10le,
            Format_gray12        = Format_gray12le,
            Format_gray14        = Format_gray14le,
            Format_gray16        = Format_gray16le,
            Format_gray32        = Format_gray32le,
            Format_gray9         = Format_gray9le,
            Format_graya16       = Format_graya16le,
            Format_graya8pack    = Format_graya8packle,
            Format_nv20          = Format_nv20le,
            Format_p010          = Format_p010le,
            Format_p016          = Format_p016le,
            Format_p210          = Format_p210le,
            Format_p216          = Format_p216le,
            Format_p410          = Format_p410le,
            Format_p416          = Format_p416le,
            Format_rgb0444       = Format_rgb0444le,
            Format_rgb0555       = Format_rgb0555le,
            Format_rgb0pack      = Format_rgb0packle,
            Format_rgb30         = Format_rgb30le,
            Format_rgb444        = Format_rgb444le,
            Format_rgb48         = Format_rgb48le,
            Format_rgb555        = Format_rgb555le,
            Format_rgb5550       = Format_rgb5550le,
            Format_rgb565        = Format_rgb565le,
            Format_rgba1010102   = Format_rgba1010102le,
            Format_rgba4444      = Format_rgba4444le,
            Format_rgba5551      = Format_rgba5551le,
            Format_rgba64        = Format_rgba64le,
            Format_rgbapack      = Format_rgbapackle,
            Format_y210          = Format_y210le,
            Format_yuv30         = Format_yuv30le,
            Format_yuv420p10     = Format_yuv420p10le,
            Format_yuv420p12     = Format_yuv420p12le,
            Format_yuv420p14     = Format_yuv420p14le,
            Format_yuv420p16     = Format_yuv420p16le,
            Format_yuv420p9      = Format_yuv420p9le,
            Format_yuv422p10     = Format_yuv422p10le,
            Format_yuv422p12     = Format_yuv422p12le,
            Format_yuv422p14     = Format_yuv422p14le,
            Format_yuv422p16     = Format_yuv422p16le,
            Format_yuv422p9      = Format_yuv422p9le,
            Format_yuv440p10     = Format_yuv440p10le,
            Format_yuv440p12     = Format_yuv440p12le,
            Format_yuv444p10     = Format_yuv444p10le,
            Format_yuv444p12     = Format_yuv444p12le,
            Format_yuv444p14     = Format_yuv444p14le,
            Format_yuv444p16     = Format_yuv444p16le,
            Format_yuv444p9      = Format_yuv444p9le,
            Format_yuv444pack    = Format_yuv444packle,
            Format_yuv555pack    = Format_yuv555packle,
            Format_yuv565pack    = Format_yuv565packle,
            Format_yuva420p10    = Format_yuva420p10le,
            Format_yuva420p16    = Format_yuva420p16le,
            Format_yuva420p9     = Format_yuva420p9le,
            Format_yuva422p10    = Format_yuva422p10le,
            Format_yuva422p12    = Format_yuva422p12le,
            Format_yuva422p16    = Format_yuva422p16le,
            Format_yuva422p9     = Format_yuva422p9le,
            Format_yuva444p10    = Format_yuva444p10le,
            Format_yuva444p12    = Format_yuva444p12le,
            Format_yuva444p16    = Format_yuva444p16le,
            Format_yuva444p9     = Format_yuva444p9le,
            Format_yuyv422_32    = Format_yuyv422_32le,
            Format_yuyv422_32_10 = Format_yuyv422_32_10le,
#else
            Format_0bgr444       = Format_0bgr444be,
            Format_0bgrpack      = Format_0bgrpackbe,
            Format_0rgbpack      = Format_0rgbpackbe,
            Format_abgr1555      = Format_abgr1555be,
            Format_abgr4444      = Format_abgr4444be,
            Format_abgrpack      = Format_abgrpackbe,
            Format_argb1555      = Format_argb1555be,
            Format_argb2101010   = Format_argb2101010be,
            Format_argb4444      = Format_argb4444be,
            Format_argb64        = Format_argb64be,
            Format_argbpack      = Format_argbpackbe,
            Format_ayuv64        = Format_ayuv64be,
            Format_ayuvpack      = Format_ayuvpackbe,
            Format_bgr0pack      = Format_bgr0packbe,
            Format_bgr0444       = Format_bgr0444be,
            Format_bgr0555       = Format_bgr0555be,
            Format_bgr30         = Format_bgr30be,
            Format_bgr444        = Format_bgr444be,
            Format_bgr48         = Format_bgr48be,
            Format_bgr555        = Format_bgr555be,
            Format_bgr565        = Format_bgr565be,
            Format_bgrapack      = Format_bgrapackbe,
            Format_bgra4444      = Format_bgra4444be,
            Format_bgra5551      = Format_bgra5551be,
            Format_bgra64        = Format_bgra64be,
            Format_gbrap10       = Format_gbrap10be,
            Format_gbrap12       = Format_gbrap12be,
            Format_gbrap16       = Format_gbrap16be,
            Format_gbrp10        = Format_gbrp10be,
            Format_gbrp12        = Format_gbrp12be,
            Format_gbrp14        = Format_gbrp14be,
            Format_gbrp16        = Format_gbrp16be,
            Format_gbrp9         = Format_gbrp9be,
            Format_gray10        = Format_gray10be,
            Format_gray12        = Format_gray12be,
            Format_gray14        = Format_gray14be,
            Format_gray16        = Format_gray16be,
            Format_gray32        = Format_gray32be,
            Format_gray9         = Format_gray9be,
            Format_graya16       = Format_graya16be,
            Format_graya8pack    = Format_graya8packbe,
            Format_nv20          = Format_nv20be,
            Format_p010          = Format_p010be,
            Format_p016          = Format_p016be,
            Format_p210          = Format_p210be,
            Format_p216          = Format_p216be,
            Format_p410          = Format_p410be,
            Format_p416          = Format_p416be,
            Format_rgb0444       = Format_rgb0444be,
            Format_rgb0555       = Format_rgb0555be,
            Format_rgb0pack      = Format_rgb0packbe,
            Format_rgb30         = Format_rgb30be,
            Format_rgb444        = Format_rgb444be,
            Format_rgb48         = Format_rgb48be,
            Format_rgb555        = Format_rgb555be,
            Format_rgb5550       = Format_rgb5550be,
            Format_rgb565        = Format_rgb565be,
            Format_rgba1010102   = Format_rgba1010102be,
            Format_rgba4444      = Format_rgba4444be,
            Format_rgba5551      = Format_rgba5551be,
            Format_rgba64        = Format_rgba64be,
            Format_rgbapack      = Format_rgbapackbe,
            Format_y210          = Format_y210be,
            Format_yuv30         = Format_yuv30be,
            Format_yuv420p10     = Format_yuv420p10be,
            Format_yuv420p12     = Format_yuv420p12be,
            Format_yuv420p14     = Format_yuv420p14be,
            Format_yuv420p16     = Format_yuv420p16be,
            Format_yuv420p9      = Format_yuv420p9be,
            Format_yuv422p10     = Format_yuv422p10be,
            Format_yuv422p12     = Format_yuv422p12be,
            Format_yuv422p14     = Format_yuv422p14be,
            Format_yuv422p16     = Format_yuv422p16be,
            Format_yuv422p9      = Format_yuv422p9be,
            Format_yuv440p10     = Format_yuv440p10be,
            Format_yuv440p12     = Format_yuv440p12be,
            Format_yuv444p10     = Format_yuv444p10be,
            Format_yuv444p12     = Format_yuv444p12be,
            Format_yuv444p14     = Format_yuv444p14be,
            Format_yuv444p16     = Format_yuv444p16be,
            Format_yuv444p9      = Format_yuv444p9be,
            Format_yuv444pack    = Format_yuv444packbe,
            Format_yuv555pack    = Format_yuv555packbe,
            Format_yuv565pack    = Format_yuv565packbe,
            Format_yuva420p10    = Format_yuva420p10be,
            Format_yuva420p16    = Format_yuva420p16be,
            Format_yuva420p9     = Format_yuva420p9be,
            Format_yuva422p10    = Format_yuva422p10be,
            Format_yuva422p12    = Format_yuva422p12be,
            Format_yuva422p16    = Format_yuva422p16be,
            Format_yuva422p9     = Format_yuva422p9be,
            Format_yuva444p10    = Format_yuva444p10be,
            Format_yuva444p12    = Format_yuva444p12be,
            Format_yuva444p16    = Format_yuva444p16be,
            Format_yuva444p9     = Format_yuva444p9be,
            Format_yuyv422_32    = Format_yuyv422_32be,
            Format_yuyv422_32_10 = Format_yuyv422_32_10be,
#endif
        };
        Q_ENUM(PixelFormat)
        using PixelFormatList = QList<PixelFormat>;

        AkVideoCaps(QObject *parent=nullptr);
        AkVideoCaps(PixelFormat format,
                    int width,
                    int height,
                    const AkFrac &fps);
        AkVideoCaps(PixelFormat format,
                    const QSize &size,
                    const AkFrac &fps);
        AkVideoCaps(const AkCaps &other);
        AkVideoCaps(const AkVideoCaps &other);
        ~AkVideoCaps();
        AkVideoCaps &operator =(const AkCaps &other);
        AkVideoCaps &operator =(const AkVideoCaps &other);
        bool operator ==(const AkVideoCaps &other) const;
        bool operator !=(const AkVideoCaps &other) const;
        operator bool() const;
        operator AkCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkVideoCaps &caps);
        Q_INVOKABLE static QObject *create(AkVideoCaps::PixelFormat format,
                                           int width,
                                           int height,
                                           const AkFrac &fps);
        Q_INVOKABLE static QObject *create(AkVideoCaps::PixelFormat format,
                                           const QSize &size,
                                           const AkFrac &fps);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE AkVideoCaps::PixelFormat format() const;
        Q_INVOKABLE int bpp() const;
        Q_INVOKABLE QSize size() const;
        Q_INVOKABLE int width() const;
        Q_INVOKABLE int height() const;
        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE AkFrac &fps();

        Q_INVOKABLE AkVideoCaps nearest(const AkVideoCapsList &caps) const;
        Q_INVOKABLE bool isSameFormat(const AkVideoCaps &other) const;

        Q_INVOKABLE static int bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat);
        Q_INVOKABLE static QString pixelFormatToString(AkVideoCaps::PixelFormat pixelFormat);
        Q_INVOKABLE static AkVideoFormatSpec formatSpecs(AkVideoCaps::PixelFormat pixelFormat);

    private:
        AkVideoCapsPrivate *d;

    Q_SIGNALS:
        void formatChanged(AkVideoCaps::PixelFormat format);
        void sizeChanged(const QSize &size);
        void widthChanged(int width);
        void heightChanged(int height);
        void fpsChanged(const AkFrac &fps);

    public Q_SLOTS:
        void setFormat(AkVideoCaps::PixelFormat format);
        void setSize(const QSize &size);
        void setWidth(int width);
        void setHeight(int height);
        void setFps(const AkFrac &fps);
        void resetFormat();
        void resetSize();
        void resetWidth();
        void resetHeight();
        void resetFps();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkVideoCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoCaps::PixelFormat format);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps);

Q_DECLARE_METATYPE(AkVideoCaps)
Q_DECLARE_METATYPE(AkVideoCapsList)
Q_DECLARE_METATYPE(AkVideoCaps::PixelFormat)
Q_DECLARE_METATYPE(AkVideoCaps::PixelFormatList)

#endif // AKVIDEOCAPS_H
