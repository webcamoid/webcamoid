/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
#include <QDir>
#include <QFileSystemWatcher>
#include <QMap>
#include <QReadWriteLock>
#include <QVariant>
#include <QVector>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <linux/videodev2.h>

#include "capturev4l2.h"
#include "ioctldefs.h"

#ifdef HAVE_LIBUSB
#include "uvcextendedcontrols.h"
#endif

using V4l2CtrlTypeMap = QMap<v4l2_ctrl_type, QString>;

inline V4l2CtrlTypeMap initV4l2CtrlTypeMap()
{
    V4l2CtrlTypeMap ctrlTypeToStr = {
        // V4L2 controls
        {V4L2_CTRL_TYPE_INTEGER     , "integer"    },
        {V4L2_CTRL_TYPE_BOOLEAN     , "boolean"    },
        {V4L2_CTRL_TYPE_MENU        , "menu"       },
        {V4L2_CTRL_TYPE_BUTTON      , "button"     },
        {V4L2_CTRL_TYPE_INTEGER64   , "integer64"  },
        {V4L2_CTRL_TYPE_CTRL_CLASS  , "ctrlClass"  },
        {V4L2_CTRL_TYPE_STRING      , "string"     },
        {V4L2_CTRL_TYPE_BITMASK     , "bitmask"    },
        {V4L2_CTRL_TYPE_INTEGER_MENU, "integerMenu"}
    };

    return ctrlTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2CtrlTypeMap, ctrlTypeToStr, (initV4l2CtrlTypeMap()))

using IoMethodMap = QMap<CaptureV4L2::IoMethod, QString>;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr = {
        {CaptureV4L2::IoMethodReadWrite  , "readWrite"  },
        {CaptureV4L2::IoMethodMemoryMap  , "memoryMap"  },
        {CaptureV4L2::IoMethodUserPointer, "userPointer"}
    };

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

using V4L2FmtToAkFmtMap = QMap<__u32, AkVideoCaps::PixelFormat>;

