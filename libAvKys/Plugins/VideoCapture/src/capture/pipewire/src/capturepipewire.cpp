/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
#include <QMap>
#include <QVariant>
#include <QVector>
#include <QtConcurrent>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>
#include <spa/utils/result.h>

#include "capturepipewire.h"

using SpaFmtToAkFmtMap = QMap<spa_video_format, AkVideoCaps::PixelFormat>;

inline SpaFmtToAkFmtMap initSpaFmtToAkFmt()
{
    SpaFmtToAkFmtMap spaFmtToAkFmt {
        {SPA_VIDEO_FORMAT_UNKNOWN   , AkVideoCaps::Format_none         },
        {SPA_VIDEO_FORMAT_I420      , AkVideoCaps::Format_yuv420p      },
        {SPA_VIDEO_FORMAT_YV12      , AkVideoCaps::Format_yvu420p      },
        {SPA_VIDEO_FORMAT_YUY2      , AkVideoCaps::Format_yuyv422      },
        {SPA_VIDEO_FORMAT_UYVY      , AkVideoCaps::Format_uyvy422      },
        {SPA_VIDEO_FORMAT_AYUV      , AkVideoCaps::Format_ayuv         },
        {SPA_VIDEO_FORMAT_RGBx      , AkVideoCaps::Format_rgbx         },
        {SPA_VIDEO_FORMAT_BGRx      , AkVideoCaps::Format_bgrx         },
        {SPA_VIDEO_FORMAT_xRGB      , AkVideoCaps::Format_xrgb         },
        {SPA_VIDEO_FORMAT_xBGR      , AkVideoCaps::Format_xbgr         },
        {SPA_VIDEO_FORMAT_RGBA      , AkVideoCaps::Format_rgba         },
        {SPA_VIDEO_FORMAT_BGRA      , AkVideoCaps::Format_bgra         },
        {SPA_VIDEO_FORMAT_ARGB      , AkVideoCaps::Format_argb         },
        {SPA_VIDEO_FORMAT_ABGR      , AkVideoCaps::Format_abgr         },
        {SPA_VIDEO_FORMAT_RGB       , AkVideoCaps::Format_rgb24        },
        {SPA_VIDEO_FORMAT_BGR       , AkVideoCaps::Format_bgr24        },
        {SPA_VIDEO_FORMAT_Y41B      , AkVideoCaps::Format_yuv411p      },
        {SPA_VIDEO_FORMAT_Y42B      , AkVideoCaps::Format_yuv422p      },
        {SPA_VIDEO_FORMAT_YVYU      , AkVideoCaps::Format_yvyu422      },
        {SPA_VIDEO_FORMAT_Y444      , AkVideoCaps::Format_yuv444p      },
        {SPA_VIDEO_FORMAT_v210      , AkVideoCaps::Format_v210         },
        {SPA_VIDEO_FORMAT_v216      , AkVideoCaps::Format_v216         },
        {SPA_VIDEO_FORMAT_NV12      , AkVideoCaps::Format_nv12         },
        {SPA_VIDEO_FORMAT_NV21      , AkVideoCaps::Format_nv21         },
        {SPA_VIDEO_FORMAT_GRAY8     , AkVideoCaps::Format_y8           },
        {SPA_VIDEO_FORMAT_GRAY16_BE , AkVideoCaps::Format_y16be        },
        {SPA_VIDEO_FORMAT_GRAY16_LE , AkVideoCaps::Format_y16le        },
        {SPA_VIDEO_FORMAT_v308      , AkVideoCaps::Format_v308         },
        {SPA_VIDEO_FORMAT_RGB16     , AkVideoCaps::Format_rgb565       },
        {SPA_VIDEO_FORMAT_BGR16     , AkVideoCaps::Format_bgr565       },
        {SPA_VIDEO_FORMAT_RGB15     , AkVideoCaps::Format_rgb555       },
        {SPA_VIDEO_FORMAT_BGR15     , AkVideoCaps::Format_bgr555       },
        {SPA_VIDEO_FORMAT_UYVP      , AkVideoCaps::Format_v210         },
        {SPA_VIDEO_FORMAT_A420      , AkVideoCaps::Format_yuva420p     },
        {SPA_VIDEO_FORMAT_YUV9      , AkVideoCaps::Format_yuv410p      },
        {SPA_VIDEO_FORMAT_YVU9      , AkVideoCaps::Format_yvu410p      },
        {SPA_VIDEO_FORMAT_ARGB64    , AkVideoCaps::Format_argb64       },
        {SPA_VIDEO_FORMAT_AYUV64    , AkVideoCaps::Format_ayuv64       },
        {SPA_VIDEO_FORMAT_r210      , AkVideoCaps::Format_xrgb2101010  },
        {SPA_VIDEO_FORMAT_I420_10BE , AkVideoCaps::Format_yuv420p10be  },
        {SPA_VIDEO_FORMAT_I420_10LE , AkVideoCaps::Format_yuv420p10le  },
        {SPA_VIDEO_FORMAT_I422_10BE , AkVideoCaps::Format_yuv422p10be  },
        {SPA_VIDEO_FORMAT_I422_10LE , AkVideoCaps::Format_yuv422p10le  },
        {SPA_VIDEO_FORMAT_Y444_10BE , AkVideoCaps::Format_yuv444p10be  },
        {SPA_VIDEO_FORMAT_Y444_10LE , AkVideoCaps::Format_yuv444p10le  },
        {SPA_VIDEO_FORMAT_GBR       , AkVideoCaps::Format_gbr24p       },
        {SPA_VIDEO_FORMAT_GBR_10BE  , AkVideoCaps::Format_gbrp10be     },
        {SPA_VIDEO_FORMAT_GBR_10LE  , AkVideoCaps::Format_gbrp10le     },
        {SPA_VIDEO_FORMAT_NV16      , AkVideoCaps::Format_nv16         },
        {SPA_VIDEO_FORMAT_NV24      , AkVideoCaps::Format_nv24         },
        {SPA_VIDEO_FORMAT_A420_10BE , AkVideoCaps::Format_yuva420p10be },
        {SPA_VIDEO_FORMAT_A420_10LE , AkVideoCaps::Format_yuva420p10le },
        {SPA_VIDEO_FORMAT_A422_10BE , AkVideoCaps::Format_yuva422p10be },
        {SPA_VIDEO_FORMAT_A422_10LE , AkVideoCaps::Format_yuva422p10le },
        {SPA_VIDEO_FORMAT_A444_10BE , AkVideoCaps::Format_yuva444p10be },
        {SPA_VIDEO_FORMAT_A444_10LE , AkVideoCaps::Format_yuva444p10le },
        {SPA_VIDEO_FORMAT_NV61      , AkVideoCaps::Format_nv61         },
        {SPA_VIDEO_FORMAT_P010_10BE , AkVideoCaps::Format_p010be       },
        {SPA_VIDEO_FORMAT_P010_10LE , AkVideoCaps::Format_p010le       },
        {SPA_VIDEO_FORMAT_VYUY      , AkVideoCaps::Format_vyuy422      },
        {SPA_VIDEO_FORMAT_GBRA      , AkVideoCaps::Format_gbrap        },
        {SPA_VIDEO_FORMAT_GBRA_10BE , AkVideoCaps::Format_gbrap10be    },
        {SPA_VIDEO_FORMAT_GBRA_10LE , AkVideoCaps::Format_gbrap10le    },
        {SPA_VIDEO_FORMAT_GBR_12BE  , AkVideoCaps::Format_gbrp12be     },
        {SPA_VIDEO_FORMAT_GBR_12LE  , AkVideoCaps::Format_gbrp12le     },
        {SPA_VIDEO_FORMAT_GBRA_12BE , AkVideoCaps::Format_gbrap12be    },
        {SPA_VIDEO_FORMAT_GBRA_12LE , AkVideoCaps::Format_gbrap12le    },
        {SPA_VIDEO_FORMAT_I420_12BE , AkVideoCaps::Format_yuv420p12be  },
        {SPA_VIDEO_FORMAT_I420_12LE , AkVideoCaps::Format_yuv420p12le  },
        {SPA_VIDEO_FORMAT_I422_12BE , AkVideoCaps::Format_yuv422p12be  },
        {SPA_VIDEO_FORMAT_I422_12LE , AkVideoCaps::Format_yuv422p12le  },
        {SPA_VIDEO_FORMAT_Y444_12BE , AkVideoCaps::Format_yuv444p12be  },
        {SPA_VIDEO_FORMAT_Y444_12LE , AkVideoCaps::Format_yuv444p12le  },
#if PW_CHECK_VERSION(0, 3, 41)
        {SPA_VIDEO_FORMAT_xRGB_210LE, AkVideoCaps::Format_xrgb2101010le},
        {SPA_VIDEO_FORMAT_xBGR_210LE, AkVideoCaps::Format_xbgr2101010le},
        {SPA_VIDEO_FORMAT_RGBx_102LE, AkVideoCaps::Format_rgbx1010102le},
        {SPA_VIDEO_FORMAT_BGRx_102LE, AkVideoCaps::Format_bgrx1010102le},
        {SPA_VIDEO_FORMAT_ARGB_210LE, AkVideoCaps::Format_argb2101010le},
        {SPA_VIDEO_FORMAT_ABGR_210LE, AkVideoCaps::Format_abgr2101010le},
        {SPA_VIDEO_FORMAT_RGBA_102LE, AkVideoCaps::Format_rgba1010102le},
        {SPA_VIDEO_FORMAT_BGRA_102LE, AkVideoCaps::Format_bgra1010102le},
#endif
    };

    return spaFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(SpaFmtToAkFmtMap,
                          spaFmtToAkFmt,
                          (initSpaFmtToAkFmt()))

using SpaCompressedToStrMap = QMap<spa_media_subtype, AkCompressedVideoCaps::VideoCodecID>;

inline SpaCompressedToStrMap initSpaCompressedToStrMap()
{
    SpaCompressedToStrMap spaCompressedToStrMap {
        {SPA_MEDIA_SUBTYPE_h264   , AkCompressedVideoCaps::VideoCodecID_h264   },
        {SPA_MEDIA_SUBTYPE_mjpg   , AkCompressedVideoCaps::VideoCodecID_mjpeg  },
        {SPA_MEDIA_SUBTYPE_mpeg1  , AkCompressedVideoCaps::VideoCodecID_mpeg1  },
        {SPA_MEDIA_SUBTYPE_mpeg2  , AkCompressedVideoCaps::VideoCodecID_mpeg2  },
        {SPA_MEDIA_SUBTYPE_mpeg4  , AkCompressedVideoCaps::VideoCodecID_mpeg4p2},
        {SPA_MEDIA_SUBTYPE_xvid   , AkCompressedVideoCaps::VideoCodecID_mpeg4p2},
        {SPA_MEDIA_SUBTYPE_vp8    , AkCompressedVideoCaps::VideoCodecID_vp8    },
        {SPA_MEDIA_SUBTYPE_vp9    , AkCompressedVideoCaps::VideoCodecID_vp9    },
        {SPA_MEDIA_SUBTYPE_jpeg   , AkCompressedVideoCaps::VideoCodecID_mjpeg  },
    };

    return spaCompressedToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(SpaCompressedToStrMap,
                          spaCompressedToStrMap,
                          (initSpaCompressedToStrMap()))

struct DeviceControl
{
    uint32_t id;
    QString description;
    QString type;
    qreal min;
    qreal max;
    qreal step;
    qreal def;
    qreal value;
    QStringList menu;
};

using DeviceControls = QVector<DeviceControl>;

struct SequenceParam
{
    uint32_t nodeId;
    uint32_t paramId;
};

using ResolutionRange = QPair<int, int>;
using FrameRateRange = QPair<AkFrac, AkFrac>;
using SpaSubTypesList = QVector<spa_media_subtype>;
using SpaFormatsList = QVector<spa_video_format>;

class DeviceSpaFormat
{
    public:
    AkCaps caps;
    spa_media_subtype subType {SPA_MEDIA_SUBTYPE_unknown};
    spa_video_format format {SPA_VIDEO_FORMAT_UNKNOWN};

    DeviceSpaFormat(const AkCaps &caps,
                    spa_media_subtype subType,
                    spa_video_format format):
          caps(caps),
          subType(subType),
          format(format)
    {

    }
};

using SpaFormats = QVector<DeviceSpaFormat>;

class CapturePipeWirePrivate
{
    public:
        CapturePipeWire *self;
        QString m_curDevice;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, SpaFormats> m_devicesCaps;
        QMap<QString, SpaFormatsList> m_rawFormats;
        QMap<QString, SpaSubTypesList> m_encodedFormats;
        QMap<QString, ResolutionRange> m_widthRange;
        QMap<QString, ResolutionRange> m_heightRange;
        QMap<QString, FrameRateRange> m_frameRateRange;
        QMap<QString, DeviceControls> m_devicesControls;
        QMap<uint32_t, QString> m_deviceIds;
        QMap<uint32_t, pw_node *> m_deviceNodes;
        QMap<int32_t, SequenceParam> m_sequenceParams;
        QMap<QString, spa_hook> m_nodeHooks;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QReadWriteLock m_mutex;
        AkPacket m_curPacket;
        QWaitCondition m_waitCondition;
        pw_main_loop *m_pwDevicesLoop {nullptr};
        pw_thread_loop *m_pwStreamLoop {nullptr};
        pw_context *m_pwStreamContext {nullptr};
        pw_core *m_pwDeviceCore {nullptr};
        pw_core *m_pwStreamCore {nullptr};
        pw_registry *m_pwRegistry {nullptr};
        pw_stream *m_pwStream {nullptr};
        spa_hook m_coreHook;
        spa_hook m_deviceHook;
        spa_hook m_streamHook;
        QThreadPool m_threadPool;
        AkVideoCaps m_curCaps;
        qint64 m_id {-1};
        int m_nBuffers {32};

        explicit CapturePipeWirePrivate(CapturePipeWire *self);
        ~CapturePipeWirePrivate();
        static void sequenceDone(void *userData, uint32_t id, int seq);
        static void nodeInfoChanged(void *userData,
                                    const struct pw_node_info *info);
        void readPropInfo(int seq, const spa_pod *param);
        void readProps(int seq, const spa_pod *param);
        QVector<AkFrac> readFrameRates(const spa_pod_prop *param) const;
        AkCaps videoCapsFromSpaFormat(spa_media_subtype mediaSubtype,
                                      spa_video_format format,
                                      const spa_rectangle &size,
                                      const AkFrac &fps);
        void readFormats(int seq, const spa_pod *param);
        void updateControl(DeviceControls &deviceControls,
                           const DeviceControl &control) const;
        void updateControlValue(DeviceControls &deviceControls,
                                const DeviceControl &control) const;
        static void nodeParamChanged(void *userData,
                                     int seq,
                                     uint32_t id,
                                     uint32_t index,
                                     uint32_t next,
                                     const struct spa_pod *param);
        static void deviceAdded(void *userData,
                                uint32_t id,
                                uint32_t permissions,
                                const char *type,
                                uint32_t version,
                                const struct spa_dict *props);
        static void deviceRemoved(void *userData, uint32_t id);
        static void onParamChanged(void *userData,
                                   uint32_t id,
                                   const struct spa_pod *param);
        static void onProcess(void *userData);
        void pipewireDevicesLoop();
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        void setControls(const QVariantMap &controls);
        const spa_pod *buildFormat(struct spa_pod_builder *podBuilder,
                                   spa_media_subtype mediaSubtype,
                                   spa_video_format format,
                                   int width,
                                   int height,
                                   const AkFrac &fps) const;
};

static const struct pw_core_events pipewireCoreEvents = {
    .version = PW_VERSION_CORE_EVENTS,
    .done       = CapturePipeWirePrivate::sequenceDone,
};

static const struct pw_node_events pipewireNodeEvents = {
    .version = PW_VERSION_NODE_EVENTS                  ,
    .info    = CapturePipeWirePrivate::nodeInfoChanged ,
    .param   = CapturePipeWirePrivate::nodeParamChanged,
};

static const struct pw_registry_events pipewireDeviceEvents = {
    .version       = PW_VERSION_REGISTRY_EVENTS           ,
    .global        = CapturePipeWirePrivate::deviceAdded  ,
    .global_remove = CapturePipeWirePrivate::deviceRemoved,
};

static const struct pw_stream_events pipewireStreamEvents = {
    .version       = PW_VERSION_STREAM_EVENTS              ,
    .param_changed = CapturePipeWirePrivate::onParamChanged,
    .process       = CapturePipeWirePrivate::onProcess     ,
};

CapturePipeWire::CapturePipeWire(QObject *parent):
    Capture(parent)
{
    this->d = new CapturePipeWirePrivate(this);

    auto binDir = QDir(BINDIR).absolutePath();
    auto pwPluginsDir = QDir(PIPEWIRE_MODULES_PATH).absolutePath();
    auto relPwPluginsDir = QDir(binDir).relativeFilePath(pwPluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relPwPluginsDir)) {
        auto path = appDir.absolutePath();
        path.replace("/", QDir::separator());

        if (QFileInfo::exists(path)
            && qEnvironmentVariableIsEmpty("PIPEWIRE_MODULE_DIR"))
            qputenv("PIPEWIRE_MODULE_DIR", path.toLocal8Bit());
    }

    auto pwSpaPluginsDir = QDir(PIPEWIRE_SPA_PLUGINS_PATH).absolutePath();
    auto relPwSpaPluginsDir = QDir(binDir).relativeFilePath(pwSpaPluginsDir);
    appDir.setPath(QCoreApplication::applicationDirPath());

    if (appDir.cd(relPwSpaPluginsDir)) {
        auto path = appDir.absolutePath();
        path.replace("/", QDir::separator());

        if (QFileInfo::exists(path)
            && qEnvironmentVariableIsEmpty("SPA_PLUGIN_DIR"))
            qputenv("SPA_PLUGIN_DIR", path.toLocal8Bit());
    }

    pw_init(nullptr, nullptr);
    auto result =
        QtConcurrent::run(&this->d->m_threadPool,
                          &CapturePipeWirePrivate::pipewireDevicesLoop,
                          this->d);
    Q_UNUSED(result)
}

CapturePipeWire::~CapturePipeWire()
{
    this->uninit();
    pw_main_loop_quit(this->d->m_pwDevicesLoop);
    this->d->m_threadPool.waitForDone();
    pw_deinit();
    delete this->d;
}

QStringList CapturePipeWire::webcams() const
{
    return this->d->m_devices;
}

QString CapturePipeWire::device() const
{
    return this->d->m_device;
}

QList<int> CapturePipeWire::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CapturePipeWire::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CapturePipeWire::ioMethod() const
{
    return {};
}

int CapturePipeWire::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CapturePipeWire::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CapturePipeWire::caps(const QString &webcam) const
{
    CaptureVideoCaps caps;

    for (auto &format: this->d->m_devicesCaps.value(webcam))
        caps << format.caps;

    return caps;
}

QVariantList CapturePipeWire::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CapturePipeWire::setImageControls(const QVariantMap &imageControls)
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

bool CapturePipeWire::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CapturePipeWire::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CapturePipeWire::setCameraControls(const QVariantMap &cameraControls)
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

bool CapturePipeWire::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CapturePipeWire::readFrame()
{
    this->d->m_controlsMutex.lockForRead();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setControls(controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lockForRead();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls)
        this->d->m_localCameraControls = cameraControls;

    AkPacket packet;

    this->d->m_mutex.lockForWrite();

    if (!this->d->m_curPacket)
        this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

    if (this->d->m_curPacket) {
        packet = this->d->m_curPacket;
        this->d->m_curPacket = {};
    }

    this->d->m_mutex.unlock();

    return packet;
}

bool CapturePipeWire::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();
    this->uninit();

    spa_media_subtype subType;
    spa_video_format format;
    int width = 0;
    int height = 0;
    AkFrac fps;

    this->d->m_curDevice = this->d->m_device;
    auto streams = this->streams();

    if (streams.isEmpty()) {
        this->uninit();

        return false;
    }

    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_curDevice);
    auto caps = supportedCaps[streams[0]];

    if (caps.caps.type() == AkCaps::CapsVideo) {
        AkVideoCaps videoCaps(caps.caps);
        subType = caps.subType;
        format = caps.format;
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    } else {
        AkCompressedVideoCaps videoCaps(caps.caps);
        subType = caps.subType;
        format = caps.format;
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    }

    this->d->m_pwStreamLoop =
        pw_thread_loop_new("PipeWire camera capture thread loop", nullptr);

    if (!this->d->m_pwStreamLoop) {
        this->uninit();
        qInfo() << "Error creating PipeWire desktop capture thread loop";

        return false;
    }

    this->d->m_pwStreamContext =
        pw_context_new(pw_thread_loop_get_loop(this->d->m_pwStreamLoop),
                       nullptr,
                       0);

    if (!this->d->m_pwStreamContext) {
        this->uninit();
        qInfo() << "Error creating PipeWire context";

        return false;
    }

    if (pw_thread_loop_start(this->d->m_pwStreamLoop) < 0) {
        this->uninit();
        qInfo() << "Error starting PipeWire main loop";

        return false;
    }

    pw_thread_loop_lock(this->d->m_pwStreamLoop);

    this->d->m_pwStreamCore =
        pw_context_connect(this->d->m_pwStreamContext, nullptr, 0);

    if (!this->d->m_pwStreamCore) {
        pw_thread_loop_unlock(this->d->m_pwStreamLoop);
        this->uninit();
        qInfo() << "Error connecting to the PipeWire file descriptor:" << strerror(errno);

        return false;
    }

    this->d->m_pwStream =
            pw_stream_new(this->d->m_pwStreamCore,
                          "Webcamoid Camera Capture",
                          pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                                            PW_KEY_MEDIA_CATEGORY, "Capture",
                                            PW_KEY_MEDIA_ROLE, "Camera",
#if PW_CHECK_VERSION(0, 3, 44)
                                            PW_KEY_TARGET_OBJECT, this->d->m_curDevice.toStdString().c_str(),
#endif
                                            nullptr));
    pw_stream_add_listener(this->d->m_pwStream,
                           &this->d->m_streamHook,
                           &pipewireStreamEvents,
                           this->d);

    QVector<const spa_pod *>params;
    static const size_t bufferSize = 4096;
    uint8_t buffer[bufferSize];
    auto podBuilder = SPA_POD_BUILDER_INIT(buffer, bufferSize);

    if (caps.caps.type() == AkCaps::CapsVideo) {
        if (!this->d->m_rawFormats.isEmpty())
            params << this->d->buildFormat(&podBuilder,
                                           subType,
                                           format,
                                           width,
                                           height,
                                           fps);

        for (auto &subType: this->d->m_encodedFormats[this->d->m_curDevice])
            params << this->d->buildFormat(&podBuilder,
                                           subType,
                                           SPA_VIDEO_FORMAT_ENCODED,
                                           width,
                                           height,
                                           fps);
    } else {
        params << this->d->buildFormat(&podBuilder,
                                       subType,
                                       SPA_VIDEO_FORMAT_ENCODED,
                                       width,
                                       height,
                                       fps);

        for (auto &subType_: this->d->m_encodedFormats[this->d->m_curDevice])
            if (subType_ != subType)
                params << this->d->buildFormat(&podBuilder,
                                               subType_,
                                               SPA_VIDEO_FORMAT_ENCODED,
                                               width,
                                               height,
                                               fps);

        if (!this->d->m_rawFormats.isEmpty()) {
            auto &rawFormats = this->d->m_rawFormats[this->d->m_curDevice];
            params << this->d->buildFormat(&podBuilder,
                                           SPA_MEDIA_SUBTYPE_raw,
                                           rawFormats.first(),
                                           width,
                                           height,
                                           fps);
        }
    }

    pw_stream_connect(this->d->m_pwStream,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT
                                      | PW_STREAM_FLAG_MAP_BUFFERS),
                      params.data(),
                      params.size());
    pw_thread_loop_unlock(this->d->m_pwStreamLoop);
    this->d->m_id = Ak::id();

    return true;
}

