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
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QDir>
#include <QFileSystemWatcher>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/version.h>
#include <linux/videodev2.h>

#ifdef HAVE_V4LUTILS
#include <libv4l2.h>

#define x_ioctl v4l2_ioctl
#define x_open v4l2_open
#define x_close v4l2_close
#define x_read v4l2_read
#define x_mmap v4l2_mmap
#define x_munmap v4l2_munmap
#else
#include <unistd.h>
#include <sys/ioctl.h>

#define x_ioctl ioctl
#define x_open open
#define x_close close
#define x_read read
#define x_mmap mmap
#define x_munmap munmap
#endif

#include "capturev4l2.h"
#include "capturebuffer.h"

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

using FourccToStrMap = QMap<__u32, QString>;

inline FourccToStrMap initFourccToStr()
{
    FourccToStrMap fourccToStr = {
        {V4L2_PIX_FMT_RGB332      , "RGB332"      },
        {V4L2_PIX_FMT_RGB444      , "RGB444"      },
        {V4L2_PIX_FMT_ARGB444     , "ARGB444"     },
        {V4L2_PIX_FMT_XRGB444     , "XRGB444"     },
        {V4L2_PIX_FMT_RGB555      , "RGB555"      },
        {V4L2_PIX_FMT_ARGB555     , "ARGB555"     },
        {V4L2_PIX_FMT_XRGB555     , "XRGB555"     },
        {V4L2_PIX_FMT_RGB565      , "RGB565"      },
        {V4L2_PIX_FMT_RGB555X     , "RGB555BE"    },
        {V4L2_PIX_FMT_ARGB555X    , "ARGB555BE"   },
        {V4L2_PIX_FMT_XRGB555X    , "XRGB555BE"   },
        {V4L2_PIX_FMT_RGB565X     , "RGB565BE"    },
        {V4L2_PIX_FMT_BGR666      , "BGR666"      },
        {V4L2_PIX_FMT_BGR24       , "BGR"         },
        {V4L2_PIX_FMT_RGB24       , "RGB"         },
        {V4L2_PIX_FMT_BGR32       , "BGRX"        },
        {V4L2_PIX_FMT_ABGR32      , "ABGR"        },
        {V4L2_PIX_FMT_XBGR32      , "XBGR"        },
        {V4L2_PIX_FMT_RGB32       , "RGBA"        },
        {V4L2_PIX_FMT_ARGB32      , "ARGB"        },
        {V4L2_PIX_FMT_XRGB32      , "XRGB"        },
        {V4L2_PIX_FMT_GREY        , "GRAY8"       },
        {V4L2_PIX_FMT_Y4          , "GRAY4"       },
        {V4L2_PIX_FMT_Y6          , "GRAY6"       },
        {V4L2_PIX_FMT_Y10         , "GRAY10"      },
        {V4L2_PIX_FMT_Y12         , "GRAY12"      },
        {V4L2_PIX_FMT_Y16         , "GRAY16"      },
        {V4L2_PIX_FMT_Y16_BE      , "GRAY16BE"    },
        {V4L2_PIX_FMT_SBGGR8      , "SBGGR8"      },
        {V4L2_PIX_FMT_SGBRG8      , "SGBRG8"      },
        {V4L2_PIX_FMT_SGRBG8      , "SGRBG8"      },
        {V4L2_PIX_FMT_SRGGB8      , "SRGGB8"      },
        {V4L2_PIX_FMT_SBGGR10     , "SBGGR10"     },
        {V4L2_PIX_FMT_SGBRG10     , "SGBRG10"     },
        {V4L2_PIX_FMT_SGRBG10     , "SGRBG10"     },
        {V4L2_PIX_FMT_SRGGB10     , "SRGGB10"     },
        {V4L2_PIX_FMT_SBGGR10P    , "SBGGR10P"    },
        {V4L2_PIX_FMT_SGBRG10P    , "SGBRG10P"    },
        {V4L2_PIX_FMT_SGRBG10P    , "SGRBG10P"    },
        {V4L2_PIX_FMT_SRGGB10P    , "SRGGB10P"    },
        {V4L2_PIX_FMT_SBGGR10ALAW8, "SBGGR10ALAW8"},
        {V4L2_PIX_FMT_SGBRG10ALAW8, "SGBRG10ALAW8"},
        {V4L2_PIX_FMT_SGRBG10ALAW8, "SGRBG10ALAW8"},
        {V4L2_PIX_FMT_SRGGB10ALAW8, "SRGGB10ALAW8"},
        {V4L2_PIX_FMT_SBGGR10DPCM8, "SBGGR10DPCM8"},
        {V4L2_PIX_FMT_SGBRG10DPCM8, "SGBRG10DPCM8"},
        {V4L2_PIX_FMT_SGRBG10DPCM8, "SGRBG10DPCM8"},
        {V4L2_PIX_FMT_SRGGB10DPCM8, "SRGGB10DPCM8"},
        {V4L2_PIX_FMT_SBGGR12     , "SBGGR12"     },
        {V4L2_PIX_FMT_SGBRG12     , "SGBRG12"     },
        {V4L2_PIX_FMT_SGRBG12     , "SGRBG12"     },
        {V4L2_PIX_FMT_SRGGB12     , "SRGGB12"     },
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
        {V4L2_PIX_FMT_SBGGR12P    , "SBGGR12P"    },
        {V4L2_PIX_FMT_SGBRG12P    , "SGBRG12P"    },
        {V4L2_PIX_FMT_SGRBG12P    , "SGRBG12P"    },
        {V4L2_PIX_FMT_SRGGB12P    , "SRGGB12P"    },
        {V4L2_PIX_FMT_SBGGR16     , "SBGGR16"     },
        {V4L2_PIX_FMT_SGBRG16     , "SGBRG16"     },
        {V4L2_PIX_FMT_SGRBG16     , "SGRBG16"     },
        {V4L2_PIX_FMT_SRGGB16     , "SRGGB16"     },
#endif
    };

    return fourccToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(FourccToStrMap, v4l2FourccToStr, (initFourccToStr()))

class CaptureV4L2Private
{
    public:
        CaptureV4L2 *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMutex m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QFileSystemWatcher *m_fsWatcher {nullptr};
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};
        QVector<CaptureBuffer> m_buffers;
        CaptureV4L2::IoMethod m_ioMethod {CaptureV4L2::IoMethodUnknown};
        int m_nBuffers {32};
        int m_fd {-1};

        explicit CaptureV4L2Private(CaptureV4L2 *self);
        ~CaptureV4L2Private();
        QVariantList capsFps(int fd,
                             const v4l2_fmtdesc &format,
                             __u32 width,
                             __u32 height) const;
        QVariantList caps(int fd) const;
        void setFps(int fd, const AkFrac &fps);
        QVariantList controls(int fd, quint32 controlClass) const;
        bool setControls(int fd,
                         quint32 controlClass,
                         const QVariantMap &controls) const;
        QVariantList queryControl(int handle,
                                  quint32 controlClass,
                                  v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findControls(int handle,
                                            quint32 controlClass) const;
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        bool startCapture();
        void stopCapture();
        QString fourccToStr(quint32 format) const;
        quint32 strToFourCC(const QString &format) const;
        AkPacket processFrame(const char *buffer,
                              size_t bufferSize,
                              qint64 pts) const;
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

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CaptureV4L2::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->d->m_device);
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

QVariantList CaptureV4L2::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QString CaptureV4L2::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString(),
                     caps.property("width").toString(),
                     caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureV4L2::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureV4L2::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lock();
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

    this->d->m_controlsMutex.lock();

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
    this->d->m_controlsMutex.lock();
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

    this->d->m_controlsMutex.lock();

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
        return AkPacket();

    if (this->d->m_fd < 0)
        return AkPacket();

    this->d->m_controlsMutex.lock();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setImageControls(this->d->m_fd, controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lock();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls) {
        auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                         cameraControls);
        this->d->setCameraControls(this->d->m_fd, controls);
        this->d->m_localCameraControls = cameraControls;
    }

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        if (x_read(this->d->m_fd,
                   this->d->m_buffers[0].start,
                   this->d->m_buffers[0].length) < 0)
            return AkPacket();

        timeval timestamp {};
        gettimeofday(&timestamp, nullptr);

        auto pts = qint64((timestamp.tv_sec + 1e-6 * timestamp.tv_usec)
                          * this->d->m_fps.value());

        return this->d->processFrame(this->d->m_buffers[0].start,
                                     this->d->m_buffers[0].length,
                                     pts);
    }

    if (this->d->m_ioMethod == IoMethodMemoryMap
        || this->d->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer {};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = (this->d->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (x_ioctl(this->d->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return AkPacket();

        if (buffer.index >= quint32(this->d->m_buffers.size()))
            return AkPacket();

        auto pts = qint64((buffer.timestamp.tv_sec
                           + 1e-6 * buffer.timestamp.tv_usec)
                          * this->d->m_fps.value());

        AkPacket packet =
                this->d->processFrame(this->d->m_buffers[int(buffer.index)].start,
                                      buffer.bytesused,
                                      pts);

        if (x_ioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) < 0)
            return AkPacket();

        return packet;
    }

    return AkPacket();
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

    if (this->d->m_fd < 0)
        return false;

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (x_ioctl(this->d->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        x_close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";
        x_close(this->d->m_fd);

        return false;
    }

    QVariantList supportedCaps = this->caps(this->d->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    v4l2_format fmt {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    x_ioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);
    auto fourcc = caps.property("fourcc").toString();
    fmt.fmt.pix.pixelformat =
            v4l2FourccToStr->key(fourcc,
                                 this->d->strToFourCC(fourcc));
    fmt.fmt.pix.width = caps.property("width").toUInt();
    fmt.fmt.pix.height = caps.property("height").toUInt();

    if (x_ioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << QString("VideoCapture: Can't set format: %1 %2x%3")
                    .arg(this->d->fourccToStr(fmt.fmt.pix.pixelformat))
                    .arg(fmt.fmt.pix.width)
                    .arg(fmt.fmt.pix.height);
        x_close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    this->d->setFps(this->d->m_fd, caps.property("fps").toString());
    this->d->m_caps = caps;
    this->d->m_fps = caps.property("fps").toString();
    this->d->m_timeBase = this->d->m_fps.invert();

    if (this->d->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->d->initReadWrite(fmt.fmt.pix.sizeimage)) {
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initMemoryMap()) {
    } else if (this->d->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initUserPointer(fmt.fmt.pix.sizeimage)) {
    } else
        this->d->m_ioMethod = IoMethodUnknown;

    if (this->d->m_ioMethod != IoMethodUnknown)
        return this->d->startCapture();

    if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->d->initMemoryMap())
            this->d->m_ioMethod = IoMethodMemoryMap;
        else if (this->d->initUserPointer(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodUserPointer;
    }

    if (this->d->m_ioMethod == IoMethodUnknown) {
        if (capabilities.capabilities & V4L2_CAP_READWRITE
            && this->d->initReadWrite(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodReadWrite;
        else
            return false;
    }

    return this->d->startCapture();
}

void CaptureV4L2::uninit()
{
    this->d->stopCapture();

    if (!this->d->m_buffers.isEmpty()) {
        if (this->d->m_ioMethod == IoMethodReadWrite)
            delete [] this->d->m_buffers[0].start;
        else if (this->d->m_ioMethod == IoMethodMemoryMap)
            for (auto &buffer: this->d->m_buffers)
                x_munmap(buffer.start, buffer.length);
        else if (this->d->m_ioMethod == IoMethodUserPointer)
            for (auto &buffer: this->d->m_buffers)
                delete [] buffer.start;
    }

    x_close(this->d->m_fd);
    this->d->m_caps.clear();
    this->d->m_fps = AkFrac();
    this->d->m_timeBase = AkFrac();
    this->d->m_buffers.clear();
}

void CaptureV4L2::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lock();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();
        int fd = x_open(device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd >= 0) {
            this->d->m_globalImageControls = this->d->imageControls(fd);
            this->d->m_globalCameraControls = this->d->cameraControls(fd);
            x_close(fd);
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
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

    auto supportedCaps = this->caps(this->d->m_device);

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
    QVariantList supportedCaps = this->caps(this->d->m_device);
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

QVariantList CaptureV4L2Private::capsFps(int fd,
                                         const struct v4l2_fmtdesc &format,
                                         __u32 width,
                                         __u32 height) const
{
    QVariantList caps;
    auto fourcc =
            v4l2FourccToStr->value(format.pixelformat,
                                   this->fourccToStr(format.pixelformat));

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

        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", fourcc);
        videoCaps.setProperty("width", width);
        videoCaps.setProperty("height", height);
        videoCaps.setProperty("fps", fps.toString());
        caps << QVariant::fromValue(videoCaps);
    }

    if (caps.isEmpty()) {
#endif
        struct v4l2_streamparm params;
        memset(&params, 0, sizeof(v4l2_streamparm));
        params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (x_ioctl(fd, VIDIOC_G_PARM, &params) >= 0) {
            AkFrac fps;

            if (params.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
                fps = AkFrac(params.parm.capture.timeperframe.denominator,
                             params.parm.capture.timeperframe.numerator);
            else
                fps = AkFrac(30, 1);

            AkCaps videoCaps;
            videoCaps.setMimeType("video/unknown");
            videoCaps.setProperty("fourcc", fourcc);
            videoCaps.setProperty("width", width);
            videoCaps.setProperty("height", height);
            videoCaps.setProperty("fps", fps.toString());
            caps << QVariant::fromValue(videoCaps);
        }
#ifdef VIDIOC_ENUM_FRAMEINTERVALS
    }
#endif

    return caps;
}

QVariantList CaptureV4L2Private::caps(int fd) const
{
    QVariantList caps;

#ifdef VIDIOC_ENUM_FRAMESIZES
    // Enumerate all supported formats.
    v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (fmtdesc.index = 0;
         x_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
         fmtdesc.index++) {
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

    if (!caps.isEmpty())
        return caps;
#endif

    // If VIDIOC_ENUM_FRAMESIZES failed, try reading the current resolution.
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    uint width = 0;
    uint height = 0;

    // Check if it has at least a default format.
    if (x_ioctl(fd, VIDIOC_G_FMT, &fmt) >= 0) {
        width = fmt.fmt.pix.width;
        height = fmt.fmt.pix.height;
    }

    if (width <= 0 || height <= 0)
        return {};

    // Enumerate all supported formats.
    memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (fmtdesc.index = 0;
         x_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
         fmtdesc.index++) {
        caps << this->capsFps(fd, fmtdesc, width, height);
    }

    return caps;
}

void CaptureV4L2Private::setFps(int fd, const AkFrac &fps)
{
    v4l2_streamparm streamparm {};
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

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

bool CaptureV4L2Private::initReadWrite(quint32 bufferSize)
{
    this->m_buffers.resize(1);
    this->m_buffers[0].length = bufferSize;
    this->m_buffers[0].start = new char[bufferSize];

    if (!this->m_buffers[0].start) {
        this->m_buffers.clear();

        return false;
    }

    memset(this->m_buffers[0].start, 0, bufferSize);

    return true;
}

bool CaptureV4L2Private::initMemoryMap()
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (x_ioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = __u32(i);

        if (x_ioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        this->m_buffers[i].length = buffer.length;
        this->m_buffers[i].start =
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
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            x_munmap(buffer.start, buffer.length);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::initUserPointer(quint32 bufferSize)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (x_ioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        this->m_buffers[i].length = bufferSize;
        this->m_buffers[i].start = new char[bufferSize];

        if (!this->m_buffers[i].start) {
            error = true;

            break;
        }

        memset(this->m_buffers[i].start, 0, bufferSize);
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            delete [] buffer.start;

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::startCapture()
{
    bool error = false;

    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (x_ioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (x_ioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    } else if (this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;
            buffer.index = __u32(i);
            buffer.m.userptr = ulong(this->m_buffers[i].start);
            buffer.length = __u32(this->m_buffers[i].length);

            if (x_ioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (x_ioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    }

    if (error)
        self->uninit();

    this->m_id = Ak::id();

    return !error;
}

void CaptureV4L2Private::stopCapture()
{
    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap
        || this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        x_ioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

QString CaptureV4L2Private::fourccToStr(quint32 format) const
{
    char fourcc[5];
    memcpy(fourcc, &format, sizeof(quint32));
    fourcc[4] = 0;

    return QString(fourcc);
}

quint32 CaptureV4L2Private::strToFourCC(const QString &format) const
{
    quint32 fourcc;
    memcpy(&fourcc, format.toStdString().c_str(), sizeof(quint32));

    return fourcc;
}

AkPacket CaptureV4L2Private::processFrame(const char *buffer,
                                          size_t bufferSize,
                                          qint64 pts) const
{
    AkPacket oPacket(this->m_caps);
    oPacket.setBuffer({buffer, int(bufferSize)});
    oPacket.setPts(pts);
    oPacket.setTimeBase(this->m_timeBase);
    oPacket.setIndex(0);
    oPacket.setId(this->m_id);

    return oPacket;
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