inline V4L2FmtToAkFmtMap initV4L2FmtToAkFmt()
{
    V4L2FmtToAkFmtMap v4l2FmtToAkFmt {
        // RGB formats (1 or 2 bytes per pixel)

        {V4L2_PIX_FMT_RGB332  , AkVideoCaps::Format_rgb332    },
        {V4L2_PIX_FMT_RGB444  , AkVideoCaps::Format_rgb444le  },
        {V4L2_PIX_FMT_ARGB444 , AkVideoCaps::Format_argb4444le},
        {V4L2_PIX_FMT_XRGB444 , AkVideoCaps::Format_rgb444le  },
        {V4L2_PIX_FMT_RGBA444 , AkVideoCaps::Format_rgba4444le},
        {V4L2_PIX_FMT_RGBX444 , AkVideoCaps::Format_rgbx444le },
        {V4L2_PIX_FMT_ABGR444 , AkVideoCaps::Format_abgr4444le},
        {V4L2_PIX_FMT_XBGR444 , AkVideoCaps::Format_xbgr444le },
        {V4L2_PIX_FMT_BGRA444 , AkVideoCaps::Format_bgra4444le},
        {V4L2_PIX_FMT_BGRX444 , AkVideoCaps::Format_bgrx444le },
        {V4L2_PIX_FMT_RGB555  , AkVideoCaps::Format_rgb555le  },
        {V4L2_PIX_FMT_ARGB555 , AkVideoCaps::Format_argb1555le},
        {V4L2_PIX_FMT_XRGB555 , AkVideoCaps::Format_rgb555le  },
        {V4L2_PIX_FMT_RGBA555 , AkVideoCaps::Format_rgba5551le},
        {V4L2_PIX_FMT_RGBX555 , AkVideoCaps::Format_rgbx555le },
        {V4L2_PIX_FMT_ABGR555 , AkVideoCaps::Format_abgr1555le},
        {V4L2_PIX_FMT_XBGR555 , AkVideoCaps::Format_bgr555le  },
        {V4L2_PIX_FMT_BGRA555 , AkVideoCaps::Format_bgra5551le},
        {V4L2_PIX_FMT_BGRX555 , AkVideoCaps::Format_bgrx555le },
        {V4L2_PIX_FMT_RGB565  , AkVideoCaps::Format_rgb565le  },
        {V4L2_PIX_FMT_RGB555X , AkVideoCaps::Format_rgb555be  },
        {V4L2_PIX_FMT_ARGB555X, AkVideoCaps::Format_argb1555be},
        {V4L2_PIX_FMT_XRGB555X, AkVideoCaps::Format_rgb555be  },
        {V4L2_PIX_FMT_RGB565X , AkVideoCaps::Format_rgb565be  },

        // RGB formats (3 or 4 bytes per pixel)

        {V4L2_PIX_FMT_BGR24 , AkVideoCaps::Format_bgr24},
        {V4L2_PIX_FMT_RGB24 , AkVideoCaps::Format_rgb24},
        {V4L2_PIX_FMT_BGR32 , AkVideoCaps::Format_xbgr },
        {V4L2_PIX_FMT_ABGR32, AkVideoCaps::Format_abgr },
        {V4L2_PIX_FMT_XBGR32, AkVideoCaps::Format_xbgr },
        {V4L2_PIX_FMT_BGRA32, AkVideoCaps::Format_bgra },
        {V4L2_PIX_FMT_BGRX32, AkVideoCaps::Format_bgrx },
        {V4L2_PIX_FMT_RGB32 , AkVideoCaps::Format_xrgb },
        {V4L2_PIX_FMT_RGBA32, AkVideoCaps::Format_rgba },
        {V4L2_PIX_FMT_RGBX32, AkVideoCaps::Format_rgbx },
        {V4L2_PIX_FMT_ARGB32, AkVideoCaps::Format_argb },
        {V4L2_PIX_FMT_XRGB32, AkVideoCaps::Format_xrgb },

        // Grey formats

        {V4L2_PIX_FMT_GREY  , AkVideoCaps::Format_y8   },
        {V4L2_PIX_FMT_Y4    , AkVideoCaps::Format_xy44 },
        {V4L2_PIX_FMT_Y6    , AkVideoCaps::Format_xy26 },
        {V4L2_PIX_FMT_Y10   , AkVideoCaps::Format_y10le},
        {V4L2_PIX_FMT_Y12   , AkVideoCaps::Format_y12le},
#ifdef V4L2_PIX_FMT_Y14
        {V4L2_PIX_FMT_Y14   , AkVideoCaps::Format_y14le},
#endif
        {V4L2_PIX_FMT_Y16   , AkVideoCaps::Format_y16le},
        {V4L2_PIX_FMT_Y16_BE, AkVideoCaps::Format_y16be},

        // Luminance+Chrominance formats

        {V4L2_PIX_FMT_YUYV  , AkVideoCaps::Format_yuyv422 },
        {V4L2_PIX_FMT_YVYU  , AkVideoCaps::Format_yvyu422 },
        {V4L2_PIX_FMT_UYVY  , AkVideoCaps::Format_uyvy422 },
        {V4L2_PIX_FMT_VYUY  , AkVideoCaps::Format_vyuy422 },
        {V4L2_PIX_FMT_Y41P  , AkVideoCaps::Format_yuv411p },
        {V4L2_PIX_FMT_YUV444, AkVideoCaps::Format_xyuv4444},
        {V4L2_PIX_FMT_YUV555, AkVideoCaps::Format_xyuv1555},
        {V4L2_PIX_FMT_YUV565, AkVideoCaps::Format_yuv565  },
#ifdef V4L2_PIX_FMT_YUV24
        {V4L2_PIX_FMT_YUV24 , AkVideoCaps::Format_yuv24   },
#endif
        {V4L2_PIX_FMT_YUV32 , AkVideoCaps::Format_xyuv    },
        {V4L2_PIX_FMT_AYUV32, AkVideoCaps::Format_ayuv    },
        {V4L2_PIX_FMT_XYUV32, AkVideoCaps::Format_xyuv    },
        {V4L2_PIX_FMT_VUYA32, AkVideoCaps::Format_vuya    },
        {V4L2_PIX_FMT_VUYX32, AkVideoCaps::Format_vuyx    },

        // two planes -- one Y, one Cr + Cb interleaved

        {V4L2_PIX_FMT_NV12, AkVideoCaps::Format_nv12},
        {V4L2_PIX_FMT_NV21, AkVideoCaps::Format_nv21},
        {V4L2_PIX_FMT_NV16, AkVideoCaps::Format_nv16},
        {V4L2_PIX_FMT_NV61, AkVideoCaps::Format_nv61},
        {V4L2_PIX_FMT_NV24, AkVideoCaps::Format_nv24},
        {V4L2_PIX_FMT_NV42, AkVideoCaps::Format_nv42},

        // two non contiguous planes - one Y, one Cr + Cb interleaved

        {V4L2_PIX_FMT_NV12M, AkVideoCaps::Format_nv12},
        {V4L2_PIX_FMT_NV21M, AkVideoCaps::Format_nv21},
        {V4L2_PIX_FMT_NV16M, AkVideoCaps::Format_nv16},
        {V4L2_PIX_FMT_NV61M, AkVideoCaps::Format_nv61},

        // three planes - Y Cb, Cr

        {V4L2_PIX_FMT_YUV410 , AkVideoCaps::Format_yuv410p},
        {V4L2_PIX_FMT_YVU410 , AkVideoCaps::Format_yvu410p},
        {V4L2_PIX_FMT_YUV411P, AkVideoCaps::Format_yuv411p},
        {V4L2_PIX_FMT_YUV420 , AkVideoCaps::Format_yuv420p},
        {V4L2_PIX_FMT_YVU420 , AkVideoCaps::Format_yvu420p},
        {V4L2_PIX_FMT_YUV422P, AkVideoCaps::Format_yuv422p},

        // three non contiguous planes - Y, Cb, Cr

        {V4L2_PIX_FMT_YUV420M, AkVideoCaps::Format_yuv420p},
        {V4L2_PIX_FMT_YVU420M, AkVideoCaps::Format_yvu420p},
        {V4L2_PIX_FMT_YUV422M, AkVideoCaps::Format_yuv422p},
        {V4L2_PIX_FMT_YVU422M, AkVideoCaps::Format_yvu422p},
        {V4L2_PIX_FMT_YUV444M, AkVideoCaps::Format_yuv444p},
        {V4L2_PIX_FMT_YVU444M, AkVideoCaps::Format_yvu444p},
    };

    return v4l2FmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4L2FmtToAkFmtMap, v4l2FmtToAkFmt, (initV4L2FmtToAkFmt()))

using CompressedFormatToStrMap = QMap<__u32, QString>;