void CapturePipeWire::uninit()
{
    if (this->d->m_pwStreamLoop)
        pw_thread_loop_stop(this->d->m_pwStreamLoop);

    if (this->d->m_pwStream) {
        pw_stream_disconnect(this->d->m_pwStream);
        pw_stream_destroy(this->d->m_pwStream);
        this->d->m_pwStream = nullptr;
    }

    if (this->d->m_pwStreamContext) {
        pw_context_destroy(this->d->m_pwStreamContext);
        this->d->m_pwStreamContext = nullptr;
    }

    if (this->d->m_pwStreamLoop) {
        pw_thread_loop_destroy(this->d->m_pwStreamLoop);
        this->d->m_pwStreamLoop = nullptr;
    }
}

void CapturePipeWire::setDevice(const QString &device)
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

        if (this->d->m_devicesControls.contains(device)) {
            QVariantList controls;

            for (auto &control: this->d->m_devicesControls[device])
                controls << QVariant(QVariantList {
                    control.description,
                    control.type,
                    control.min,
                    control.max,
                    control.step,
                    control.def,
                    control.value,
                    control.menu
                });

            this->d->m_globalImageControls = controls;
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

void CapturePipeWire::setStreams(const QList<int> &streams)
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

void CapturePipeWire::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CapturePipeWire::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CapturePipeWire::resetDevice()
{
    this->setDevice("");
}

void CapturePipeWire::resetStreams()
{
    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CapturePipeWire::resetIoMethod()
{
    this->setIoMethod("any");
}

void CapturePipeWire::resetNBuffers()
{
    this->setNBuffers(32);
}

void CapturePipeWire::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

CapturePipeWirePrivate::CapturePipeWirePrivate(CapturePipeWire *self):
    self(self)
{
}

CapturePipeWirePrivate::~CapturePipeWirePrivate()
{
}

void CapturePipeWirePrivate::sequenceDone(void *userData, uint32_t id, int seq)
{
    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);
    self->m_sequenceParams.remove(seq - 1);
}

void CapturePipeWirePrivate::nodeInfoChanged(void *userData,
                                             const pw_node_info *info)
{
    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);

    for (uint32_t i = 0; i < info->n_params; i++) {
        if (!(info->params[i].flags & SPA_PARAM_INFO_READ))
            continue;

        auto &id = info->params[i].id;

        switch (id) {
        case SPA_PARAM_PropInfo: {
            auto node = self->m_deviceNodes.value(info->id);

            if (!node)
                return;

            auto &deviceId = self->m_deviceIds[info->id];

            if (!self->m_devicesControls.contains(deviceId))
                self->m_devicesControls[deviceId] = {};

            auto seq = pw_node_enum_params(node,
                                           0,
                                           id,
                                           0,
                                           -1,
                                           nullptr);
            self->m_sequenceParams[seq] = {info->id, id};
            pw_core_sync(self->m_pwDeviceCore, PW_ID_CORE, seq);

            break;
        }

        case SPA_PARAM_Props: {
            auto node = self->m_deviceNodes.value(info->id);

            if (!node)
                return;

            auto &deviceId = self->m_deviceIds[info->id];

            if (!self->m_devicesControls.contains(deviceId))
                self->m_devicesControls[deviceId] = {};

            auto seq = pw_node_enum_params(node,
                                           0,
                                           id,
                                           0,
                                           -1,
                                           nullptr);
            self->m_sequenceParams[seq] = {info->id, id};
            pw_core_sync(self->m_pwDeviceCore, PW_ID_CORE, seq);

            break;
        }

        case SPA_PARAM_EnumFormat: {
            auto node = self->m_deviceNodes.value(info->id);

            if (!node)
                return;

            auto &deviceId = self->m_deviceIds[info->id];

            if (!self->m_devicesCaps.contains(deviceId))
                self->m_devicesCaps[deviceId] = {};

            if (!self->m_rawFormats.contains(deviceId))
                self->m_rawFormats[deviceId] = {};

            if (!self->m_encodedFormats.contains(deviceId))
                self->m_encodedFormats[deviceId] = {};

            if (!self->m_widthRange.contains(deviceId))
                self->m_widthRange[deviceId] =
                    {std::numeric_limits<int>::max(), 0};

            if (!self->m_heightRange.contains(deviceId))
                self->m_heightRange[deviceId] =
                    {std::numeric_limits<int>::max(), 0};

            if (!self->m_frameRateRange.contains(deviceId))
                self->m_frameRateRange[deviceId] =
                    {{std::numeric_limits<qint64>::max(), 1},
                     {0, 1}};

            auto seq = pw_node_enum_params(node,
                                           0,
                                           id,
                                           0,
                                           -1,
                                           nullptr);
            self->m_sequenceParams[seq] = {info->id, id};
            pw_core_sync(self->m_pwDeviceCore, PW_ID_CORE, seq);

            break;
        }

        default:
            break;
        }
    }
}

