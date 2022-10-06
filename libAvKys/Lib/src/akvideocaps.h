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

#define AkFourCC(a, b, c, d) \
    (((quint32(a) & 0xff) << 24) \
   | ((quint32(b) & 0xff) << 16) \
   | ((quint32(c) & 0xff) <<  8) \
   |  (quint32(d) & 0xff))

#define AK_FOURCC_NULL AkFourCC(0, 0, 0, 0)

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
            Format_none = -1,
            Format_0bgr,
            Format_0bgr444be,
            Format_0bgr444le,
            Format_0rgb,
            Format_0rgbpackbe,
            Format_0rgbpackle,
            Format_0yuv,
            Format_abgr,
            Format_abgr1555be,
            Format_abgr1555le,
            Format_abgr4444be,
            Format_abgr4444le,
            Format_argb,
            Format_argbpackbe,
            Format_argbpackle,
            Format_argb1555be,
            Format_argb1555le,
            Format_argb4444be,
            Format_argb4444le,
            Format_ayuv,
            Format_ayuv64be,
            Format_ayuv64le,
            Format_bgr0,
            Format_bgr0444be,
            Format_bgr0444le,
            Format_bgr0555be,
            Format_bgr0555le,
            Format_bgr24,
            Format_bgr30be,
            Format_bgr30le,
            Format_bgr444be,
            Format_bgr444le,
            Format_bgr48be,
            Format_bgr48le,
            Format_bgr555be,
            Format_bgr555le,
            Format_bgr565be,
            Format_bgr565le,
            Format_bgr233,
            Format_bgr332,
            Format_bgra,
            Format_bgra4444be,
            Format_bgra4444le,
            Format_bgra5551be,
            Format_bgra5551le,
            Format_bgra64be,
            Format_bgra64le,
            Format_gbr24p,
            Format_gbrap,
            Format_gbrap10be,
            Format_gbrap10le,
            Format_gbrap12be,
            Format_gbrap12le,
            Format_gbrap16be,
            Format_gbrap16le,
            Format_gbrp,
            Format_gbrp10be,
            Format_gbrp10le,
            Format_gbrp12be,
            Format_gbrp12le,
            Format_gbrp14be,
            Format_gbrp14le,
            Format_gbrp16be,
            Format_gbrp16le,
            Format_gbrp9be,
            Format_gbrp9le,
            Format_gray4,
            Format_gray6,
            Format_gray8,
            Format_gray9be,
            Format_gray9le,
            Format_gray10be,
            Format_gray10le,
            Format_gray12be,
            Format_gray12le,
            Format_gray14be,
            Format_gray14le,
            Format_gray16be,
            Format_gray16le,
            Format_gray32be,
            Format_gray32le,
            Format_graya16be,
            Format_graya16le,
            Format_graya8,
            Format_nv12,
            Format_nv16,
            Format_nv20be,
            Format_nv20le,
            Format_nv21,
            Format_nv24,
            Format_nv42,
            Format_nv61,
            Format_p010be,
            Format_p010le,
            Format_p016be,
            Format_p016le,
            Format_p210be,
            Format_p210le,
            Format_p216be,
            Format_p216le,
            Format_p410be,
            Format_p410le,
            Format_p416be,
            Format_p416le,
            Format_rgb0,
            Format_rgb0444be,
            Format_rgb0444le,
            Format_rgb24,
            Format_rgb24p,
            Format_rgb30be,
            Format_rgb30le,
            Format_rgb444be,
            Format_rgb444le,
            Format_rgb48be,
            Format_rgb48le,
            Format_rgb555be,
            Format_rgb555le,
            Format_rgb565be,
            Format_rgb565le,
            Format_rgb233,
            Format_rgb332,
            Format_rgba,
            Format_rgba4444be,
            Format_rgba4444le,
            Format_rgb0555be,
            Format_rgb0555le,
            Format_rgba5551be,
            Format_rgba5551le,
            Format_rgba64be,
            Format_rgba64le,
            Format_rgbap,
            Format_uyvy422,
            Format_y210be,
            Format_y210le,
            Format_vuya,
            Format_vuy0,
            Format_vyuy422,
            Format_yuv24,
            Format_yuv410p,
            Format_yuv411p,
            Format_yuv420p,
            Format_yuv420p10be,
            Format_yuv420p10le,
            Format_yuv420p12be,
            Format_yuv420p12le,
            Format_yuv420p14be,
            Format_yuv420p14le,
            Format_yuv420p16be,
            Format_yuv420p16le,
            Format_yuv420p9be,
            Format_yuv420p9le,
            Format_yuv422p,
            Format_yuv422p10be,
            Format_yuv422p10le,
            Format_yuv422p12be,
            Format_yuv422p12le,
            Format_yuv422p14be,
            Format_yuv422p14le,
            Format_yuv422p16be,
            Format_yuv422p16le,
            Format_yuv422p9be,
            Format_yuv422p9le,
            Format_yuv440p,
            Format_yuv440p10be,
            Format_yuv440p10le,
            Format_yuv440p12be,
            Format_yuv440p12le,
            Format_yuv444,
            Format_yuv444p,
            Format_yuv444p10be,
            Format_yuv444p10le,
            Format_yuv444p12be,
            Format_yuv444p12le,
            Format_yuv444p14be,
            Format_yuv444p14le,
            Format_yuv444p16be,
            Format_yuv444p16le,
            Format_yuv444p9be,
            Format_yuv444p9le,
            Format_yuv444packbe,
            Format_yuv444packle,
            Format_yuv555packbe,
            Format_yuv555packle,
            Format_yuv565packbe,
            Format_yuv565packle,
            Format_yuva420p,
            Format_yuva420p10be,
            Format_yuva420p10le,
            Format_yuva420p16be,
            Format_yuva420p16le,
            Format_yuva420p9be,
            Format_yuva420p9le,
            Format_yuva422p,
            Format_yuva422p10be,
            Format_yuva422p10le,
            Format_yuva422p12be,
            Format_yuva422p12le,
            Format_yuva422p16be,
            Format_yuva422p16le,
            Format_yuva422p9be,
            Format_yuva422p9le,
            Format_yuva444p,
            Format_yuva444p10be,
            Format_yuva444p10le,
            Format_yuva444p12be,
            Format_yuva444p12le,
            Format_yuva444p16be,
            Format_yuva444p16le,
            Format_yuva444p9be,
            Format_yuva444p9le,
            Format_yuyv422,
            Format_yvu410p,
            Format_yvu420p,
            Format_yvu422p,
            Format_yvu444p,
            Format_yvyu422,

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            Format_0bgr444    = Format_0bgr444le,
            Format_0rgbpack   = Format_0rgbpackle,
            Format_abgr1555   = Format_abgr1555le,
            Format_abgr4444   = Format_abgr4444le,
            Format_argbpack   = Format_argbpackle,
            Format_argb1555   = Format_argb1555le,
            Format_argb4444   = Format_argb4444le,
            Format_ayuv64     = Format_ayuv64le,
            Format_bgr0444    = Format_bgr0444le,
            Format_bgr0555    = Format_bgr0555le,
            Format_bgr444     = Format_bgr444le,
            Format_bgr30      = Format_bgr30le,
            Format_bgr48      = Format_bgr48le,
            Format_bgr555     = Format_bgr555le,
            Format_bgr565     = Format_bgr565le,
            Format_bgra4444   = Format_bgra4444le,
            Format_bgra5551   = Format_bgra5551le,
            Format_bgra64     = Format_bgra64le,
            Format_gbrap10    = Format_gbrap10le,
            Format_gbrap12    = Format_gbrap12le,
            Format_gbrap16    = Format_gbrap16le,
            Format_gbrp10     = Format_gbrp10le,
            Format_gbrp12     = Format_gbrp12le,
            Format_gbrp14     = Format_gbrp14le,
            Format_gbrp16     = Format_gbrp16le,
            Format_gbrp9      = Format_gbrp9le,
            Format_gray9      = Format_gray9le,
            Format_gray10     = Format_gray10le,
            Format_gray12     = Format_gray12le,
            Format_gray14     = Format_gray14le,
            Format_gray16     = Format_gray16le,
            Format_gray32     = Format_gray32le,
            Format_graya16    = Format_graya16le,
            Format_nv20       = Format_nv20le,
            Format_p010       = Format_p010le,
            Format_p016       = Format_p016le,
            Format_p210       = Format_p210le,
            Format_p216       = Format_p216le,
            Format_p410       = Format_p410le,
            Format_p416       = Format_p416le,
            Format_rgb0444    = Format_rgb0444le,
            Format_rgb444     = Format_rgb444le,
            Format_rgb30      = Format_rgb30le,
            Format_rgb48      = Format_rgb48le,
            Format_rgb555     = Format_rgb555le,
            Format_rgb565     = Format_rgb565le,
            Format_rgba4444   = Format_rgba4444le,
            Format_rgb0555    = Format_rgb0555le,
            Format_rgba5551   = Format_rgba5551le,
            Format_rgba64     = Format_rgba64le,
            Format_y210       = Format_y210le,
            Format_yuv420p10  = Format_yuv420p10le,
            Format_yuv420p12  = Format_yuv420p12le,
            Format_yuv420p14  = Format_yuv420p14le,
            Format_yuv420p16  = Format_yuv420p16le,
            Format_yuv420p9   = Format_yuv420p9le,
            Format_yuv422p10  = Format_yuv422p10le,
            Format_yuv422p12  = Format_yuv422p12le,
            Format_yuv422p14  = Format_yuv422p14le,
            Format_yuv422p16  = Format_yuv422p16le,
            Format_yuv422p9   = Format_yuv422p9le,
            Format_yuv440p10  = Format_yuv440p10le,
            Format_yuv440p12  = Format_yuv440p12le,
            Format_yuv444p10  = Format_yuv444p10le,
            Format_yuv444p12  = Format_yuv444p12le,
            Format_yuv444p14  = Format_yuv444p14le,
            Format_yuv444p16  = Format_yuv444p16le,
            Format_yuv444p9   = Format_yuv444p9le,
            Format_yuv444pack = Format_yuv444packle,
            Format_yuv555pack = Format_yuv555packle,
            Format_yuv565pack = Format_yuv565packle,
            Format_yuva420p10 = Format_yuva420p10le,
            Format_yuva420p16 = Format_yuva420p16le,
            Format_yuva420p9  = Format_yuva420p9le,
            Format_yuva422p10 = Format_yuva422p10le,
            Format_yuva422p12 = Format_yuva422p12le,
            Format_yuva422p16 = Format_yuva422p16le,
            Format_yuva422p9  = Format_yuva422p9le,
            Format_yuva444p10 = Format_yuva444p10le,
            Format_yuva444p12 = Format_yuva444p12le,
            Format_yuva444p16 = Format_yuva444p16le,
            Format_yuva444p9  = Format_yuva444p9le,