inline CompressedFormatToStrMap initCompressedFormatToStr()
{
    CompressedFormatToStrMap compressedFormatToStr {
        {V4L2_PIX_FMT_MJPEG         , "mjpg" },
        {V4L2_PIX_FMT_JPEG          , "jpeg" },
        {V4L2_PIX_FMT_DV            , "dvsd" },
        {V4L2_PIX_FMT_MPEG          , "mpeg" },
        {V4L2_PIX_FMT_H264          , "h264" },
        {V4L2_PIX_FMT_H264_NO_SC    , "h264" },
        {V4L2_PIX_FMT_H264_MVC      , "h264" },
        {V4L2_PIX_FMT_H263          , "h263" },
        {V4L2_PIX_FMT_MPEG1         , "mpeg1"},
        {V4L2_PIX_FMT_MPEG2         , "mpeg2"},
        {V4L2_PIX_FMT_MPEG2_SLICE   , "mpeg2"},
        {V4L2_PIX_FMT_MPEG4         , "mpeg4"},
        {V4L2_PIX_FMT_XVID          , "xvid" },
        {V4L2_PIX_FMT_VC1_ANNEX_G   , "vc1"  },
        {V4L2_PIX_FMT_VC1_ANNEX_L   , "vc1"  },
        {V4L2_PIX_FMT_VP8           , "vp8"  },
#ifdef V4L2_PIX_FMT_VP8_FRAME
        {V4L2_PIX_FMT_VP8_FRAME     , "vp8"  },
#endif
        {V4L2_PIX_FMT_VP9           , "vp9"  },
#ifdef V4L2_PIX_FMT_VP9_FRAME
        {V4L2_PIX_FMT_VP9_FRAME     , "vp9"  },
#endif
        {V4L2_PIX_FMT_HEVC          , "hevc" },
#ifdef V4L2_PIX_FMT_H264_SLICE
        {V4L2_PIX_FMT_H264_SLICE    , "h264" },
#endif
    };

    return compressedFormatToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CompressedFormatToStrMap,
                          compressedFormatToStr,
                          (initCompressedFormatToStr()))

struct CaptureBuffer
{
    char *start[VIDEO_MAX_PLANES];
    size_t length[VIDEO_MAX_PLANES];
};

class DeviceV4L2Format
{
    public:
        AkCaps caps;
        __u32 type {0};
        __u32 pixelformat {0};

        DeviceV4L2Format(const AkCaps &caps,
                         __u32 type,
                         __u32 pixelformat):
            caps(caps),
            type(type),
            pixelformat(pixelformat)
        {

        }
};

using V4L2Formats = QVector<DeviceV4L2Format>;

class CaptureV4L2Private
{
    public:
        CaptureV4L2 *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, V4L2Formats> m_devicesCaps;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QFileSystemWatcher *m_fsWatcher {nullptr};
        AkVideoPacket m_outPacket;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};
        QVector<CaptureBuffer> m_buffers;
        v4l2_format m_v4l2Format;
        CaptureV4L2::IoMethod m_ioMethod {CaptureV4L2::IoMethodUnknown};
        int m_nBuffers {32};
        int m_fd {-1};

#ifdef HAVE_LIBUSB
        UvcExtendedControls m_extendedControls;