void CapturePipeWirePrivate::readPropInfo(int seq, const spa_pod *param)
{
    if (SPA_POD_TYPE(param) != SPA_TYPE_Object)
        return;

    uint32_t propertyID = 0;
    const char *propertyDescription = nullptr;
    const struct spa_pod *type = nullptr;
    const struct spa_pod *labels = nullptr;

    if (spa_pod_parse_object(param,
                             SPA_TYPE_OBJECT_PropInfo , nullptr,
                             SPA_PROP_INFO_id         , SPA_POD_Id(&propertyID),
#if PW_CHECK_VERSION(0, 3, 41)
                             SPA_PROP_INFO_description, SPA_POD_String(&propertyDescription),
#else
                             SPA_PROP_INFO_name       , SPA_POD_String(&propertyDescription),
#endif
                             SPA_PROP_INFO_type       , SPA_POD_PodChoice(&type),
                             SPA_PROP_INFO_labels     , SPA_POD_OPT_PodStruct(&labels)) < 0) {
        return;
    }

    uint32_t nValues = 0;
    uint32_t choice = 0;
    auto values = spa_pod_get_values(type, &nValues, &choice);

    if (!values)
        return;

    switch (SPA_POD_TYPE(values)) {
    case SPA_TYPE_Int: {
        switch (choice) {
        case SPA_CHOICE_Step: {
            auto vals = reinterpret_cast<int32_t *>(SPA_POD_BODY(values));

            DeviceControl control {
                propertyID,
                propertyDescription,
                "integer",
                qreal(vals[1]),
                qreal(vals[2]),
                qreal(vals[3]),
                qreal(vals[0]),
                qreal(vals[0]),
                QStringList()
            };

            auto &nodeId = this->m_sequenceParams[seq].nodeId;
            auto &deviceId = this->m_deviceIds[nodeId];
            this->updateControl(this->m_devicesControls[deviceId], control);

            break;
        }

        case SPA_CHOICE_Enum: {
            if (!labels)
                return;

            struct spa_pod_parser podParser;
            struct spa_pod_frame podFrame;
            spa_pod_parser_pod(&podParser, labels);

            if (spa_pod_parser_push_struct(&podParser, &podFrame) < 0)
                return;

            auto vals = reinterpret_cast<int32_t *>(SPA_POD_BODY(values));

            QStringList menu;
            int32_t minMenu = std::numeric_limits<int32_t>::max();
            int32_t maxMenu = 0;

            forever {
                int32_t val = 0;
                const char *desc = nullptr;

                if (spa_pod_parser_get_int(&podParser, &val) < 0
                    || spa_pod_parser_get_string(&podParser, &desc) < 0)
                    break;

                menu << desc;
                minMenu = qMin(minMenu, val);
                maxMenu = qMax(maxMenu, val);
            }

            DeviceControl control {
                propertyID,
                propertyDescription,
                "menu",
                qreal(minMenu),
                qreal(maxMenu),
                1.0,
                qreal(vals[0]),
                qreal(vals[0]),
                menu
            };

            auto &nodeId = this->m_sequenceParams[seq].nodeId;
            auto &deviceId = this->m_deviceIds[nodeId];
            this->updateControl(this->m_devicesControls[deviceId], control);

            break;
        }

        default:
            break;
        }

        break;
    }

    case SPA_TYPE_Float: {
        auto vals = reinterpret_cast<float *>(SPA_POD_BODY(values));
        qreal step = qAbs(vals[2] - vals[1]) / 100.0;

        DeviceControl control {
            propertyID,
            propertyDescription,
            "float",
            vals[1],
            vals[2],
            step,
            vals[0],
            vals[0],
            QStringList()
        };

        auto &nodeId = this->m_sequenceParams[seq].nodeId;
        auto &deviceId = this->m_deviceIds[nodeId];
        this->updateControl(this->m_devicesControls[deviceId], control);

        break;
    }

    case SPA_TYPE_Bool: {
        auto vals = reinterpret_cast<int32_t *>(SPA_POD_BODY(values));

        DeviceControl control {
            propertyID,
            propertyDescription,
            "boolean",
            0.0,
            1.0,
            1.0,
            qreal(vals[0]),
            qreal(vals[0]),
            QStringList()
        };

        auto &nodeId = this->m_sequenceParams[seq].nodeId;
        auto &deviceId = this->m_deviceIds[nodeId];
        this->updateControl(this->m_devicesControls[deviceId], control);

        break;
    }

    default:
        break;
    }
}