#else
            Format_0bgr444    = Format_0bgr444be,
            Format_0rgbpack   = Format_0rgbpackbe,
            Format_argb1555   = Format_argb1555be,
            Format_argb4444   = Format_argb4444be,
            Format_abgr1555   = Format_abgr1555be,
            Format_abgr4444   = Format_abgr4444be,
            Format_argbpack   = Format_argbpackbe,
            Format_ayuv64     = Format_ayuv64be,
            Format_bgr0444    = Format_bgr0444be,
            Format_bgr0555    = Format_bgr0555be,
            Format_bgr444     = Format_bgr444be,
            Format_bgr30      = Format_bgr30be,
            Format_bgr48      = Format_bgr48be,
            Format_bgr555     = Format_bgr555be,
            Format_bgr565     = Format_bgr565be,
            Format_bgra4444   = Format_bgra4444be,
            Format_bgra5551   = Format_bgra5551be,
            Format_bgra64     = Format_bgra64be,
            Format_gbrap10    = Format_gbrap10be,
            Format_gbrap12    = Format_gbrap12be,
            Format_gbrap16    = Format_gbrap16be,
            Format_gbrp10     = Format_gbrp10be,
            Format_gbrp12     = Format_gbrp12be,
            Format_gbrp14     = Format_gbrp14be,
            Format_gbrp16     = Format_gbrp16be,
            Format_gbrp9      = Format_gbrp9be,
            Format_gray9      = Format_gray9be,
            Format_gray10     = Format_gray10be,
            Format_gray12     = Format_gray12be,
            Format_gray14     = Format_gray14be,
            Format_gray16     = Format_gray16be,
            Format_gray32     = Format_gray32be,
            Format_graya16    = Format_graya16be,
            Format_nv20       = Format_nv20be,
            Format_p010       = Format_p010be,
            Format_p016       = Format_p016be,
            Format_p210       = Format_p210be,
            Format_p216       = Format_p216be,
            Format_p410       = Format_p410be,
            Format_p416       = Format_p416be,
            Format_rgb0444    = Format_rgb0444be,
            Format_rgb444     = Format_rgb444be,
            Format_rgb30      = Format_rgb30be,
            Format_rgb48      = Format_rgb48be,
            Format_rgb555     = Format_rgb555be,
            Format_rgb565     = Format_rgb565be,
            Format_rgba4444   = Format_rgba4444be,
            Format_rgb0555    = Format_rgb0555be,
            Format_rgba5551   = Format_rgba5551be,
            Format_rgba64     = Format_rgba64be,
            Format_y210       = Format_y210be,
            Format_yuv420p10  = Format_yuv420p10be,
            Format_yuv420p12  = Format_yuv420p12be,
            Format_yuv420p14  = Format_yuv420p14be,
            Format_yuv420p16  = Format_yuv420p16be,
            Format_yuv420p9   = Format_yuv420p9be,
            Format_yuv422p10  = Format_yuv422p10be,
            Format_yuv422p12  = Format_yuv422p12be,
            Format_yuv422p14  = Format_yuv422p14be,
            Format_yuv422p16  = Format_yuv422p16be,
            Format_yuv422p9   = Format_yuv422p9be,
            Format_yuv440p10  = Format_yuv440p10be,
            Format_yuv440p12  = Format_yuv440p12be,
            Format_yuv444p10  = Format_yuv444p10be,
            Format_yuv444p12  = Format_yuv444p12be,
            Format_yuv444p14  = Format_yuv444p14be,
            Format_yuv444p16  = Format_yuv444p16be,
            Format_yuv444p9   = Format_yuv444p9be,
            Format_yuv444pack = Format_yuv444packbe,
            Format_yuv555pack = Format_yuv555packbe,
            Format_yuv565pack = Format_yuv565packbe,
            Format_yuva420p10 = Format_yuva420p10be,
            Format_yuva420p16 = Format_yuva420p16be,
            Format_yuva420p9  = Format_yuva420p9be,
            Format_yuva422p10 = Format_yuva422p10be,
            Format_yuva422p12 = Format_yuva422p12be,
            Format_yuva422p16 = Format_yuva422p16be,
            Format_yuva422p9  = Format_yuva422p9be,
            Format_yuva444p10 = Format_yuva444p10be,
            Format_yuva444p12 = Format_yuva444p12be,
            Format_yuva444p16 = Format_yuva444p16be,
            Format_yuva444p9  = Format_yuva444p9be,
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
        Q_INVOKABLE static QObject *create(const QString &format,
                                           int width,
                                           int height,
                                           const AkFrac &fps);
        Q_INVOKABLE static QObject *create(AkVideoCaps::PixelFormat format,
                                           const QSize &size,
                                           const AkFrac &fps);
        Q_INVOKABLE static QObject *create(const QString &format,
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
        Q_INVOKABLE static int bitsPerPixel(const QString &pixelFormat);
        Q_INVOKABLE static QString pixelFormatToString(AkVideoCaps::PixelFormat pixelFormat);
        Q_INVOKABLE static AkVideoCaps::PixelFormat pixelFormatFromString(const QString &pixelFormat);
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