#endif

        explicit CaptureV4L2Private(CaptureV4L2 *self);
        ~CaptureV4L2Private();
        inline int planesCount(const v4l2_format &format) const;
        V4L2Formats capsFps(int fd,
                            const v4l2_fmtdesc &format,
                            __u32 width,
                            __u32 height) const;
        V4L2Formats caps(int fd) const;
        void setFps(int fd, __u32 bufferType, const AkFrac &fps);
        QVariantList controls(int fd, quint32 controlClass) const;
        bool setControls(int fd,
                         quint32 controlClass,
                         const QVariantMap &controls) const;
        QVariantList queryControl(int handle,
                                  quint32 controlClass,
                                  v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findControls(int handle,
                                            quint32 controlClass) const;
        bool initReadWrite(const v4l2_format &format);
        bool initMemoryMap(const v4l2_format &format);
        bool initUserPointer(const v4l2_format &format);
        bool startCapture(const v4l2_format &format);
        void stopCapture(const v4l2_format &format);
        QString fourccToStr(quint32 format) const;
        AkPacket processFrame(const char * const *planeData,
                              const ssize_t *planeSize,
                              qint64 pts);
        QVariantList imageControls(int fd) const;
        bool setImageControls(int fd,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(int fd) const;
        bool setCameraControls(int fd,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        inline QStringList v4l2Devices() const;
        void updateDevices();
};

CaptureV4L2::CaptureV4L2(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureV4L2Private(this);
}

CaptureV4L2::~CaptureV4L2()
{
    delete this->d;
}

QStringList CaptureV4L2::webcams() const
{
    return this->d->m_devices;
}

QString CaptureV4L2::device() const
{
    return this->d->m_device;
}

QList<int> CaptureV4L2::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CaptureV4L2::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureV4L2::ioMethod() const
{
    return ioMethodToStr->value(this->d->m_ioMethod, "any");
}

int CaptureV4L2::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CaptureV4L2::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CaptureV4L2::caps(const QString &webcam) const
{
    CaptureVideoCaps caps;

    for (auto &format: this->d->m_devicesCaps.value(webcam))
        caps << format.caps;

    return caps;
}

QVariantList CaptureV4L2::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureV4L2::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lockForRead();
    auto globalImageControls = this->d->m_globalImageControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        auto control = globalImageControls[i].toList();
        auto controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

    if (this->d->m_globalImageControls == globalImageControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalImageControls = globalImageControls;
    this->d->m_controlsMutex.unlock();

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool CaptureV4L2::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureV4L2::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureV4L2::setCameraControls(const QVariantMap &cameraControls)
{
    this->d->m_controlsMutex.lockForRead();
    auto globalCameraControls = this->d->m_globalCameraControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

    if (this->d->m_globalCameraControls == globalCameraControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalCameraControls = globalCameraControls;
    this->d->m_controlsMutex.unlock();
    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool CaptureV4L2::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureV4L2::readFrame()
{
    if (this->d->m_buffers.isEmpty())
        return {};

    if (this->d->m_fd < 0)
        return {};

    this->d->m_controlsMutex.lockForRead();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setImageControls(this->d->m_fd, controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lockForRead();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls) {
        auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                         cameraControls);
        this->d->setCameraControls(this->d->m_fd, controls);

#ifdef HAVE_LIBUSB
        this->d->m_extendedControls.setControls(this->d->m_fd, controls);
#endif

        this->d->m_localCameraControls = cameraControls;
    }

    int planesCount = this->d->planesCount(this->d->m_v4l2Format);
    ssize_t planeSize[planesCount];
    memset(planeSize, 0, planesCount * sizeof(ssize_t));

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        for (int i = 0; i < planesCount; i++) {
            planeSize[i] = x_read(this->d->m_fd,
                                  this->d->m_buffers[0].start[i],
                                  this->d->m_buffers[0].length[i]);

            if (planeSize[i] < 0)
                return AkPacket();
        }

        timeval timestamp {};
        gettimeofday(&timestamp, nullptr);

        auto pts = qint64((timestamp.tv_sec + 1e-6 * timestamp.tv_usec)
                          * this->d->m_fps.value());

        return this->d->processFrame(this->d->m_buffers[0].start,
                                     planeSize,
                                     pts);
    }

    if (this->d->m_ioMethod == IoMethodMemoryMap
        || this->d->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = this->d->m_v4l2Format.type;
        buffer.memory = (this->d->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (x_ioctl(this->d->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return AkPacket();

        if (buffer.index >= quint32(this->d->m_buffers.size()))
            return AkPacket();

        if (this->d->m_v4l2Format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            planeSize[0] = buffer.bytesused;
        else
            for (int i = 0; i < planesCount; i++)
                planeSize[i] = buffer.m.planes[i].bytesused;

        auto pts = qint64((buffer.timestamp.tv_sec
                           + 1e-6 * buffer.timestamp.tv_usec)
                          * this->d->m_fps.value());

        auto packet =
                this->d->processFrame(this->d->m_buffers[int(buffer.index)].start,
                                      planeSize,
                                      pts);

        if (x_ioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) < 0)
            return AkPacket();

        return packet;
    }

    return {};
}

bool CaptureV4L2::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

    // Frames read must be blocking so we does not waste CPU time.
    this->d->m_fd =
            x_open(this->d->m_device.toStdString().c_str(),
                   O_RDWR, // | O_NONBLOCK,
                   0);

    if (this->d->m_fd < 0) {
        qDebug() << "Can't open device:" << this->d->m_device;

        return false;
    }

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (x_ioctl(this->d->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        x_close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    auto streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";
        x_close(this->d->m_fd);

        return false;
    }

    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);
    auto caps = supportedCaps[streams[0]];
    auto v4l2PixelFormat = caps.pixelformat;
    int width = 0;
    int height = 0;
    AkFrac fps;

    if (caps.caps.type() == AkCaps::CapsVideo) {
        AkVideoCaps videoCaps(caps.caps);
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    } else {
        AkCompressedVideoCaps videoCaps(caps.caps);
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    }

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = caps.type;
    x_ioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);

    if (fmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        fmt.fmt.pix.pixelformat = v4l2PixelFormat;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
    } else {
        fmt.fmt.pix_mp.pixelformat = v4l2PixelFormat;
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
    }

    if (x_ioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << QString("VideoCapture: Can't set format: %1 %2x%3")
                    .arg(this->d->fourccToStr(v4l2PixelFormat))
                    .arg(width)
                    .arg(height);
        x_close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    memcpy(&this->d->m_v4l2Format, &fmt, sizeof(v4l2_format));
    this->d->m_fps = fps;
    this->d->setFps(this->d->m_fd, fmt.type, this->d->m_fps);
    this->d->m_caps = caps.caps;
    this->d->m_timeBase = this->d->m_fps.invert();

    if (this->d->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->d->initReadWrite(fmt)) {
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initMemoryMap(fmt)) {
    } else if (this->d->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initUserPointer(fmt)) {
    } else {
        this->d->m_ioMethod = IoMethodUnknown;
    }

    if (this->d->m_ioMethod != IoMethodUnknown) {
        if (!this->d->startCapture(fmt)) {
            qDebug() << "Start capture failed";

            return false;
        }

        if (this->d->m_caps.type() == AkCaps::CapsVideo) {
            this->d->m_outPacket = {this->d->m_caps};
            this->d->m_outPacket.setTimeBase(this->d->m_timeBase);
            this->d->m_outPacket.setIndex(0);
            this->d->m_outPacket.setId(this->d->m_id);
        }

        return true;
    }

    if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->d->initMemoryMap(fmt))
            this->d->m_ioMethod = IoMethodMemoryMap;
        else if (this->d->initUserPointer(fmt))
            this->d->m_ioMethod = IoMethodUserPointer;
    }

    if (this->d->m_ioMethod == IoMethodUnknown) {
        if (capabilities.capabilities & V4L2_CAP_READWRITE
            && this->d->initReadWrite(fmt))
            this->d->m_ioMethod = IoMethodReadWrite;
        else
            return false;
    }

    if (!this->d->startCapture(fmt)) {
        qDebug() << "Start capture failed";

        return false;
    }

    if (this->d->m_caps.type() == AkCaps::CapsVideo) {
        this->d->m_outPacket = {this->d->m_caps};
        this->d->m_outPacket.setTimeBase(this->d->m_timeBase);
        this->d->m_outPacket.setIndex(0);
        this->d->m_outPacket.setId(this->d->m_id);
    }

    return true;
}

void CaptureV4L2::uninit()
{
    this->d->stopCapture(this->d->m_v4l2Format);
    int planesCount = this->d->planesCount(this->d->m_v4l2Format);

    if (!this->d->m_buffers.isEmpty()) {
        if (this->d->m_ioMethod == IoMethodReadWrite) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    delete [] buffer.start[i];
        } else if (this->d->m_ioMethod == IoMethodMemoryMap) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    x_munmap(buffer.start[i], buffer.length[i]);
        } else if (this->d->m_ioMethod == IoMethodUserPointer) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    delete [] buffer.start[i];
        }
    }

    if (this->d->m_fd >= 0) {
        x_close(this->d->m_fd);
        this->d->m_fd = -1;
    }

    this->d->m_caps = {};
    this->d->m_fps = AkFrac();
    this->d->m_timeBase = AkFrac();
    this->d->m_buffers.clear();
    this->d->m_outPacket = AkVideoPacket();
}