void CapturePipeWirePrivate::readProps(int seq, const spa_pod *param)
{
    if (SPA_POD_TYPE(param) != SPA_TYPE_Object)
        return;

    static const QVector<spa_prop> supportedProperties {
        SPA_PROP_brightness,
        SPA_PROP_contrast,
        SPA_PROP_saturation,
        SPA_PROP_hue,
        SPA_PROP_gamma,
        SPA_PROP_exposure,
        SPA_PROP_gain,
        SPA_PROP_sharpness,
    };

    struct spa_pod_prop *prop = nullptr;

    SPA_POD_OBJECT_FOREACH(reinterpret_cast<const spa_pod_object *>(param), prop) {
        if (supportedProperties.contains(spa_prop(prop->key))
            || prop->key > SPA_PROP_START_CUSTOM) {
            switch (SPA_POD_TYPE(&prop->value)) {
            case SPA_TYPE_Bool: {
                auto value = SPA_POD_VALUE(spa_pod_bool, &prop->value);

                DeviceControl control {
                    prop->key,
                    "",
                    "",
                    0.0,
                    1.0,
                    1.0,
                    qreal(value),
                    qreal(value),
                    QStringList()
                };

                auto &nodeId = this->m_sequenceParams[seq].nodeId;
                auto &deviceId = this->m_deviceIds[nodeId];
                this->updateControlValue(this->m_devicesControls[deviceId], control);

                break;
            }

            case SPA_TYPE_Int: {
                auto value = SPA_POD_VALUE(spa_pod_int, &prop->value);

                DeviceControl control {
                    prop->key,
                    "",
                    "",
                    0.0,
                    1.0,
                    1.0,
                    qreal(value),
                    qreal(value),
                    QStringList()
                };
                auto &nodeId = this->m_sequenceParams[seq].nodeId;
                auto &deviceId = this->m_deviceIds[nodeId];
                this->updateControlValue(this->m_devicesControls[deviceId], control);

                break;
            }

            case SPA_TYPE_Float: {
                auto value = SPA_POD_VALUE(spa_pod_float, &prop->value);

                DeviceControl control {
                    prop->key,
                    "",
                    "",
                    0.0,
                    1.0,
                    1.0,
                    qreal(value),
                    qreal(value),
                    QStringList()
                };
                auto &nodeId = this->m_sequenceParams[seq].nodeId;
                auto &deviceId = this->m_deviceIds[nodeId];
                this->updateControlValue(this->m_devicesControls[deviceId], control);

                break;
            }

            default:
                break;
            }
        }
    }
}

