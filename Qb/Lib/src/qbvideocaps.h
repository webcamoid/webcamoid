/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBVIDEOCAPS_H
#define QBVIDEOCAPS_H

#include "qbcaps.h"
#include "qbfrac.h"

class QbVideoCapsPrivate;

class QbVideoCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(PixelFormat)
    Q_PROPERTY(bool isValid
               READ isValid)
    Q_PROPERTY(PixelFormat format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
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
    Q_PROPERTY(QbFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        enum PixelFormat
        {
            Format_none = -1,
            Format_yuv420p,
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
            Format_xvmcmc,
            Format_xvmcidct,
            Format_uyvy422,
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
            Format_vdpau_h264,
            Format_vdpau_mpeg1,
            Format_vdpau_mpeg2,
            Format_vdpau_wmv3,
            Format_vdpau_vc1,
            Format_rgb48be,
            Format_rgb48le,
            Format_rgb565be,
            Format_rgb565le,
            Format_rgb555be,
            Format_rgb555le,
            Format_bgr565be,
            Format_bgr565le,
            Format_bgr555be,
            Format_bgr555le,
            Format_vaapi_moco,
            Format_vaapi_idct,
            Format_vaapi_vld,
            Format_yuv420p16le,
            Format_yuv420p16be,
            Format_yuv422p16le,
            Format_yuv422p16be,
            Format_yuv444p16le,
            Format_yuv444p16be,
            Format_vdpau_mpeg4,
            Format_dxva2_vld,
            Format_rgb444le,
            Format_rgb444be,
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
            Format_vda_vld,
            Format_gbrp,
            Format_gbrp9be,
            Format_gbrp9le,
            Format_gbrp10be,
            Format_gbrp10le,
            Format_gbrp16be,
            Format_gbrp16le,
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
            Format_vdpau,
            Format_xyz12le,
            Format_xyz12be,
            Format_nv16,
            Format_nv20le,
            Format_nv20be,
            Format_yvyu422,
            Format_vda,
            Format_ya16be,
            Format_ya16le,
            Format_qsv,
            Format_mmal,
            Format_d3d11va_vld,
            Format_rgba64be,
            Format_rgba64le,
            Format_bgra64be,
            Format_bgra64le,
            Format_0rgb,
            Format_rgb0,
            Format_0bgr,
            Format_bgr0,
            Format_yuva444p,
            Format_yuva422p,
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
            Format_gbrap,
            Format_gbrap16be,
            Format_gbrap16le,
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
            Format_yuv440p12be
        };

        explicit QbVideoCaps(QObject *parent=NULL);
        QbVideoCaps(const QVariantMap &caps);
        QbVideoCaps(const QString &caps);
        QbVideoCaps(const QbCaps &caps);
        QbVideoCaps(const QbVideoCaps &other);
        ~QbVideoCaps();
        QbVideoCaps &operator =(const QbVideoCaps &other);
        QbVideoCaps &operator =(const QbCaps &caps);
        QbVideoCaps &operator =(const QString &caps);
        bool operator ==(const QbVideoCaps &other) const;
        bool operator !=(const QbVideoCaps &other) const;
        operator bool() const;

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE bool &isValid();
        Q_INVOKABLE PixelFormat format() const;
        Q_INVOKABLE PixelFormat &format();
        Q_INVOKABLE int width() const;
        Q_INVOKABLE int &width();
        Q_INVOKABLE int height() const;
        Q_INVOKABLE int &height();
        Q_INVOKABLE QbFrac fps() const;
        Q_INVOKABLE QbFrac &fps();

        Q_INVOKABLE QbVideoCaps &fromMap(const QVariantMap &caps);
        Q_INVOKABLE QbVideoCaps &fromString(const QString &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbVideoCaps &update(const QbCaps &caps);
        Q_INVOKABLE QbCaps toCaps() const;

        Q_INVOKABLE static QString pixelFormatToString(PixelFormat pixelFormat);
        Q_INVOKABLE static PixelFormat pixelFormatFromString(const QString &pixelFormat);

    private:
        QbVideoCapsPrivate *d;

    signals:
        void formatChanged(PixelFormat format);
        void widthChanged(int width);
        void heightChanged(int height);
        void fpsChanged(const QbFrac &fps);

    public slots:
        void setFormat(PixelFormat format);
        void setWidth(int width);
        void setHeight(int height);
        void setFps(const QbFrac &fps);
        void resetFormat();
        void resetWidth();
        void resetHeight();
        void resetFps();
        void clear();

    friend QDebug operator <<(QDebug debug, const QbVideoCaps &caps);
    friend QDataStream &operator >>(QDataStream &istream, QbVideoCaps &caps);
    friend QDataStream &operator <<(QDataStream &ostream, const QbVideoCaps &caps);
};

QDebug operator <<(QDebug debug, const QbVideoCaps &caps);
QDataStream &operator >>(QDataStream &istream, QbVideoCaps &caps);
QDataStream &operator <<(QDataStream &ostream, const QbVideoCaps &caps);

Q_DECLARE_METATYPE(QbVideoCaps)
Q_DECLARE_METATYPE(QbVideoCaps::PixelFormat)

#endif // QBVIDEOCAPS_H