void CaptureV4L2::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();
        int fd = x_open(device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd >= 0) {
            this->d->m_globalImageControls = this->d->imageControls(fd);
            this->d->m_globalCameraControls = this->d->cameraControls(fd);

#ifdef HAVE_LIBUSB
            this->d->m_extendedControls.load(fd);
            this->d->m_globalCameraControls += this->d->m_extendedControls.controls(fd);
#endif

            x_close(fd);
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForRead();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void CaptureV4L2::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureV4L2::setIoMethod(const QString &ioMethod)
{
    if (this->d->m_fd >= 0)
        return;

    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodUnknown);

    if (this->d->m_ioMethod == ioMethodEnum)
        return;

    this->d->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void CaptureV4L2::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureV4L2::resetDevice()
{
    this->setDevice("");
}

void CaptureV4L2::resetStreams()
{
    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureV4L2::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureV4L2::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureV4L2::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

CaptureV4L2Private::CaptureV4L2Private(CaptureV4L2 *self):
    self(self)
{
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, self);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this->self,
                     [this] () {
        this->updateDevices();
    });
    this->updateDevices();
}

CaptureV4L2Private::~CaptureV4L2Private()
{
    delete this->m_fsWatcher;
}

int CaptureV4L2Private::planesCount(const v4l2_format &format) const
{
    return format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE?
                1:
                format.fmt.pix_mp.num_planes;
}

V4L2Formats CaptureV4L2Private::capsFps(int fd,
                                        const struct v4l2_fmtdesc &format,
                                        __u32 width,
                                        __u32 height) const
{
    V4L2Formats caps;
    AkVideoCaps::PixelFormat fmt;
    QString fmtCompressed;
    bool isRaw = v4l2FmtToAkFmt->contains(format.pixelformat);

    if (isRaw)
        fmt = v4l2FmtToAkFmt->value(format.pixelformat);
    else
        fmtCompressed = compressedFormatToStr->value(format.pixelformat);

#ifdef VIDIOC_ENUM_FRAMEINTERVALS
    v4l2_frmivalenum frmival {};
    frmival.pixel_format = format.pixelformat;
    frmival.width = width;
    frmival.height = height;

    for (frmival.index = 0;
         x_ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0;
         frmival.index++) {
        if (!frmival.discrete.numerator
            || !frmival.discrete.denominator)
            continue;

        AkFrac fps;

        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
            fps = AkFrac(frmival.discrete.denominator,
                         frmival.discrete.numerator);
        else
            fps = AkFrac(frmival.stepwise.min.denominator,
                         frmival.stepwise.max.numerator);

        if (isRaw) {
            AkVideoCaps videoCaps(fmt, width, height, fps);
            caps << DeviceV4L2Format(videoCaps,
                                     format.type,
                                     format.pixelformat);
        } else {
            AkCompressedVideoCaps videoCaps(fmtCompressed,
                                            width,
                                            height,
                                            fps);
            caps << DeviceV4L2Format(videoCaps,
                                     format.type,
                                     format.pixelformat);
        }
    }

    if (caps.isEmpty()) {
#endif
        struct v4l2_streamparm params;
        memset(&params, 0, sizeof(v4l2_streamparm));
        params.type = format.type;

        if (x_ioctl(fd, VIDIOC_G_PARM, &params) >= 0) {
            AkFrac fps;

            if (params.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
                fps = AkFrac(params.parm.capture.timeperframe.denominator,
                             params.parm.capture.timeperframe.numerator);
            else
                fps = AkFrac(30, 1);

            if (isRaw) {
                AkVideoCaps videoCaps(fmt, width, height, fps);
                caps << DeviceV4L2Format(videoCaps,
                                         format.type,
                                         format.pixelformat);
            } else {
                AkCompressedVideoCaps videoCaps(fmtCompressed,
                                                width,
                                                height,
                                                fps);
                caps << DeviceV4L2Format(videoCaps,
                                         format.type,
                                         format.pixelformat);
            }
        }
#ifdef VIDIOC_ENUM_FRAMEINTERVALS
    }
#endif

    return caps;
}

V4L2Formats CaptureV4L2Private::caps(int fd) const
{
    V4L2Formats caps;
    static const v4l2_buf_type bufferTypes[] = {
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
        v4l2_buf_type(0)
    };
    v4l2_fmtdesc fmtdesc;

#ifdef VIDIOC_ENUM_FRAMESIZES
    for (int i = 0; bufferTypes[i]; i++) {
        // Enumerate all supported formats.
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.type = bufferTypes[i];

        for (fmtdesc.index = 0;
             x_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
             fmtdesc.index++) {
            if (v4l2FmtToAkFmt->contains(fmtdesc.pixelformat)) {
                if (fmtdesc.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                    auto specs = AkVideoCaps::formatSpecs(v4l2FmtToAkFmt->value(fmtdesc.pixelformat));

                    if (specs.planes() > 1) {
                        qDebug() << "Multiplanar format reported as single planar:"
                                 << v4l2FmtToAkFmt->value(fmtdesc.pixelformat);

                        continue;
                    }
                }
            } else if (!compressedFormatToStr->contains(fmtdesc.pixelformat)) {
                qDebug() << "Unknown pixel format:"
                         << fmtdesc.pixelformat
                         << "(" << fourccToStr(fmtdesc.pixelformat) << ")";

                continue;
            }

            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmtdesc.pixelformat;

            // Enumerate frame sizes.
            for (frmsize.index = 0;
                 x_ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0;
                 frmsize.index++) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    caps << this->capsFps(fd,
                                          fmtdesc,
                                          frmsize.discrete.width,
                                          frmsize.discrete.height);
                } else {
#if 0
                    for (uint height = frmsize.stepwise.min_height;
                         height < frmsize.stepwise.max_height;
                         height += frmsize.stepwise.step_height)
                        for (uint width = frmsize.stepwise.min_width;
                             width < frmsize.stepwise.max_width;
                             width += frmsize.stepwise.step_width) {
                            caps << this->capsFps(fd, fmtdesc, width, height);
                        }
#endif
                }
            }
        }
    }

    if (!caps.isEmpty())
        return caps;