QVector<AkFrac> CapturePipeWirePrivate::readFrameRates(const spa_pod_prop *param) const
{
    uint32_t nValues = 0;
    uint32_t choice = 0;
    auto values = spa_pod_get_values(&param->value, &nValues, &choice);

    if (!values || values->type != SPA_TYPE_Fraction)
        return {};

    auto fract = reinterpret_cast<spa_fraction *>(SPA_POD_BODY(values));
    QVector<AkFrac> frameRates;

    switch (choice) {
    case SPA_CHOICE_None:
        frameRates << AkFrac(fract[0].num, fract[0].denom);

        break;

    case SPA_CHOICE_Range:
    case SPA_CHOICE_Step:
    case SPA_CHOICE_Enum:
        for (uint32_t i = 0; i < nValues; i++) {
            AkFrac frameRate(fract[i].num, fract[i].denom);

            if (!frameRate.isNull() && !frameRates.contains(frameRate))
                frameRates << frameRate;
        }

        break;

    default:
        break;
    }

    return frameRates;
}

AkCaps CapturePipeWirePrivate::videoCapsFromSpaFormat(spa_media_subtype mediaSubtype,
                                                      spa_video_format format,
                                                      const spa_rectangle &size,
                                                      const AkFrac &fps)
{
    if (mediaSubtype == SPA_MEDIA_SUBTYPE_raw) {
        return AkVideoCaps(spaFmtToAkFmt->value(format),
                           size.width,
                           size.height,
                           fps);
    }

    return AkCompressedVideoCaps(spaCompressedToStrMap->value(mediaSubtype),
                                 size.width,
                                 size.height,
                                 fps);
}

