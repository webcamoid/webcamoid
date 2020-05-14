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

inline quint32 AkFourCCS(const QString &fourcc)
{
    if (fourcc.size() != 4)
        return 0;

    return AkFourCC(fourcc[0].toLatin1(),
                    fourcc[1].toLatin1(),
                    fourcc[2].toLatin1(),
                    fourcc[3].toLatin1());
}

#define AkFourCCR(a, b, c, d) \
    (((quint32(d) & 0xff) << 24) \
   | ((quint32(c) & 0xff) << 16) \
   | ((quint32(b) & 0xff) <<  8) \
   |  (quint32(a) & 0xff))

inline quint32 AkFourCCRS(const QString &fourcc)
{
    if (fourcc.size() != 4)
        return 0;

    return AkFourCCR(fourcc[0].toLatin1(),
                     fourcc[1].toLatin1(),
                     fourcc[2].toLatin1(),
                     fourcc[3].toLatin1());
}

#define AK_FOURCC_NULL AkFourCC(0, 0, 0, 0)

class AkVideoCaps;
class AkVideoCapsPrivate;
class AkCaps;
class AkFrac;

using AkVideoCapsList = QList<AkVideoCaps>;

class AKCOMMONS_EXPORT AkVideoCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(PixelFormat)
    Q_PROPERTY(PixelFormat format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
    Q_PROPERTY(int bpp
               READ bpp)
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
    Q_PROPERTY(int align
               READ align
               WRITE setAlign
               RESET resetAlign
               NOTIFY alignChanged)
    Q_PROPERTY(size_t pictureSize
               READ pictureSize)
    Q_PROPERTY(int planes
               READ planes)

    public:
        enum PixelFormat
        {
            Format_none = -1,
            Format_yuv420p,
            Format_yvu420p,
            Format_yuyv422,
            Format_rgb24,
            Format_bgr24,
            Format_yuv422p,
            Format_yuv444p,
            Format_yuv410p,
            Format_yuv411p,
            Format_gray,
            Format_monow,
            Format_monob,
            Format_pal8,
            Format_yuvj420p,
            Format_yuvj422p,
            Format_yuvj444p,
            Format_uyvy422,
            Format_vyuy422,
            Format_uyyvyy411,
            Format_bgr8,
            Format_bgr4,
            Format_bgr4_byte,
            Format_rgb8,
            Format_rgb4,
            Format_rgb4_byte,
            Format_nv12,
            Format_nv21,
            Format_argb,
            Format_rgba,
            Format_abgr,
            Format_bgra,
            Format_gray16be,
            Format_gray16le,
            Format_yuv440p,
            Format_yuvj440p,
            Format_yuva420p,
            Format_rgb48be,
            Format_rgb48le,
            Format_rgb565be,
            Format_rgb565le,
            Format_rgb555be,
            Format_rgb555le,
            Format_argb555be,
            Format_argb555le,
            Format_bgr565be,
            Format_bgr565le,
            Format_bgr555be,
            Format_bgr555le,
            Format_rgb666,
            Format_argb1665,
            Format_argb1666,
            Format_bgr666,
            Format_argb6666,
            Format_abgr6666,
            Format_yuv420p16le,
            Format_yuv420p16be,
            Format_yuv422p16le,
            Format_yuv422p16be,
            Format_yuv444p16le,
            Format_yuv444p16be,
            Format_rgb444le,
            Format_rgb444be,
            Format_argb444be,
            Format_argb444le,
            Format_bgr444le,
            Format_bgr444be,
            Format_ya8,
            Format_bgr48be,
            Format_bgr48le,
            Format_yuv420p9be,
            Format_yuv420p9le,
            Format_yuv420p10be,
            Format_yuv420p10le,
            Format_yuv422p10be,
            Format_yuv422p10le,
            Format_yuv444p9be,
            Format_yuv444p9le,
            Format_yuv444p10be,
            Format_yuv444p10le,
            Format_yuv422p9be,
            Format_yuv422p9le,
            Format_gbrp,
            Format_gbrp9be,
            Format_gbrp9le,
            Format_gbrp10be,
            Format_gbrp10le,
            Format_gbrp16be,
            Format_gbrp16le,
            Format_yuva422p,
            Format_yuva444p,
            Format_yuva420p9be,
            Format_yuva420p9le,
            Format_yuva422p9be,
            Format_yuva422p9le,
            Format_yuva444p9be,
            Format_yuva444p9le,
            Format_yuva420p10be,
            Format_yuva420p10le,
            Format_yuva422p10be,
            Format_yuva422p10le,
            Format_yuva444p10be,
            Format_yuva444p10le,
            Format_yuva420p16be,
            Format_yuva420p16le,
            Format_yuva422p16be,
            Format_yuva422p16le,
            Format_yuva444p16be,
            Format_yuva444p16le,
            Format_xyz12le,
            Format_xyz12be,
            Format_nv16,
            Format_nv20le,
            Format_nv20be,
            Format_rgba64be,
            Format_rgba64le,
            Format_bgra64be,
            Format_bgra64le,
            Format_yvyu422,
            Format_ya16be,
            Format_ya16le,
            Format_gbrap,
            Format_gbrap16be,
            Format_gbrap16le,
            Format_0rgb,
            Format_rgb0,
            Format_0bgr,
            Format_bgr0,
            Format_yuv420p12be,
            Format_yuv420p12le,
            Format_yuv420p14be,
            Format_yuv420p14le,
            Format_yuv422p12be,
            Format_yuv422p12le,
            Format_yuv422p14be,
            Format_yuv422p14le,
            Format_yuv444p12be,
            Format_yuv444p12le,
            Format_yuv444p14be,
            Format_yuv444p14le,
            Format_gbrp12be,
            Format_gbrp12le,
            Format_gbrp14be,
            Format_gbrp14le,
            Format_yuvj411p,
            Format_bayer_bggr8,
            Format_bayer_rggb8,
            Format_bayer_gbrg8,
            Format_bayer_grbg8,
            Format_bayer_bggr16le,
            Format_bayer_bggr16be,
            Format_bayer_rggb16le,
            Format_bayer_rggb16be,
            Format_bayer_gbrg16le,
            Format_bayer_gbrg16be,
            Format_bayer_grbg16le,
            Format_bayer_grbg16be,
            Format_yuv440p10le,
            Format_yuv440p10be,
            Format_yuv440p12le,
            Format_yuv440p12be,
            Format_ayuv64le,
            Format_ayuv64be,
            Format_p010le,
            Format_p010be,
            Format_gbrap12be,
            Format_gbrap12le,
            Format_gbrap10be,
            Format_gbrap10le,
            Format_gray12be,
            Format_gray12le,
            Format_gray10be,
            Format_gray10le,
            Format_p016le,
            Format_p016be,
            Format_gray9be,
            Format_gray9le,
            Format_gbrpf32be,
            Format_gbrpf32le,
            Format_gbrapf32be,
            Format_gbrapf32le,
            Format_gray14be,
            Format_gray14le,
            Format_grayf32be,
            Format_grayf32le,
            Format_rgbp,
            Format_rgbap,
            Format_argb1887,
            Format_bgra1888,
            Format_yuv444,
            Format_gray2,
            Format_gray4,
            Format_gray24,
            Format_gray32,
        };
        using PixelFormatList = QList<PixelFormat>;

        AkVideoCaps(QObject *parent=nullptr);
        AkVideoCaps(PixelFormat format,
                    int width,
                    int height,
                    const AkFrac &fps,
                    int align=1);
        AkVideoCaps(PixelFormat format,
                    const QSize &size,
                    const AkFrac &fps,
                    int align=1);
        AkVideoCaps(const AkCaps &caps);
        AkVideoCaps(const AkVideoCaps &other);
        ~AkVideoCaps();
        AkVideoCaps &operator =(const AkVideoCaps &other);
        AkVideoCaps &operator =(const AkCaps &caps);
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
                                           const AkFrac &fps,
                                           int align=1);
        Q_INVOKABLE static QObject *create(const QString &format,
                                           int width,
                                           int height,
                                           const AkFrac &fps,
                                           int align=1);
        Q_INVOKABLE static QObject *create(AkVideoCaps::PixelFormat format,
                                           const QSize &size,
                                           const AkFrac &fps,
                                           int align=1);
        Q_INVOKABLE static QObject *create(const QString &format,
                                           const QSize &size,
                                           const AkFrac &fps,
                                           int align=1);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE PixelFormat format() const;
        Q_INVOKABLE quint32 fourCC() const;
        Q_INVOKABLE int bpp() const;
        Q_INVOKABLE QSize size() const;
        Q_INVOKABLE int width() const;
        Q_INVOKABLE int height() const;
        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE AkFrac &fps();
        Q_INVOKABLE int align() const;
        Q_INVOKABLE size_t pictureSize() const;

        Q_INVOKABLE static AkVideoCaps fromMap(const QVariantMap &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE AkVideoCaps &update(const AkCaps &caps);
        Q_INVOKABLE size_t planeOffset(int plane) const;
        Q_INVOKABLE size_t lineOffset(int plane, int y) const;
        Q_INVOKABLE size_t bytesPerLine(int plane) const;
        Q_INVOKABLE int planes() const;
        Q_INVOKABLE size_t planeSize(int plane) const;

        Q_INVOKABLE static int bitsPerPixel(PixelFormat pixelFormat);
        Q_INVOKABLE static int bitsPerPixel(const QString &pixelFormat);
        Q_INVOKABLE static QString pixelFormatToString(PixelFormat pixelFormat);
        Q_INVOKABLE static PixelFormat pixelFormatFromString(const QString &pixelFormat);
        Q_INVOKABLE static quint32 fourCC(PixelFormat pixelFormat);
        Q_INVOKABLE static quint32 fourCC(const QString &pixelFormat);

    private:
        AkVideoCapsPrivate *d;

    Q_SIGNALS:
        void formatChanged(PixelFormat format);
        void sizeChanged(const QSize &size);
        void widthChanged(int width);
        void heightChanged(int height);
        void fpsChanged(const AkFrac &fps);
        void alignChanged(int height);

    public Q_SLOTS:
        void setFormat(PixelFormat format);
        void setSize(const QSize &size);
        void setWidth(int width);
        void setHeight(int height);
        void setFps(const AkFrac &fps);
        void setAlign(int align);
        void resetFormat();
        void resetSize();
        void resetWidth();
        void resetHeight();
        void resetFps();
        void resetAlign();
        void clear();
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