#endif

    for (int i = 0; bufferTypes[i]; i++) {
        // If VIDIOC_ENUM_FRAMESIZES failed, try reading the current resolution.
        v4l2_format fmt;
        memset(&fmt, 0, sizeof(v4l2_format));
        fmt.type = bufferTypes[i];
        uint width = 0;
        uint height = 0;

        // Check if it has at least a default format.
        if (x_ioctl(fd, VIDIOC_G_FMT, &fmt) >= 0) {
            if (fmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                width = fmt.fmt.pix.width;
                height = fmt.fmt.pix.height;
            } else {
                width = fmt.fmt.pix_mp.width;
                height = fmt.fmt.pix_mp.height;
            }
        }

        if (width <= 0 || height <= 0)
            return {};

        // Enumerate all supported formats.
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.type = bufferTypes[i];

        for (fmtdesc.index = 0;
             x_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
             fmtdesc.index++) {
            if (v4l2FmtToAkFmt->contains(fmtdesc.pixelformat)) {
                qDebug() << "Unknown pixel format:"
                         << fmtdesc.pixelformat
                         << "(" << fourccToStr(fmtdesc.pixelformat) << ")";

                continue;
            }

            caps << this->capsFps(fd, fmtdesc, width, height);
        }
    }

    return caps;
}

void CaptureV4L2Private::setFps(int fd,
                                __u32 bufferType,
                                const AkFrac &fps)
{
    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(v4l2_streamparm));
    streamparm.type = bufferType;

    if (x_ioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0)
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            streamparm.parm.capture.timeperframe.numerator = __u32(fps.den());
            streamparm.parm.capture.timeperframe.denominator = __u32(fps.num());
            x_ioctl(fd, VIDIOC_S_PARM, &streamparm);
        }
}

QVariantList CaptureV4L2Private::controls(int fd, quint32 controlClass) const
{
    QVariantList controls;

    if (fd < 0)
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (x_ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        auto control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        queryctrl.id = id;

        if (x_ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            auto control = this->queryControl(fd, controlClass, &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
         x_ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0;
         queryctrl.id++) {
        auto control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    return controls;
}

bool CaptureV4L2Private::setControls(int fd,
                                     quint32 controlClass,
                                     const QVariantMap &controls) const
{
    if (fd < 0)
        return false;

    auto ctrl2id = this->findControls(fd, controlClass);

    for (auto it = controls.cbegin(); it != controls.cend(); it++) {
        if (!ctrl2id.contains(it.key()))
            continue;

        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = ctrl2id[it.key()];
        ctrl.value = it.value().toInt();
        x_ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    }

    return true;
}

QVariantList CaptureV4L2Private::queryControl(int handle,
                                              quint32 controlClass,
                                              v4l2_queryctrl *queryctrl) const
{
    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
        return {};

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != controlClass)
        return {};

    v4l2_ext_control ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(v4l2_ext_control));
    ext_ctrl.id = queryctrl->id;

    v4l2_ext_controls ctrls;
    memset(&ctrls, 0, sizeof(v4l2_ext_controls));
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(queryctrl->id);
    ctrls.count = 1;
    ctrls.controls = &ext_ctrl;

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != V4L2_CTRL_CLASS_USER &&
        queryctrl->id < V4L2_CID_PRIVATE_BASE) {
        if (x_ioctl(handle, VIDIOC_G_EXT_CTRLS, &ctrls))
            return {};
    } else {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = queryctrl->id;

        if (x_ioctl(handle, VIDIOC_G_CTRL, &ctrl))
            return QVariantList();

        ext_ctrl.value = ctrl.value;
    }

    v4l2_querymenu qmenu;
    memset(&qmenu, 0, sizeof(v4l2_querymenu));
    qmenu.id = queryctrl->id;
    QStringList menu;

    if (queryctrl->type == V4L2_CTRL_TYPE_MENU)
        for (int i = 0; i < queryctrl->maximum + 1; i++) {
            qmenu.index = __u32(i);

            if (x_ioctl(handle, VIDIOC_QUERYMENU, &qmenu))
                continue;

            menu << QString(reinterpret_cast<const char *>(qmenu.name));
        }

    auto type = static_cast<v4l2_ctrl_type>(queryctrl->type);

    return QVariantList {
        QString(reinterpret_cast<const char *>(queryctrl->name)),
        ctrlTypeToStr->value(type),
        queryctrl->minimum,
        queryctrl->maximum,
        queryctrl->step,
        queryctrl->default_value,
        ext_ctrl.value,
        menu
    };
}