void CapturePipeWirePrivate::readFormats(int seq, const spa_pod *param)
{
    if (SPA_POD_TYPE(param) != SPA_TYPE_Object)
        return;

    spa_media_subtype mediaSubtype = SPA_MEDIA_SUBTYPE_unknown;
    spa_video_format format = SPA_VIDEO_FORMAT_UNKNOWN;
    spa_rectangle size;

    if (spa_pod_parse_object(param,
                             SPA_TYPE_OBJECT_Format , nullptr,
                             SPA_FORMAT_mediaSubtype, SPA_POD_Id(&mediaSubtype),
                             SPA_FORMAT_VIDEO_format, SPA_POD_Id(&format),
                             SPA_FORMAT_VIDEO_size  , SPA_POD_Rectangle(&size)) < 0) {
        return;
    }


    if (mediaSubtype == SPA_MEDIA_SUBTYPE_raw) {
        if (!spaFmtToAkFmt->contains(format))
            return;
    } else {
        if (!spaCompressedToStrMap->contains(mediaSubtype))
            return;
    }

    QVector<AkFrac> frameRates;
    auto frameRate = spa_pod_object_find_prop(reinterpret_cast<const spa_pod_object *>(param),
                                              nullptr,
                                              SPA_FORMAT_VIDEO_framerate);

    if (frameRate)
        frameRates << this->readFrameRates(frameRate);
    else
        frameRates << AkFrac(30, 1);

    auto &nodeId = this->m_sequenceParams[seq].nodeId;
    auto &deviceId = this->m_deviceIds[nodeId];
    auto &rawFormats = this->m_rawFormats[deviceId];
    auto &encodedFormats = this->m_encodedFormats[deviceId];
    auto &widthRange = this->m_widthRange[deviceId];
    auto &heightRange = this->m_heightRange[deviceId];
    auto &fpsRange = this->m_frameRateRange[deviceId];
    auto &deviceCaps = this->m_devicesCaps[deviceId];

    for (auto &fps: frameRates) {
        auto caps =
            this->videoCapsFromSpaFormat(mediaSubtype, format, size, fps);
        auto it = std::find_if(deviceCaps.begin(),
                               deviceCaps.end(),
                               [&caps] (const DeviceSpaFormat &devCaps) {
                                   return devCaps.caps == caps;
                               });

        if (it != deviceCaps.end())
            continue;

        deviceCaps << DeviceSpaFormat(caps, mediaSubtype, format);
        widthRange.first = qMin(widthRange.first, int(size.width));
        widthRange.second = qMax(widthRange.second, int(size.width));
        heightRange.first = qMin(heightRange.first, int(size.height));
        heightRange.second = qMax(heightRange.second, int(size.height));
        fpsRange.first = qMin(fpsRange.first, fps);
        fpsRange.second = qMax(fpsRange.second, fps);
    }

    if (mediaSubtype == SPA_MEDIA_SUBTYPE_raw)
        if (!rawFormats.contains(format))
            rawFormats << format;

    if (mediaSubtype != SPA_MEDIA_SUBTYPE_raw)
        if (!encodedFormats.contains(mediaSubtype))
            encodedFormats << mediaSubtype;

    this->m_widthRange[deviceId] = widthRange;
    this->m_heightRange[deviceId] = heightRange;
    this->m_frameRateRange[deviceId] = fpsRange;
}

void CapturePipeWirePrivate::updateControl(DeviceControls &deviceControls,
                                           const DeviceControl &control) const
{
    for (auto &control_: deviceControls)
        if (control_.id == control.id) {
            control_.description = control.description;
            control_.type = control.type;
            control_.min = control.min;
            control_.max = control.max;
            control_.step = control.step;
            control_.def = control.def;
            control_.menu = control.menu;

            return;
        }

    deviceControls << control;
}

void CapturePipeWirePrivate::updateControlValue(DeviceControls &deviceControls,
                                                const DeviceControl &control) const
{
    for (auto &control_: deviceControls)
        if (control_.id == control.id) {
            control_.value = control.value;

            return;
        }

    deviceControls << control;
}

void CapturePipeWirePrivate::nodeParamChanged(void *userData,
                                              int seq,
                                              uint32_t id,
                                              uint32_t index,
                                              uint32_t next,
                                              const spa_pod *param)
{
    Q_UNUSED(id)
    Q_UNUSED(index)
    Q_UNUSED(next)

    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);

    switch (self->m_sequenceParams[seq].paramId) {
    case SPA_PARAM_PropInfo:
        self->readPropInfo(seq, param);

        break;

    case SPA_PARAM_Props:
        self->readProps(seq, param);

        break;

    case SPA_PARAM_EnumFormat:
        self->readFormats(seq, param);

        break;

    default:
        break;
    }
}

void CapturePipeWirePrivate::deviceAdded(void *userData,
                                         uint32_t id,
                                         uint32_t permissions,
                                         const char *type,
                                         uint32_t version,
                                         const spa_dict *props)
{
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);

    if (QString(type) != PW_TYPE_INTERFACE_Node)
        return;

    if (!props)
        return;

    auto mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    auto mediaRole = spa_dict_lookup(props, PW_KEY_MEDIA_ROLE);

    if (QString(mediaClass) != "Video/Source"
        || QString(mediaRole) != "Camera") {
        return;
    }

    auto path = spa_dict_lookup(props, PW_KEY_OBJECT_PATH);

    if (!path)
        return;

    auto node =
        reinterpret_cast<pw_node *>(pw_registry_bind(self->m_pwRegistry,
                                                     id,
                                                     type,
                                                     PW_VERSION_NODE,
                                                     0));

    if (!node)
        return;

    auto name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    auto description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);

    if (!self->m_devices.contains(name))
        self->m_devices << name;

    self->m_descriptions[name] = description;
    self->m_deviceIds[id] = name;
    self->m_deviceNodes[id] = node;
    self->m_nodeHooks[name] = {};
    auto &hook = self->m_nodeHooks[name];
    pw_proxy_add_object_listener(reinterpret_cast<pw_proxy *>(node),
                                 &hook,
                                 &pipewireNodeEvents,
                                 self);
    emit self->self->webcamsChanged(self->m_devices);
}

void CapturePipeWirePrivate::deviceRemoved(void *userData, uint32_t id)
{
    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);
    auto name = self->m_deviceIds.value(id);

    if (name.isEmpty())
        return;

    self->m_devices.removeAll(name);
    self->m_descriptions.remove(name);
    self->m_devicesCaps.remove(name);
    self->m_deviceIds.remove(id);
    self->m_deviceNodes.remove(id);
    self->m_devicesControls.remove(name);
    auto &hook = self->m_nodeHooks[name];
    spa_hook_remove(&hook);
    self->m_nodeHooks.remove(name);
    emit self->self->webcamsChanged(self->m_devices);
}

void CapturePipeWirePrivate::onParamChanged(void *userData,
                                            uint32_t id,
                                            const spa_pod *param)
{
    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);

    if (!param)
        return;

    switch (id) {
    case SPA_PARAM_Format: {
        uint32_t mediaType = 0;
        uint32_t mediaSubtype = 0;

        if (spa_format_parse(param,
                             &mediaType,
                             &mediaSubtype) < 0)
            return;

        if (mediaType != SPA_MEDIA_TYPE_video ||
            mediaSubtype != SPA_MEDIA_SUBTYPE_raw)
            return;

        spa_video_info_raw info;

        if (spa_format_video_raw_parse(param, &info) < 0)
            return;

        AkFrac fps(info.framerate.num, info.framerate.denom);
        self->m_curCaps =
            {spaFmtToAkFmt->value(info.format, AkVideoCaps::Format_none),
             int(info.size.width),
             int(info.size.height),
             fps};

        break;
    }

    default:
        break;
    }
}

void CapturePipeWirePrivate::onProcess(void *userData)
{
    auto self = reinterpret_cast<CapturePipeWirePrivate *>(userData);
    auto buffer = pw_stream_dequeue_buffer(self->m_pwStream);

    if (!buffer) {
        qDebug() << "out of buffers: %m";

        return;
    }

    if (!buffer->buffer->datas[0].data)
        return;

    AkVideoPacket packet(self->m_curCaps);
    auto iLineSize = buffer->buffer->datas[0].chunk->stride;
    auto oLineSize = packet.lineSize(0);
    auto lineSize = qMin<size_t>(iLineSize, oLineSize);

    for (int y = 0; y < packet.caps().height(); y++)
        memcpy(packet.line(0, y),
               reinterpret_cast<quint8 *>(buffer->buffer->datas[0].data) + y * iLineSize,
               lineSize);

    auto fps = self->m_curCaps.fps();
    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);
    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(self->m_id);

    self->m_mutex.lockForWrite();
    self->m_curPacket = packet;
    self->m_waitCondition.wakeAll();
    self->m_mutex.unlock();

    pw_stream_queue_buffer(self->m_pwStream, buffer);
}