QMap<QString, quint32> CaptureV4L2Private::findControls(int handle,
                                                        quint32 controlClass) const
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, quint32> controls;

    while (x_ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        qctrl.id = id;

        if (x_ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0
            && !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (x_ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

bool CaptureV4L2Private::initReadWrite(const v4l2_format &format)
{
    int planesCount = this->planesCount(format);
    this->m_buffers.resize(1);
    bool error = false;

    for (auto &buffer: this->m_buffers)
        for (int i = 0; i < planesCount; i++) {
            buffer.length[i] = format.fmt.pix.sizeimage;
            buffer.start[i] = new char[format.fmt.pix.sizeimage];

            if (!buffer.start[i]) {
                error = true;

                break;
            }

            memset(buffer.start[i], 0, buffer.length[i]);
        }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i])
                    delete [] buffer.start[i];

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::initMemoryMap(const v4l2_format &format)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = format.type;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (x_ioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    int planesCount = this->planesCount(format);

    if (planesCount < 1)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        v4l2_plane planes[planesCount];
        memset(planes, 0, planesCount * sizeof(v4l2_plane));

        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = format.type;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = __u32(i);

        if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buffer.length = planesCount;
            buffer.m.planes = planes;
        }

        if (x_ioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
            this->m_buffers[i].length[0] = buffer.length;
            this->m_buffers[i].start[0] =
                    reinterpret_cast<char *>(x_mmap(nullptr,
                                                    buffer.length,
                                                    PROT_READ | PROT_WRITE,
                                                    MAP_SHARED,
                                                    this->m_fd,
                                                    buffer.m.offset));

            if (this->m_buffers[i].start == MAP_FAILED) {
                error = true;

                break;
            }
        } else {
            for (int j = 0; j < planesCount; j++) {
                this->m_buffers[i].length[j] = buffer.m.planes[j].length;
                this->m_buffers[i].start[j] =
                        reinterpret_cast<char *>(x_mmap(nullptr,
                                                        buffer.m.planes[j].length,
                                                        PROT_READ | PROT_WRITE,
                                                        MAP_SHARED,
                                                        this->m_fd,
                                                        buffer.m.planes[j].m.mem_offset));

                if(this->m_buffers[i].start[j] == MAP_FAILED){
                    error = true;

                    break;
                }
            }

            if (error)
                break;
        }
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i] != MAP_FAILED)
                    x_munmap(buffer.start[i], buffer.length[i]);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::initUserPointer(const v4l2_format &format)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = format.type;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (x_ioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    int planesCount = this->planesCount(format);
    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
            this->m_buffers[i].length[0] = format.fmt.pix.sizeimage;
            this->m_buffers[i].start[0] = new char[format.fmt.pix.sizeimage];

            if (!this->m_buffers[i].start[0]) {
                error = true;

                break;
            }

            memset(this->m_buffers[i].start[0], 0, format.fmt.pix.sizeimage);
        } else {
            for (int j = 0; j < format.fmt.pix_mp.num_planes; j++) {
                auto imageSize = format.fmt.pix_mp.plane_fmt[i].sizeimage;
                this->m_buffers[i].length[i] = imageSize;
                this->m_buffers[i].start[i] = new char[imageSize];

                if (!this->m_buffers[i].start[i]) {
                    error = true;

                    break;
                }

                memset(this->m_buffers[i].start[i], 0, imageSize);
            }

            if (error)
                break;
        }
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i])
                    delete [] buffer.start[i];

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::startCapture(const v4l2_format &format)
{
    bool error = false;

    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = format.type;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (x_ioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        if (x_ioctl(this->m_fd, VIDIOC_STREAMON, &format.type) < 0)
            error = true;
    } else if (this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
        int planesCount = this->planesCount(format);

        if (planesCount > 0) {
            for (int i = 0; i < this->m_buffers.size(); i++) {
                v4l2_buffer buffer;
                memset(&buffer, 0, sizeof(v4l2_buffer));
                buffer.type = format.type;
                buffer.memory = V4L2_MEMORY_USERPTR;
                buffer.index = __u32(i);

                if (this->m_v4l2Format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                    buffer.m.userptr = ulong(this->m_buffers[i].start[0]);
                    buffer.length = __u32(this->m_buffers[i].length[0]);
                } else {
                    v4l2_plane planes[planesCount];
                    memset(planes, 0, planesCount * sizeof(v4l2_plane));
                    buffer.length = format.fmt.pix_mp.num_planes;
                    buffer.m.planes = planes;

                    for (int j = 0; j < buffer.length; j++) {
                        planes[j].m.userptr = ulong(this->m_buffers[i].start[j]);
                        planes[j].length = __u32(this->m_buffers[i].length[j]);
                    }
                }

                if (x_ioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                    error = true;
            }

            if (x_ioctl(this->m_fd, VIDIOC_STREAMON, &format.type) < 0)
                error = true;
        } else {
            error = true;
        }
    }

    if (error)
        self->uninit();

    this->m_id = Ak::id();

    return !error;
}

void CaptureV4L2Private::stopCapture(const v4l2_format &format)
{
    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap
        || this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
        x_ioctl(this->m_fd, VIDIOC_STREAMOFF, &format.type);
    }
}