void CapturePipeWirePrivate::pipewireDevicesLoop()
{
    this->m_pwDevicesLoop = pw_main_loop_new(nullptr);

    if (this->m_pwDevicesLoop) {
        auto pwContext = pw_context_new(pw_main_loop_get_loop(this->m_pwDevicesLoop),
                                        nullptr,
                                        0);
        this->m_pwDeviceCore = pw_context_connect(pwContext, nullptr, 0);
        memset(&this->m_coreHook, 0, sizeof(spa_hook));
        pw_core_add_listener(this->m_pwDeviceCore,
                             &this->m_coreHook,
                             &pipewireCoreEvents,
                             this);
        this->m_pwRegistry = pw_core_get_registry(this->m_pwDeviceCore,
                                                  PW_VERSION_REGISTRY,
                                                  0);
        memset(&this->m_deviceHook, 0, sizeof(spa_hook));
        pw_registry_add_listener(this->m_pwRegistry,
                                 &this->m_deviceHook,
                                 &pipewireDeviceEvents,
                                 this);
        pw_main_loop_run(this->m_pwDevicesLoop);
        pw_proxy_destroy((struct pw_proxy *) this->m_pwRegistry);
        pw_core_disconnect(this->m_pwDeviceCore);
        pw_context_destroy(pwContext);
        pw_main_loop_destroy(this->m_pwDevicesLoop);
    }
}

QVariantMap CapturePipeWirePrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CapturePipeWirePrivate::mapDiff(const QVariantMap &map1,
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

void CapturePipeWirePrivate::setControls(const QVariantMap &controls)
{
    if (!this->m_devicesControls.contains(this->m_curDevice))
        return;

    auto &controlsTable = this->m_devicesControls[this->m_curDevice];
    auto node =
        this->m_deviceNodes.value(this->m_deviceIds.key(this->m_curDevice));

    for (auto it = controls.cbegin(); it != controls.cend(); it++)
        for (auto &control: controlsTable)
            if (control.description == it.key()) {
                char buffer[1024];
                auto podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
                struct spa_pod_frame podFrame;

                spa_pod_builder_push_object(&podBuilder,
                                            &podFrame,
                                            SPA_TYPE_OBJECT_Props,
                                            SPA_PARAM_Props);

                if (control.type == "integer")
                    spa_pod_builder_add(&podBuilder,
                                        control.id,
                                        SPA_POD_Int(it.value().toInt()),
                                        nullptr);
                else if (control.type == "float")
                    spa_pod_builder_add(&podBuilder,
                                        control.id,
                                        SPA_POD_Float(it.value().toFloat()),
                                        nullptr);
                else if (control.type == "boolean")
                    spa_pod_builder_add(&podBuilder,
                                        control.id,
                                        SPA_POD_Bool(it.value().toBool()),
                                        nullptr);
                else
                  continue;

                auto param =
                    reinterpret_cast<spa_pod *>(spa_pod_builder_pop(&podBuilder,
                                                                    &podFrame));
                pw_node_set_param(node, SPA_PARAM_Props, 0, param);
            }
}

const spa_pod *CapturePipeWirePrivate::buildFormat(spa_pod_builder *podBuilder,
                                                   spa_media_subtype mediaSubtype,
                                                   spa_video_format format,
                                                   int width,
                                                   int height,
                                                   const AkFrac &fps) const
{
    const auto &widthRange = this->m_widthRange[this->m_curDevice];
    const auto &heightRange = this->m_heightRange[this->m_curDevice];
    const auto &fpsRange = this->m_frameRateRange[this->m_curDevice];

    auto defFrameSize = SPA_RECTANGLE(quint32(width), quint32(height));
    auto minFrameSize = SPA_RECTANGLE(uint32_t(widthRange.first),
                                      uint32_t(heightRange.first));
    auto maxFrameSize = SPA_RECTANGLE(uint32_t(widthRange.second),
                                      uint32_t(heightRange.second));

    auto defFps = SPA_FRACTION(quint32(fps.num()), quint32(fps.den()));
    auto minFps = SPA_FRACTION(uint32_t(fpsRange.first.num()),
                               uint32_t(fpsRange.first.den()));
    auto maxFps = SPA_FRACTION(uint32_t(fpsRange.second.num()),
                               uint32_t(fpsRange.second.den()));

    struct spa_pod_frame podFrame[2];
    spa_pod_builder_push_object(podBuilder,
                                &podFrame[0],
                                SPA_TYPE_OBJECT_Format,
                                SPA_PARAM_EnumFormat);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_mediaType,
                        SPA_POD_Id(SPA_MEDIA_TYPE_video),
                        0);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_mediaSubtype,
                        SPA_POD_Id(mediaSubtype),
                        0);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_VIDEO_format,
                        SPA_POD_Id(format),
                        0);
    spa_pod_builder_push_choice(podBuilder, &podFrame[1], SPA_CHOICE_Enum, 0);

    QVector<spa_video_format> formatChoices;

    if (mediaSubtype == SPA_MEDIA_SUBTYPE_raw) {
        formatChoices << format
                      << this->m_rawFormats[this->m_curDevice];
    } else {
        formatChoices << format
                      << SPA_VIDEO_FORMAT_ENCODED;
    }

    int i = 0;

    for (auto &fmt: formatChoices) {
          if (i == 0)
             spa_pod_builder_id(podBuilder, fmt);

        spa_pod_builder_id(podBuilder, fmt);
        i++;
    }

    spa_pod_builder_pop(podBuilder, &podFrame[1]);
    spa_pod_builder_add(podBuilder, SPA_FORMAT_VIDEO_size,
                        SPA_POD_CHOICE_RANGE_Rectangle(
                            &defFrameSize,
                            &minFrameSize,
                            &maxFrameSize),
                        0);
    spa_pod_builder_add(podBuilder, SPA_FORMAT_VIDEO_framerate,
                        SPA_POD_CHOICE_RANGE_Fraction(
                            &defFps,
                            &minFps,
                            &maxFps),
                        0);
    auto pod = spa_pod_builder_pop(podBuilder, &podFrame[0]);

    return reinterpret_cast<const spa_pod *>(pod);
}

#include "moc_capturepipewire.cpp"