QString CaptureV4L2Private::fourccToStr(quint32 format) const
{
    char fourcc[5];
    memcpy(fourcc, &format, sizeof(quint32));
    fourcc[4] = 0;

    return QString(fourcc);
}

AkPacket CaptureV4L2Private::processFrame(const char * const *planeData,
                                          const ssize_t *planeSize,
                                          qint64 pts)
{
    if (this->m_caps.type() == AkCaps::CapsVideoCompressed) {
        AkCompressedVideoPacket oPacket(this->m_caps, planeSize[0]);
        memcpy(oPacket.data(), planeData[0], planeSize[0]);
        oPacket.setPts(pts);
        oPacket.setTimeBase(this->m_timeBase);
        oPacket.setIndex(0);
        oPacket.setId(this->m_id);

        return oPacket;
    }

    if (this->m_outPacket) {
        this->m_outPacket.setPts(pts);

        if (this->m_v4l2Format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
            auto iData = planeData[0];
            auto iLineSize = this->m_v4l2Format.fmt.pix.bytesperline;
            auto oLineSize = this->m_outPacket.lineSize(0);
            auto lineSize = qMin<size_t>(iLineSize, oLineSize);

            for (int y = 0; y < this->m_v4l2Format.fmt.pix.height; ++y)
                memcpy(this->m_outPacket.line(0, y),
                       iData + y * iLineSize,
                       lineSize);
        } else {
            for (int plane = 0; plane < this->planesCount(this->m_v4l2Format); ++plane) {
                auto iData = planeData[plane];
                auto iLineSize = this->m_v4l2Format.fmt.pix_mp.plane_fmt[plane].bytesperline;
                auto oLineSize = this->m_outPacket.lineSize(plane);
                auto lineSize = qMin<size_t>(iLineSize, oLineSize);
                auto heightDiv = this->m_outPacket.heightDiv(plane);

                for (int y = 0; y < this->m_v4l2Format.fmt.pix_mp.height; ++y) {
                    int ys = y >> heightDiv;
                    memcpy(this->m_outPacket.line(plane, y),
                           iData + ys * iLineSize,
                           lineSize);
                }
            }
        }
    }

    return this->m_outPacket;
}

QVariantList CaptureV4L2Private::imageControls(int fd) const
{
    return this->controls(fd, V4L2_CTRL_CLASS_USER);
}

bool CaptureV4L2Private::setImageControls(int fd,
                                          const QVariantMap &imageControls) const
{
    return this->setControls(fd, V4L2_CTRL_CLASS_USER, imageControls);
}

QVariantList CaptureV4L2Private::cameraControls(int fd) const
{
#ifdef V4L2_CTRL_CLASS_CAMERA
    return this->controls(fd, V4L2_CTRL_CLASS_CAMERA);
#else
    Q_UNUSED(fd)

    return {};
#endif
}

bool CaptureV4L2Private::setCameraControls(int fd,
                                           const QVariantMap &cameraControls) const
{
#ifdef V4L2_CTRL_CLASS_CAMERA
    return this->setControls(fd, V4L2_CTRL_CLASS_CAMERA, cameraControls);
#else
    Q_UNUSED(fd)
    Q_UNUSED(cameraControls)

    return false;
#endif
}

QVariantMap CaptureV4L2Private::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureV4L2Private::mapDiff(const QVariantMap &map1,
                                        const QVariantMap &map2) const
{
    QVariantMap map;

    for (auto it = map2.cbegin(); it != map2.cend(); it++)
        if (!map1.contains(it.key())
            || map1[it.key()] != it.value()) {
            map[it.key()] = it.value();
        }

    return map;
}

QStringList CaptureV4L2Private::v4l2Devices() const
{
    QDir devicesDir("/dev");

    return devicesDir.entryList(QStringList() << "video*",
                                QDir::System
                                | QDir::Readable
                                | QDir::Writable
                                | QDir::NoSymLinks
                                | QDir::NoDotAndDotDot
                                | QDir::CaseSensitive,
                                QDir::Name);
}

void CaptureV4L2Private::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    QDir devicesDir("/dev");
    auto devicesFiles = this->v4l2Devices();

    for (auto &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = x_open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        auto caps = this->caps(fd);

        if (!caps.empty()) {
            v4l2_capability capability;
            memset(&capability, 0, sizeof(v4l2_capability));
            QString description;

            if (x_ioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0)
                description = reinterpret_cast<const char *>(capability.card);

            devices << fileName;
            descriptions[fileName] = description;
            devicesCaps[fileName] = caps;
        }

        x_close(fd);
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->removePaths(this->m_devices);

        this->m_devices = devices;
#ifndef Q_OS_BSD4
        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->addPaths(this->m_devices);
#endif
        emit self->webcamsChanged(this->m_devices);
    }
}

#include "moc_capturev4l2.cpp"
