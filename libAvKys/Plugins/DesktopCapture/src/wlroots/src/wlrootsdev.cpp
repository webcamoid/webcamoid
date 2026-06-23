/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QElapsedTimer>
#include <QMutex>
#include <QDir>
#include <QThreadPool>
#include <QtConcurrent>
#include <QFuture>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <wayland-client.h>
#include <wayland-client-core.h>

#include "wlrootsdev.h"
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include "ext-image-capture-source-v1-client-protocol.h"
#include "ext-image-copy-capture-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "ext-foreign-toplevel-list-v1-client-protocol.h"

#define DEFAULT_PIXEL_FORMAT AkVideoCaps::Format_xrgbpack

enum CaptureProtocol
{
    None,
    ExtImageCopyCapture,
    WlrScreencopy,
};

enum CaptureType
{
    Output,
    Toplevel,
};

struct OutputInfo
{
    wl_output *output {nullptr};
    zxdg_output_v1 *xdgOutput {nullptr};
    ext_image_capture_source_v1 *captureSource {nullptr};
};

struct ToplevelInfo
{
    ext_foreign_toplevel_handle_v1 *handle {nullptr};
    ext_image_capture_source_v1 *captureSource {nullptr};
};

class ExtCaptureSession
{
    public:
        ext_image_copy_capture_session_v1 *session {nullptr};
        ext_image_copy_capture_frame_v1 *currentFrame {nullptr};

        int bufferWidth {0};
        int bufferHeight {0};
        uint32_t shmFormat {WL_SHM_FORMAT_XRGB8888};
        bool hasBufferSize {false};
        bool hasShmFormat {false};
        bool constraintsDone {false};
        bool stopped {false};

        bool frameReady {false};
        bool frameFailed {false};
        uint32_t failureReason {0};
        bool hasDamage {false};
        QRect damageRect;
        uint32_t transform {WL_OUTPUT_TRANSFORM_NORMAL};

        wl_buffer *buffer {nullptr};
        void *shmData {nullptr};
        int shmSize {0};
        int bufferStride {0};

        bool frameCaptured {false};
        int captureAttempts {0};
};

struct OutputData
{
    uint32_t name;
    QString id;
    QString description;
    int width;
    int height;
};

struct ToplevelData
{
    QString id;
    QString identifier;
    QString title;
    QString appId;
    bool closed;
};

class UpdateContext {
    public:
        wl_display *display;
        wl_registry *registry;
        wl_shm *shm;
        zxdg_output_manager_v1 *xdgOutputManager;
        ext_foreign_toplevel_list_v1 *toplevelList;
        QMap<uint32_t, OutputData> *outputs;
        QMap<QString, ToplevelData> *toplevels;
        bool done;

        UpdateContext():
            display(nullptr),
            registry(nullptr),
            shm(nullptr),
            xdgOutputManager(nullptr),
            toplevelList(nullptr),
            outputs(nullptr),
            toplevels(nullptr),
            done(false)
        {
        }
};

class WlrootsDevPrivate
{
    public:
        WlrootsDev *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCaps> m_devicesCaps;
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        QMutex m_mutex;
        QMutex m_waylandMutex;

        QThreadPool m_threadPool;
        QFuture<void> m_captureLoopResult;
        bool m_run {false};

        wl_display *m_display {nullptr};
        wl_registry *m_registry {nullptr};
        wl_shm *m_shm {nullptr};

        CaptureProtocol m_protocol {CaptureProtocol::None};
        CaptureType m_captureType {CaptureType::Output};
        zwlr_screencopy_manager_v1 *m_screencopyManager {nullptr};
        ext_image_copy_capture_manager_v1 *m_extCaptureManager {nullptr};
        ext_output_image_capture_source_manager_v1 *m_extOutputSourceManager {nullptr};
        ext_toplevel_image_capture_source_manager_v1 *m_extToplevelSourceManager {nullptr};
        ext_foreign_toplevel_list_v1 *m_toplevelList {nullptr};
        zxdg_output_manager_v1 *m_xdgOutputManager {nullptr};

        QMap<uint32_t, OutputInfo> m_outputs;
        uint32_t m_currentOutputId {0};

        QMap<QString, ToplevelInfo> m_toplevels;
        QString m_currentToplevelId;

        QMap<QString, uint32_t> m_deviceToOutputId;
        QMap<QString, QString> m_deviceToToplevelId;

        QMap<uint32_t, ExtCaptureSession> m_extSessions;
        QMap<QString, ExtCaptureSession> m_extToplevelSessions;

        zwlr_screencopy_frame_v1 *m_currentFrame {nullptr};
        wl_buffer *m_currentBuffer {nullptr};
        void *m_shmData {nullptr};
        int m_shmSize {0};
        int m_bufferWidth {0};
        int m_bufferHeight {0};
        int m_bufferStride {0};
        uint32_t m_bufferFormat {WL_SHM_FORMAT_XRGB8888};
        bool m_frameReady {false};
        bool m_frameFailed {false};
        bool m_bufferDone {false};
        bool m_hasFrameDamage {false};
        QRect m_damageRect;

        bool m_showCursor {false};

        int m_consecutiveFailures {0};
        const int MAX_CONSECUTIVE_FAILURES = 5;

        explicit WlrootsDevPrivate(WlrootsDev *self);
        ~WlrootsDevPrivate();

        bool connectWayland();
        void disconnectWayland();
        void captureLoop();
        bool checkDisplayHealth();

        void readFrameExtCapture(QElapsedTimer &et);
        void readFrameScreencopy(QElapsedTimer &et);

        bool createShmBuffer(int width, int height, int stride, uint32_t format,
                             wl_buffer **outBuffer, void **outData, int *outSize);
        void destroyShmBuffer(wl_buffer **buffer, void **data, int *size);

        bool initExtSession(uint32_t outputId);
        bool initExtToplevelSession(const QString &toplevelId);
        void destroyExtSession(uint32_t outputId);
        void destroyExtToplevelSession(const QString &toplevelId);
        void cleanupExtSessionResources(uint32_t outputId);
        void cleanupExtToplevelSessionResources(const QString &toplevelId);

        static int bytesPerPixel(uint32_t wlFormat);
        AkVideoCaps::PixelFormat pixelFormat(uint32_t wlFormat) const;

        ExtCaptureSession *findSession(ext_image_copy_capture_session_v1 *session);
        ExtCaptureSession *findSession(ext_image_copy_capture_frame_v1 *frame);

        static void registryGlobal(void *data,
                                   wl_registry *registry,
                                   uint32_t name,
                                   const char *interface,
                                   uint32_t version);
        static void registryGlobalRemove(void *data,
                                         wl_registry *registry,
                                         uint32_t name);

        static void toplevelListToplevel(void *data,
                                         ext_foreign_toplevel_list_v1 *list,
                                         ext_foreign_toplevel_handle_v1 *toplevel);
        static void toplevelListFinished(void *data,
                                         ext_foreign_toplevel_list_v1 *list);

        static void toplevelHandleClosed(void *data,
                                         ext_foreign_toplevel_handle_v1 *handle);
        static void toplevelHandleDone(void *data,
                                       ext_foreign_toplevel_handle_v1 *handle);
        static void toplevelHandleTitle(void *data,
                                        ext_foreign_toplevel_handle_v1 *handle,
                                        const char *title);
        static void toplevelHandleAppId(void *data,
                                        ext_foreign_toplevel_handle_v1 *handle,
                                        const char *app_id);
        static void toplevelHandleIdentifier(void *data,
                                             ext_foreign_toplevel_handle_v1 *handle,
                                             const char *identifier);

        static void extSessionBufferSize(void *data,
                                         ext_image_copy_capture_session_v1 *session,
                                         uint32_t width, uint32_t height);
        static void extSessionShmFormat(void *data,
                                        ext_image_copy_capture_session_v1 *session,
                                        uint32_t format);
        static void extSessionDmabufDevice(void *data,
                                           ext_image_copy_capture_session_v1 *session,
                                           struct wl_array *device);
        static void extSessionDmabufFormat(void *data,
                                           ext_image_copy_capture_session_v1 *session,
                                           uint32_t format,
                                           struct wl_array *modifiers);
        static void extSessionDone(void *data,
                                   ext_image_copy_capture_session_v1 *session);
        static void extSessionStopped(void *data,
                                      ext_image_copy_capture_session_v1 *session);

        static void extFrameTransform(void *data,
                                      ext_image_copy_capture_frame_v1 *frame,
                                      uint32_t transform);
        static void extFrameDamage(void *data,
                                   ext_image_copy_capture_frame_v1 *frame,
                                   int32_t x, int32_t y,
                                   int32_t width, int32_t height);
        static void extFramePresentationTime(void *data,
                                             ext_image_copy_capture_frame_v1 *frame,
                                             uint32_t tv_sec_hi,
                                             uint32_t tv_sec_lo,
                                             uint32_t tv_nsec);
        static void extFrameReady(void *data,
                                  ext_image_copy_capture_frame_v1 *frame);
        static void extFrameFailed(void *data,
                                   ext_image_copy_capture_frame_v1 *frame,
                                   uint32_t reason);

        static void screencopyFrameBuffer(void *data,
                                          zwlr_screencopy_frame_v1 *frame,
                                          uint32_t format,
                                          uint32_t width, uint32_t height,
                                          uint32_t stride);
        static void screencopyFrameFlags(void *data,
                                         zwlr_screencopy_frame_v1 *frame,
                                         uint32_t flags);
        static void screencopyFrameReady(void *data,
                                         zwlr_screencopy_frame_v1 *frame,
                                         uint32_t tv_sec_hi,
                                         uint32_t tv_sec_lo,
                                         uint32_t tv_nsec);
        static void screencopyFrameFailed(void *data,
                                          zwlr_screencopy_frame_v1 *frame);
        static void screencopyFrameDamage(void *data,
                                          zwlr_screencopy_frame_v1 *frame,
                                          uint32_t x, uint32_t y,
                                          uint32_t width, uint32_t height);
        static void screencopyFrameLinuxDmabuf(void *data,
                                               zwlr_screencopy_frame_v1 *frame,
                                               uint32_t format,
                                               uint32_t width, uint32_t height);
        static void screencopyFrameBufferDone(void *data,
                                              zwlr_screencopy_frame_v1 *frame);
};

static const wl_registry_listener registryListener = {
    WlrootsDevPrivate::registryGlobal,
    WlrootsDevPrivate::registryGlobalRemove
};

static const ext_foreign_toplevel_list_v1_listener toplevelListListener = {
    WlrootsDevPrivate::toplevelListToplevel,
    WlrootsDevPrivate::toplevelListFinished
};

static const ext_foreign_toplevel_handle_v1_listener toplevelHandleListener = {
    WlrootsDevPrivate::toplevelHandleClosed,
    WlrootsDevPrivate::toplevelHandleDone,
    WlrootsDevPrivate::toplevelHandleTitle,
    WlrootsDevPrivate::toplevelHandleAppId,
    WlrootsDevPrivate::toplevelHandleIdentifier
};

static const ext_image_copy_capture_session_v1_listener extSessionListener = {
    WlrootsDevPrivate::extSessionBufferSize,
    WlrootsDevPrivate::extSessionShmFormat,
    WlrootsDevPrivate::extSessionDmabufDevice,
    WlrootsDevPrivate::extSessionDmabufFormat,
    WlrootsDevPrivate::extSessionDone,
    WlrootsDevPrivate::extSessionStopped
};

static const ext_image_copy_capture_frame_v1_listener extFrameListener = {
    WlrootsDevPrivate::extFrameTransform,
    WlrootsDevPrivate::extFrameDamage,
    WlrootsDevPrivate::extFramePresentationTime,
    WlrootsDevPrivate::extFrameReady,
    WlrootsDevPrivate::extFrameFailed
};

static const zwlr_screencopy_frame_v1_listener screencopyFrameListener = {
    WlrootsDevPrivate::screencopyFrameBuffer,
    WlrootsDevPrivate::screencopyFrameFlags,
    WlrootsDevPrivate::screencopyFrameReady,
    WlrootsDevPrivate::screencopyFrameFailed,
    WlrootsDevPrivate::screencopyFrameDamage,
    WlrootsDevPrivate::screencopyFrameLinuxDmabuf,
    WlrootsDevPrivate::screencopyFrameBufferDone
};

WlrootsDev::WlrootsDev():
ScreenDev()
{
    this->d = new WlrootsDevPrivate(this);
    this->updateDevices();
}

WlrootsDev::~WlrootsDev()
{
    this->uninit();
    delete this->d;
}

AkFrac WlrootsDev::fps() const
{
    return this->d->m_fps;
}

QStringList WlrootsDev::medias()
{
    QMutexLocker locker(&this->d->m_waylandMutex);

    return this->d->m_devices;
}

QString WlrootsDev::media() const
{
    QMutexLocker locker(&this->d->m_waylandMutex);

    return this->d->m_device;
}

QList<int> WlrootsDev::streams() const
{
    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (!caps)
        return {};

    return {0};
}

int WlrootsDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString WlrootsDev::description(const QString &media)
{
    QMutexLocker locker(&this->d->m_waylandMutex);

    return this->d->m_descriptions.value(media);
}

AkVideoCaps WlrootsDev::caps(int stream)
{
    Q_UNUSED(stream)
    QMutexLocker locker(&this->d->m_waylandMutex);

    return this->d->m_devicesCaps.value(this->d->m_device);
}

bool WlrootsDev::canCaptureWindows() const
{
    return this->d->m_toplevelList != nullptr
           && this->d->m_extToplevelSourceManager != nullptr
           && this->d->m_extCaptureManager != nullptr;
}

bool WlrootsDev::canCaptureCursor() const
{
    return true;
}

bool WlrootsDev::canChangeCursorSize() const
{
    return false;
}

bool WlrootsDev::showCursor() const
{
    return this->d->m_showCursor;
}

int WlrootsDev::cursorSize() const
{
    return 0;
}

bool WlrootsDev::isWindow(const QString &media) const
{
    return media.startsWith("window://");
}

void WlrootsDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);

    if (this->d->m_run) {
        this->uninit();
        this->init();
    }
}

void WlrootsDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void WlrootsDev::setMedia(const QString &media)
{
    if (this->d->m_device == media)
        return;

    this->d->m_device = media;
    emit this->mediaChanged(media);
}

void WlrootsDev::setShowCursor(bool showCursor)
{
    if (this->d->m_showCursor == showCursor)
        return;

    this->d->m_showCursor = showCursor;
    emit this->showCursorChanged(showCursor);

    if (this->d->m_run) {
        this->uninit();
        this->init();
    }
}

void WlrootsDev::setCursorSize(int cursorSize)
{
    Q_UNUSED(cursorSize)
}

void WlrootsDev::resetMedia()
{
    QMutexLocker locker(&this->d->m_waylandMutex);

    if (!this->d->m_devices.isEmpty())
        this->setMedia(this->d->m_devices.first());
}

void WlrootsDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void WlrootsDev::resetStreams()
{
}

void WlrootsDev::resetShowCursor()
{
    this->setShowCursor(false);
}

void WlrootsDev::resetCursorSize()
{
}

bool WlrootsDev::init()
{
    this->uninit();

    this->d->m_run = true;
    this->d->m_captureLoopResult =
    QtConcurrent::run(&this->d->m_threadPool,
                      [this]() {
                          this->d->captureLoop();
                      });

    return true;
}

bool WlrootsDev::uninit()
{
    this->d->m_run = false;
    this->d->m_captureLoopResult.waitForFinished();

    return true;
}

void WlrootsDev::updateDevices()
{
    auto display = wl_display_connect(nullptr);

    if (!display) {
        qWarning() << "Failed to connect to Wayland display";

        return;
    }

    QMap<uint32_t, OutputData> outputs;
    QMap<QString, ToplevelData> toplevels;

    UpdateContext ctx;
    ctx.display = display;
    ctx.outputs = &outputs;
    ctx.toplevels = &toplevels;

    auto registryGlobal = [] (void *data,
                              wl_registry *registry,
                              uint32_t name,
                              const char *interface,
                              uint32_t version) {
        auto ctx = reinterpret_cast<UpdateContext *>(data);

        if (strcmp(interface, wl_shm_interface.name) == 0) {
            ctx->shm = reinterpret_cast<wl_shm *>(
                           wl_registry_bind(registry,
                                            name,
                                            &wl_shm_interface,
                                            1));
        } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
            ctx->xdgOutputManager =
                    reinterpret_cast<zxdg_output_manager_v1 *>(
                        wl_registry_bind(registry,
                                         name,
                                         &zxdg_output_manager_v1_interface,
                                         qMin<uint32_t>(version, 3)));
        } else if (strcmp(interface, ext_foreign_toplevel_list_v1_interface.name) == 0) {
            ctx->toplevelList =
                    reinterpret_cast<ext_foreign_toplevel_list_v1 *>(
                        wl_registry_bind(registry,
                                         name,
                                         &ext_foreign_toplevel_list_v1_interface,
                                         qMin<uint32_t>(version, 1)));
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
            OutputData output;
            output.name = name;
            output.width = 1920;
            output.height = 1080;

            auto outputRef = &(*ctx->outputs)[name];
            *outputRef = output;

            auto wlOutput = reinterpret_cast<wl_output *>(
                                wl_registry_bind(registry,
                                                 name,
                                                 &wl_output_interface,
                                                 qMin<uint32_t>(version, 4)));

            struct OutputListenerData {
                uint32_t name;
                OutputData *outputData;
                int logicalWidth;
                int logicalHeight;
                bool done;
            };

            auto outData = new OutputListenerData;
            outData->name = name;
            outData->outputData = outputRef;
            outData->logicalWidth = 0;
            outData->logicalHeight = 0;
            outData->done = false;

            auto outputGeometry = [](void *data,
                                     wl_output *output,
                                     int32_t x,
                                     int32_t y,
                                     int32_t physicalWidth,
                                     int32_t physicalHeight,
                                     int32_t subpixel,
                                     const char *make,
                                     const char *model,
                                     int32_t transform) {
                Q_UNUSED(output)
                Q_UNUSED(x)
                Q_UNUSED(y)
                Q_UNUSED(physicalWidth)
                Q_UNUSED(physicalHeight)
                Q_UNUSED(subpixel)
                Q_UNUSED(make)
                Q_UNUSED(transform)
                auto d = reinterpret_cast<OutputListenerData *>(data);

                if (model && strlen(model) > 0
                    && d->outputData->description.isEmpty()) {
                    d->outputData->description = QString::fromUtf8(model);
                }
            };

            auto outputMode = [](void *data, wl_output *output,
                                 uint32_t flags,
                                 int32_t width, int32_t height,
                                 int32_t refresh) {
                Q_UNUSED(output)
                Q_UNUSED(refresh)
                auto d = reinterpret_cast<OutputListenerData *>(data);

                if (flags & WL_OUTPUT_MODE_CURRENT) {
                    d->outputData->width = width;
                    d->outputData->height = height;
                }
            };

            auto outputDone = [](void *data, wl_output *output) {
                Q_UNUSED(output)
                auto d = reinterpret_cast<OutputListenerData *>(data);
                d->done = true;
            };

            auto outputScale = [](void *data,
                                  wl_output *output,
                                  int32_t factor) {
                Q_UNUSED(data)
                Q_UNUSED(output)
                Q_UNUSED(factor)
            };

            auto outputName = [](void *data,
                                 wl_output *output,
                                 const char *name) {
                Q_UNUSED(output)
                auto d = reinterpret_cast<OutputListenerData *>(data);

                if (name) {
                    d->outputData->id = QString::fromUtf8(name);

                    if (d->outputData->description.isEmpty())
                        d->outputData->description = QString::fromUtf8(name);
                }
            };

            auto outputDescription = [](void *data,
                                        wl_output *output,
                                        const char *description) {
                Q_UNUSED(output)
                auto d = reinterpret_cast<OutputListenerData *>(data);

                if (description && d->outputData->description.isEmpty())
                    d->outputData->description = QString::fromUtf8(description);
            };

            static const wl_output_listener outputListener = {
                outputGeometry,
                outputMode,
                outputDone,
                outputScale,
                outputName,
                outputDescription
            };

            wl_output_add_listener(wlOutput, &outputListener, outData);

            if (ctx->xdgOutputManager) {
                auto xdgOutput =
                        zxdg_output_manager_v1_get_xdg_output(ctx->xdgOutputManager,
                                                              wlOutput);

                struct XdgOutputData {
                    OutputListenerData *outData;
                    int logicalWidth;
                    int logicalHeight;
                    bool done;
                };

                auto xdgData = new XdgOutputData;
                xdgData->outData = outData;
                xdgData->logicalWidth = 0;
                xdgData->logicalHeight = 0;
                xdgData->done = false;

                auto xdgLogicalPosition = [](void *data,
                                             zxdg_output_v1 *xdgOutput,
                                             int32_t x,
                                             int32_t y) {
                    Q_UNUSED(data)
                    Q_UNUSED(xdgOutput)
                    Q_UNUSED(x)
                    Q_UNUSED(y)
                };

                auto xdgLogicalSize = [](void *data,
                                         zxdg_output_v1 *xdgOutput,
                                         int32_t width,
                                         int32_t height) {
                    Q_UNUSED(xdgOutput)
                    auto d = reinterpret_cast<XdgOutputData *>(data);
                    d->logicalWidth = width;
                    d->logicalHeight = height;
                };

                auto xdgDone = [](void *data, zxdg_output_v1 *xdgOutput) {
                    Q_UNUSED(xdgOutput)
                    auto d = reinterpret_cast<XdgOutputData *>(data);

                    if (zxdg_output_v1_get_version(xdgOutput) < 3)
                        d->done = true;
                };

                auto xdgName = [](void *data,
                                  zxdg_output_v1 *xdgOutput,
                                  const char *name) {
                    Q_UNUSED(xdgOutput)
                    auto d = reinterpret_cast<XdgOutputData *>(data);

                    if (name) {
                        d->outData->outputData->id = QString::fromUtf8(name);

                        if (d->outData->outputData->description.isEmpty())
                            d->outData->outputData->description = QString::fromUtf8(name);
                    }
                };

                auto xdgDescription = [](void *data, zxdg_output_v1 *xdgOutput,
                                         const char *description) {
                    Q_UNUSED(xdgOutput)
                    auto d = reinterpret_cast<XdgOutputData *>(data);

                    if (description)
                        d->outData->outputData->description =
                                QString::fromUtf8(description);
                };

                static const zxdg_output_v1_listener xdgOutputListener = {
                    xdgLogicalPosition,
                    xdgLogicalSize,
                    xdgDone,
                    xdgName,
                    xdgDescription
                };

                zxdg_output_v1_add_listener(xdgOutput, &xdgOutputListener, xdgData);
            }
        }
    };

    auto registryGlobalRemove = [](void *data,
                                   wl_registry *registry,
                                   uint32_t name) {
        Q_UNUSED(data)
        Q_UNUSED(registry)
        Q_UNUSED(name)
    };

    static const wl_registry_listener updateRegistryListener = {
        registryGlobal,
        registryGlobalRemove
    };

    ctx.registry = wl_display_get_registry(display);
    wl_registry_add_listener(ctx.registry, &updateRegistryListener, &ctx);

    wl_display_roundtrip(display);

    if (ctx.toplevelList) {
        struct UpdateToplevelContext {
            QMap<QString, ToplevelData> *toplevels;
            QMap<QString, ToplevelData> pendingToplevels;
        };

        auto toplevelCtx = new UpdateToplevelContext;
        toplevelCtx->toplevels = &toplevels;

        auto toplevelListToplevel = [](void *data,
                                       ext_foreign_toplevel_list_v1 *list,
                                       ext_foreign_toplevel_handle_v1 *toplevel) {
            Q_UNUSED(list)
            auto ctx = reinterpret_cast<UpdateToplevelContext *>(data);

            struct ToplevelHandleData {
                QString key;
                ToplevelData data;
                bool closed;
                bool done;
                bool hasIdentifier;
            };

            auto handleData = new ToplevelHandleData;
            handleData->closed = false;
            handleData->done = false;
            handleData->hasIdentifier = false;
            handleData->data.closed = false;

            auto closed = [](void *data, ext_foreign_toplevel_handle_v1 *handle) {
                Q_UNUSED(handle)
                auto d = reinterpret_cast<ToplevelHandleData *>(data);
                d->closed = true;
                d->data.closed = true;
            };

            auto done = [](void *data, ext_foreign_toplevel_handle_v1 *handle) {
                Q_UNUSED(handle)
                auto d = reinterpret_cast<ToplevelHandleData *>(data);
                d->done = true;
            };

            auto title = [](void *data, ext_foreign_toplevel_handle_v1 *handle,
                            const char *title) {
                Q_UNUSED(handle)
                auto d = reinterpret_cast<ToplevelHandleData *>(data);

                if (title)
                    d->data.title = QString::fromUtf8(title);
            };

            auto appId = [](void *data, ext_foreign_toplevel_handle_v1 *handle,
                            const char *app_id) {
                Q_UNUSED(handle)
                auto d = reinterpret_cast<ToplevelHandleData *>(data);

                if (app_id)
                    d->data.appId = QString::fromUtf8(app_id);
            };

            auto identifier = [](void *data, ext_foreign_toplevel_handle_v1 *handle,
                                 const char *identifier) {
                Q_UNUSED(handle)
                auto d = reinterpret_cast<ToplevelHandleData *>(data);

                if (identifier) {
                    d->data.identifier = QString::fromUtf8(identifier);
                    d->hasIdentifier = true;
                    d->data.id = QString::fromUtf8(identifier);
                }
            };

            static const ext_foreign_toplevel_handle_v1_listener handleListener = {
                closed,
                done,
                title,
                appId,
                identifier
            };

            ext_foreign_toplevel_handle_v1_add_listener(toplevel,
                                                        &handleListener,
                                                        handleData);

            handleData->key =
                    QString::number(reinterpret_cast<quintptr>(toplevel));
            ctx->pendingToplevels[handleData->key] = handleData->data;
        };

        auto toplevelListFinished = [](void *data,
                                       ext_foreign_toplevel_list_v1 *list) {
            Q_UNUSED(list)
            Q_UNUSED(data)
        };

        static const ext_foreign_toplevel_list_v1_listener updateToplevelListListener = {
            toplevelListToplevel,
            toplevelListFinished
        };

        ext_foreign_toplevel_list_v1_add_listener(ctx.toplevelList,
                                                  &updateToplevelListListener,
                                                  toplevelCtx);

        wl_display_roundtrip(display);

        for (auto it = toplevelCtx->pendingToplevels.begin();
             it != toplevelCtx->pendingToplevels.end();
             ++it) {
            toplevels[it.key()] = it.value();
        }

        delete toplevelCtx;
    }

    QStringList devices;
    QMap<QString, QString> descriptions;
    QMap<QString, AkVideoCaps> devicesCaps;
    QMap<QString, uint32_t> deviceToOutputId;
    QMap<QString, QString> deviceToToplevelId;

    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        auto outputName = it.value().id;

        if (outputName.isEmpty())
            outputName = QString("output_%1").arg(it.key());

        auto deviceId = QString("screen://%1").arg(outputName);
        devices << deviceId;
        deviceToOutputId[deviceId] = it.key();

        auto desc = it.value().description;

        if (desc.isEmpty())
            desc = outputName;

        descriptions[deviceId] = desc;
        devicesCaps[deviceId] = AkVideoCaps(DEFAULT_PIXEL_FORMAT,
                                            it.value().width,
                                            it.value().height,
                                            this->d->m_fps);
    }

    for (auto it = toplevels.begin(); it != toplevels.end(); ++it) {
        if (it.value().closed || it.value().identifier.isEmpty())
            continue;

        auto deviceId = QString("window://%1").arg(it.value().identifier);
        devices << deviceId;
        deviceToToplevelId[deviceId] = it.key();

        auto desc = it.value().title;

        if (desc.isEmpty())
            desc = it.value().appId;

        if (desc.isEmpty())
            desc = it.value().identifier;

        descriptions[deviceId] = desc;
        devicesCaps[deviceId] = AkVideoCaps(DEFAULT_PIXEL_FORMAT,
                                            1920,
                                            1080,
                                            this->d->m_fps);
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
        deviceToOutputId.clear();
        deviceToToplevelId.clear();
    }

    bool devicesChanged = false;
    bool mediaChanged = false;
    QString newMedia;

    {
        QMutexLocker locker(&this->d->m_waylandMutex);

        this->d->m_descriptions = descriptions;
        this->d->m_devicesCaps = devicesCaps;
        this->d->m_deviceToOutputId = deviceToOutputId;
        this->d->m_deviceToToplevelId = deviceToToplevelId;

        devicesChanged = (this->d->m_devices != devices);

        if (devicesChanged)
            this->d->m_devices = devices;

        if (!this->d->m_devices.contains(this->d->m_device)) {
            newMedia = devices.isEmpty()? QString(): devices.first();
            mediaChanged = (this->d->m_device != newMedia);
            this->d->m_device = newMedia;
        }
    }

    if (devicesChanged)
        emit this->mediasChanged(devices);

    if (mediaChanged)
        emit this->mediaChanged(newMedia);

    wl_display_disconnect(display);
}

WlrootsDevPrivate::WlrootsDevPrivate(WlrootsDev *self):
    self(self)
{
}

WlrootsDevPrivate::~WlrootsDevPrivate()
{
}

void WlrootsDevPrivate::captureLoop()
{
    if (!this->connectWayland()) {
        qWarning() << "Failed to connect Wayland in capture loop";

        return;
    }

    this->m_captureType = self->isWindow(this->m_device)?
                              CaptureType::Toplevel:
                              CaptureType::Output;

    if (this->m_captureType == CaptureType::Output) {
        this->m_currentOutputId =
                this->m_deviceToOutputId.value(this->m_device, 0);

        if (this->m_currentOutputId == 0) {
            qCritical() << "A valid session ID was not found for" << this->m_device;
            qCritical() << "Valid devices ID are:" << this->m_deviceToOutputId.keys();
            this->disconnectWayland();

            return;
        }

        if (this->m_protocol == CaptureProtocol::ExtImageCopyCapture) {
            if (!this->initExtSession(this->m_currentOutputId)) {
                if (this->m_screencopyManager) {
                    qWarning() << "ext-image-copy-capture failed, falling back to wlr-screencopy";
                    this->m_protocol = CaptureProtocol::WlrScreencopy;
                } else {
                    qCritical() << "Failed to initialize the screen capture session for" << this->m_device;
                    this->cleanupExtSessionResources(this->m_currentOutputId);
                    this->m_currentOutputId = 0;
                    this->disconnectWayland();

                    return;
                }
            }
        }
    } else {
        if (this->m_protocol != CaptureProtocol::ExtImageCopyCapture) {
            qCritical() << "ExtImageCopyCapture protocol is required for capturing windows";
            this->disconnectWayland();

            return;
        }

        this->m_currentToplevelId =
                this->m_deviceToToplevelId.value(this->m_device);

        if (this->m_currentToplevelId.isEmpty()) {
            qCritical() << "A valid session ID was not found for" << this->m_device;
            qCritical() << "Valid devices ID are:" << this->m_deviceToOutputId.keys();
            this->disconnectWayland();

            return;
        }

        if (!this->initExtToplevelSession(this->m_currentToplevelId)) {
            qCritical() << "Failed to initialize the window capture session for" << this->m_device;
            this->cleanupExtToplevelSessionResources(this->m_currentToplevelId);
            this->m_currentToplevelId.clear();
            this->disconnectWayland();

            return;
        }
    }

    qInfo() << "Wayland screen capture started";

    this->m_id = Ak::id();
    this->m_consecutiveFailures = 0;

    QElapsedTimer et;
    et.start();

    while (this->m_run) {
        QMutexLocker locker(&this->m_waylandMutex);

        if (this->m_display && this->m_run) {
            wl_display_dispatch_pending(this->m_display);
            wl_display_flush(this->m_display);

            if (this->m_protocol != CaptureProtocol::None) {
                if (this->m_captureType == CaptureType::Toplevel) {
                    if (this->m_protocol == CaptureProtocol::ExtImageCopyCapture)
                        this->readFrameExtCapture(et);
                } else {
                    switch (this->m_protocol) {
                        case CaptureProtocol::ExtImageCopyCapture:
                            this->readFrameExtCapture(et);

                            break;

                        case CaptureProtocol::WlrScreencopy:
                            this->readFrameScreencopy(et);

                            break;

                        default:
                            break;
                    }
                }
            }
        }
    }

    QMutexLocker locker(&this->m_waylandMutex);

    if (this->m_currentOutputId != 0) {
        this->cleanupExtSessionResources(this->m_currentOutputId);
        this->m_currentOutputId = 0;
    }

    if (!this->m_currentToplevelId.isEmpty()) {
        this->cleanupExtToplevelSessionResources(this->m_currentToplevelId);
        this->m_currentToplevelId.clear();
    }

    this->disconnectWayland();

    qInfo() << "Wayland screen capture finished";
}

bool WlrootsDevPrivate::connectWayland()
{
    if (this->m_display)
        return true;

    this->m_display = wl_display_connect(nullptr);

    if (!this->m_display) {
        qWarning() << "Failed to connect to Wayland display";

        return false;
    }

    this->m_registry = wl_display_get_registry(this->m_display);
    wl_registry_add_listener(this->m_registry, &registryListener, this);

    if (wl_display_roundtrip(this->m_display) < 0) {
        qWarning() << "Wayland roundtrip failed";
        this->disconnectWayland();

        return false;
    }

    if (this->m_toplevelList) {
        ext_foreign_toplevel_list_v1_add_listener(this->m_toplevelList,
                                                  &toplevelListListener,
                                                  this);

        if (wl_display_roundtrip(this->m_display) < 0)
            qWarning() << "Wayland roundtrip for toplevels failed";
    }

    if (this->m_extCaptureManager && this->m_extOutputSourceManager) {
        this->m_protocol = CaptureProtocol::ExtImageCopyCapture;
        qInfo() << "Using ext-image-copy-capture-v1";
    } else if (this->m_screencopyManager) {
        this->m_protocol = CaptureProtocol::WlrScreencopy;
        qInfo() << "Using wlr-screencopy-unstable-v1";
    } else {
        qWarning() << "No screen capture protocol available";
        this->disconnectWayland();

        return false;
    }

    if (!this->m_shm) {
        qWarning() << "Compositor does not support wl_shm";
        this->disconnectWayland();

        return false;
    }

    return true;
}

void WlrootsDevPrivate::disconnectWayland()
{
    for (auto it = this->m_extSessions.begin();
         it != this->m_extSessions.end();
         ++it) {
        this->destroyExtSession(it.key());
    }

    this->m_extSessions.clear();

    for (auto it = this->m_extToplevelSessions.begin();
         it != this->m_extToplevelSessions.end();
         ++it) {
        this->destroyExtToplevelSession(it.key());
    }

    this->m_extToplevelSessions.clear();

    this->destroyShmBuffer(&this->m_currentBuffer,
                           &this->m_shmData,
                           &this->m_shmSize);

    if (this->m_currentFrame) {
        zwlr_screencopy_frame_v1_destroy(this->m_currentFrame);
        this->m_currentFrame = nullptr;
    }

    for (auto it = this->m_outputs.begin(); it != this->m_outputs.end(); ++it) {
        if (it.value().captureSource) {
            ext_image_capture_source_v1_destroy(it.value().captureSource);
            it.value().captureSource = nullptr;
        }

        if (it.value().output) {
            wl_output_destroy(it.value().output);
            it.value().output = nullptr;
        }

        if (it.value().xdgOutput) {
            zxdg_output_v1_destroy(it.value().xdgOutput);
            it.value().xdgOutput = nullptr;
        }
    }

    this->m_outputs.clear();

    for (auto it = this->m_toplevels.begin(); it != this->m_toplevels.end(); ++it) {
        if (it.value().captureSource) {
            ext_image_capture_source_v1_destroy(it.value().captureSource);
            it.value().captureSource = nullptr;
        }

        if (it.value().handle) {
            ext_foreign_toplevel_handle_v1_destroy(it.value().handle);
            it.value().handle = nullptr;
        }
    }

    this->m_toplevels.clear();

    if (this->m_toplevelList) {
        ext_foreign_toplevel_list_v1_destroy(this->m_toplevelList);
        this->m_toplevelList = nullptr;
    }

    if (this->m_extToplevelSourceManager) {
        ext_toplevel_image_capture_source_manager_v1_destroy(this->m_extToplevelSourceManager);
        this->m_extToplevelSourceManager = nullptr;
    }

    if (this->m_extOutputSourceManager) {
        ext_output_image_capture_source_manager_v1_destroy(this->m_extOutputSourceManager);
        this->m_extOutputSourceManager = nullptr;
    }

    if (this->m_extCaptureManager) {
        ext_image_copy_capture_manager_v1_destroy(this->m_extCaptureManager);
        this->m_extCaptureManager = nullptr;
    }

    if (this->m_screencopyManager) {
        zwlr_screencopy_manager_v1_destroy(this->m_screencopyManager);
        this->m_screencopyManager = nullptr;
    }

    if (this->m_xdgOutputManager) {
        zxdg_output_manager_v1_destroy(this->m_xdgOutputManager);
        this->m_xdgOutputManager = nullptr;
    }

    if (this->m_shm) {
        wl_shm_destroy(this->m_shm);
        this->m_shm = nullptr;
    }

    if (this->m_registry) {
        wl_registry_destroy(this->m_registry);
        this->m_registry = nullptr;
    }

    if (this->m_display) {
        wl_display_disconnect(this->m_display);
        this->m_display = nullptr;
    }

    this->m_protocol = CaptureProtocol::None;
}

bool WlrootsDevPrivate::checkDisplayHealth()
{
    if (!this->m_display)
        return false;

    if (wl_display_get_error(this->m_display)) {
        qWarning() << "Wayland display error detected";

        return false;
    }

    return true;
}

bool WlrootsDevPrivate::initExtSession(uint32_t outputId)
{
    if (!this->m_extCaptureManager || !this->m_extOutputSourceManager)
        return false;

    if (!this->m_outputs.contains(outputId))
        return false;

    auto output = &this->m_outputs[outputId];
    this->cleanupExtSessionResources(outputId);

    output->captureSource =
            ext_output_image_capture_source_manager_v1_create_source(this->m_extOutputSourceManager,
                                                                     output->output);

    if (!output->captureSource) {
        qWarning() << "Failed to create image capture source for output";

        return false;
    }

    auto &session = this->m_extSessions[outputId];
    uint32_t options = this->m_showCursor?
                           EXT_IMAGE_COPY_CAPTURE_MANAGER_V1_OPTIONS_PAINT_CURSORS:
                           0;

    session.session = ext_image_copy_capture_manager_v1_create_session(this->m_extCaptureManager,
                                                                       output->captureSource,
                                                                       options);

    if (!session.session) {
        qWarning() << "Failed to create image copy capture session";
        this->m_extSessions.remove(outputId);

        return false;
    }

    ext_image_copy_capture_session_v1_add_listener(session.session,
                                                   &extSessionListener,
                                                   this);

    wl_display_roundtrip(this->m_display);

    if (!session.constraintsDone || session.stopped) {
        qWarning() << "Capture session constraints failed or session stopped";
        this->destroyExtSession(outputId);

        return false;
    }

    return true;
}

bool WlrootsDevPrivate::initExtToplevelSession(const QString &toplevelId)
{
    if (!this->m_extCaptureManager || !this->m_extToplevelSourceManager)
        return false;

    if (!this->m_toplevels.contains(toplevelId))
        return false;

    auto toplevel = &this->m_toplevels[toplevelId];
    this->cleanupExtToplevelSessionResources(toplevelId);

    toplevel->captureSource =
            ext_toplevel_image_capture_source_manager_v1_create_source(this->m_extToplevelSourceManager,
                                                                       toplevel->handle);

    if (!toplevel->captureSource) {
        qWarning() << "Failed to create image capture source for toplevel";

        return false;
    }

    auto &session = this->m_extToplevelSessions[toplevelId];
    uint32_t options = this->m_showCursor?
                           EXT_IMAGE_COPY_CAPTURE_MANAGER_V1_OPTIONS_PAINT_CURSORS:
                           0;

    session.session = ext_image_copy_capture_manager_v1_create_session(this->m_extCaptureManager,
                                                                       toplevel->captureSource,
                                                                       options);

    if (!session.session) {
        qWarning() << "Failed to create image copy capture session for toplevel";
        this->m_extToplevelSessions.remove(toplevelId);

        return false;
    }

    ext_image_copy_capture_session_v1_add_listener(session.session,
                                                   &extSessionListener,
                                                   this);

    wl_display_roundtrip(this->m_display);

    if (!session.constraintsDone || session.stopped) {
        qWarning() << "Toplevel capture session constraints failed or session stopped";
        this->destroyExtToplevelSession(toplevelId);

        return false;
    }

    return true;
}

void WlrootsDevPrivate::cleanupExtSessionResources(uint32_t outputId)
{
    this->destroyExtSession(outputId);

    if (this->m_outputs.contains(outputId)) {
        auto output = &this->m_outputs[outputId];

        if (output->captureSource) {
            ext_image_capture_source_v1_destroy(output->captureSource);
            output->captureSource = nullptr;
        }
    }
}

void WlrootsDevPrivate::cleanupExtToplevelSessionResources(const QString &toplevelId)
{
    this->destroyExtToplevelSession(toplevelId);

    if (this->m_toplevels.contains(toplevelId)) {
        auto toplevel = &this->m_toplevels[toplevelId];

        if (toplevel->captureSource) {
            ext_image_capture_source_v1_destroy(toplevel->captureSource);
            toplevel->captureSource = nullptr;
        }
    }
}

void WlrootsDevPrivate::destroyExtSession(uint32_t outputId)
{
    if (!this->m_extSessions.contains(outputId))
        return;

    auto &session = this->m_extSessions[outputId];

    if (session.currentFrame) {
        ext_image_copy_capture_frame_v1_destroy(session.currentFrame);
        session.currentFrame = nullptr;
    }

    this->destroyShmBuffer(&session.buffer, &session.shmData, &session.shmSize);

    if (session.session) {
        ext_image_copy_capture_session_v1_destroy(session.session);
        session.session = nullptr;
    }

    session.bufferWidth = 0;
    session.bufferHeight = 0;
    session.shmFormat = WL_SHM_FORMAT_XRGB8888;
    session.hasBufferSize = false;
    session.hasShmFormat = false;
    session.constraintsDone = false;
    session.stopped = false;
    session.frameReady = false;
    session.frameFailed = false;
    session.failureReason = 0;
    session.hasDamage = false;
    session.bufferStride = 0;
    session.frameCaptured = false;
    session.captureAttempts = 0;

    this->m_extSessions.remove(outputId);
}

void WlrootsDevPrivate::destroyExtToplevelSession(const QString &toplevelId)
{
    if (!this->m_extToplevelSessions.contains(toplevelId))
        return;

    auto &session = this->m_extToplevelSessions[toplevelId];

    if (session.currentFrame) {
        ext_image_copy_capture_frame_v1_destroy(session.currentFrame);
        session.currentFrame = nullptr;
    }

    this->destroyShmBuffer(&session.buffer, &session.shmData, &session.shmSize);

    if (session.session) {
        ext_image_copy_capture_session_v1_destroy(session.session);
        session.session = nullptr;
    }

    session.bufferWidth = 0;
    session.bufferHeight = 0;
    session.shmFormat = WL_SHM_FORMAT_XRGB8888;
    session.hasBufferSize = false;
    session.hasShmFormat = false;
    session.constraintsDone = false;
    session.stopped = false;
    session.frameReady = false;
    session.frameFailed = false;
    session.failureReason = 0;
    session.hasDamage = false;
    session.bufferStride = 0;
    session.frameCaptured = false;
    session.captureAttempts = 0;

    this->m_extToplevelSessions.remove(toplevelId);
}

int WlrootsDevPrivate::bytesPerPixel(uint32_t wlFormat)
{
    switch (wlFormat) {
    case WL_SHM_FORMAT_RGB565:
        return 2;
    case WL_SHM_FORMAT_RGB888:
    case WL_SHM_FORMAT_BGR888:
        return 3;
    default:
        return 4;
    }
}

void WlrootsDevPrivate::readFrameExtCapture(QElapsedTimer &et)
{
    ExtCaptureSession *session = nullptr;

    if (this->m_captureType == CaptureType::Output) {
        if (this->m_currentOutputId == 0
            || !this->m_extSessions.contains(this->m_currentOutputId))
            return;

        session = &this->m_extSessions[this->m_currentOutputId];
    } else {
        if (this->m_currentToplevelId.isEmpty()
            || !this->m_extToplevelSessions.contains(this->m_currentToplevelId))
            return;

        session = &this->m_extToplevelSessions[this->m_currentToplevelId];
    }

    if (!session->session || session->stopped)
        return;

    session->captureAttempts++;

    if (session->currentFrame) {
        ext_image_copy_capture_frame_v1_destroy(session->currentFrame);
        session->currentFrame = nullptr;
        session->frameReady = false;
        session->frameFailed = false;
        session->hasDamage = false;
        session->frameCaptured = false;
    }

    session->currentFrame =
            ext_image_copy_capture_session_v1_create_frame(session->session);

    if (!session->currentFrame) {
        qWarning() << "Failed to create ext capture frame";

        return;
    }

    ext_image_copy_capture_frame_v1_add_listener(session->currentFrame,
                                                 &extFrameListener,
                                                 this);

    int stride = session->bufferWidth * bytesPerPixel(session->shmFormat);

    if (session->buffer)
        this->destroyShmBuffer(&session->buffer,
                               &session->shmData,
                               &session->shmSize);

    if (!this->createShmBuffer(session->bufferWidth,
        session->bufferHeight,
        stride,
        session->shmFormat,
        &session->buffer,
        &session->shmData,
        &session->shmSize)) {
        qWarning() << "Failed to create SHM buffer for ext capture";
        ext_image_copy_capture_frame_v1_destroy(session->currentFrame);
        session->currentFrame = nullptr;

        return;
    }

    session->bufferStride = stride;

    ext_image_copy_capture_frame_v1_attach_buffer(session->currentFrame, session->buffer);
    ext_image_copy_capture_frame_v1_damage_buffer(session->currentFrame,
                                                  0,
                                                  0,
                                                  session->bufferWidth,
                                                  session->bufferHeight);

    ext_image_copy_capture_frame_v1_capture(session->currentFrame);
    session->frameCaptured = true;

    int roundtripResult = wl_display_roundtrip(this->m_display);

    if (roundtripResult < 0) {
        qWarning() << "Wayland roundtrip failed";
        ext_image_copy_capture_frame_v1_destroy(session->currentFrame);
        session->currentFrame = nullptr;
        session->frameCaptured = false;

        return;
    }

    wl_display_dispatch_pending(this->m_display);

    if (!session->frameReady && !session->frameFailed) {
        session->frameCaptured = false;

        return;
    }

    if (session->frameFailed || !session->shmData) {
        ext_image_copy_capture_frame_v1_destroy(session->currentFrame);
        session->currentFrame = nullptr;
        session->frameReady = false;
        session->frameFailed = false;
        session->frameCaptured = false;
        this->m_consecutiveFailures++;

        return;
    }

    this->m_consecutiveFailures = 0;

    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

    AkVideoCaps videoCaps(this->pixelFormat(session->shmFormat),
                          session->bufferWidth,
                          session->bufferHeight,
                          fps);
    AkVideoPacket videoPacket(videoCaps);

    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);
    videoPacket.setPts(pts);
    videoPacket.setDuration(1);
    videoPacket.setTimeBase(fps.invert());
    videoPacket.setIndex(0);
    videoPacket.setId(this->m_id);

    auto lineSize = qMin<size_t>(session->bufferStride,
                                 videoPacket.lineSize(0));

    for (int y = 0; y < session->bufferHeight; y++) {
        auto src = reinterpret_cast<quint8 *>(session->shmData) + y * session->bufferStride;
        auto dst = videoPacket.line(0, y);
        memcpy(dst, src, lineSize);
    }

    int intervalMs = qRound(1.e3 * this->m_fps.invert().value());

    if (et.elapsed() >= intervalMs) {
        emit self->oStream(videoPacket);
        et.restart();
    }

    ext_image_copy_capture_frame_v1_destroy(session->currentFrame);
    session->currentFrame = nullptr;
    session->frameReady = false;
    session->frameFailed = false;
    session->hasDamage = false;
    session->frameCaptured = false;
    session->captureAttempts = 0;
}

void WlrootsDevPrivate::readFrameScreencopy(QElapsedTimer &et)
{
    if (!this->m_screencopyManager || this->m_currentOutputId == 0)
        return;

    if (!this->m_outputs.contains(this->m_currentOutputId))
        return;

    auto output = &this->m_outputs[this->m_currentOutputId];

    if (this->m_currentFrame) {
        zwlr_screencopy_frame_v1_destroy(this->m_currentFrame);
        this->m_currentFrame = nullptr;
    }

    this->m_frameReady = false;
    this->m_frameFailed = false;
    this->m_bufferDone = false;
    this->m_hasFrameDamage = false;

    this->m_currentFrame =
            zwlr_screencopy_manager_v1_capture_output(this->m_screencopyManager,
                                                      this->m_showCursor? 1: 0,
                                                      output->output);

    if (!this->m_currentFrame) {
        qWarning() << "Failed to create screencopy frame";

        return;
    }

    zwlr_screencopy_frame_v1_add_listener(this->m_currentFrame,
                                          &screencopyFrameListener,
                                          this);

    wl_display_flush(this->m_display);
    wl_display_roundtrip(this->m_display);

    if (!this->m_bufferDone && !this->m_frameFailed) {
        qWarning() << "Timeout waiting for buffer information";
        zwlr_screencopy_frame_v1_destroy(this->m_currentFrame);
        this->m_currentFrame = nullptr;

        return;
    }

    if (this->m_frameFailed
        || this->m_bufferWidth == 0
        || this->m_bufferHeight == 0)
        return;

    if (!this->createShmBuffer(this->m_bufferWidth,
        this->m_bufferHeight,
        this->m_bufferStride,
        this->m_bufferFormat,
        &this->m_currentBuffer,
        &this->m_shmData,
        &this->m_shmSize)) {
        return;
    }

    zwlr_screencopy_frame_v1_copy(this->m_currentFrame, this->m_currentBuffer);
    wl_display_flush(this->m_display);
    wl_display_roundtrip(this->m_display);

    if (!this->m_frameReady && !this->m_frameFailed) {
        qWarning() << "Timeout waiting for frame ready";

        return;
    }

    if (this->m_frameFailed || !this->m_shmData)
        return;

    this->m_consecutiveFailures = 0;

    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

    AkVideoCaps videoCaps(this->pixelFormat(this->m_bufferFormat),
                          this->m_bufferWidth,
                          this->m_bufferHeight,
                          fps);
    AkVideoPacket videoPacket(videoCaps);

    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
    * fps.value() / 1e3);
    videoPacket.setPts(pts);
    videoPacket.setDuration(1);
    videoPacket.setTimeBase(fps.invert());
    videoPacket.setIndex(0);
    videoPacket.setId(this->m_id);

    auto lineSize = qMin<size_t>(this->m_bufferStride, videoPacket.lineSize(0));

    for (int y = 0; y < this->m_bufferHeight; y++) {
        auto src = reinterpret_cast<quint8 *>(this->m_shmData) + y * this->m_bufferStride;
        auto dst = videoPacket.line(0, y);
        memcpy(dst, src, lineSize);
    }

    int intervalMs = qRound(1.e3 * this->m_fps.invert().value());

    if (et.elapsed() >= intervalMs) {
        emit self->oStream(videoPacket);
        et.restart();
    }
}

bool WlrootsDevPrivate::createShmBuffer(int width, int height, int stride,
                                        uint32_t format,
                                        wl_buffer **outBuffer,
                                        void **outData,
                                        int *outSize)
{
    int size = stride * height;

    if (*outBuffer && *outData && *outSize == size)
        return true;

    this->destroyShmBuffer(outBuffer, outData, outSize);

    auto tmpPathName = QString("wlroots-screencap-%1-%2")
                       .arg(Ak::id())
                       .arg(QTime::currentTime().msec());
    auto tempPath = QDir::temp().filePath(tmpPathName);
    int fd = open(tempPath.toUtf8().constData(), O_CREAT | O_RDWR | O_EXCL, 0600);

    if (fd < 0) {
        qWarning() << "Failed to create SHM temp file:" << strerror(errno);

        return false;
    }

    unlink(tempPath.toUtf8().constData());

    if (ftruncate(fd, size) < 0) {
        qWarning() << "Failed to truncate SHM temp file:" << strerror(errno);
        close(fd);

        return false;
    }

    *outData = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (*outData == MAP_FAILED) {
        qWarning() << "Failed to mmap SHM:" << strerror(errno);
        close(fd);
        *outData = nullptr;

        return false;
    }

    *outSize = size;

    wl_shm_pool *pool = wl_shm_create_pool(this->m_shm, fd, size);
    *outBuffer = wl_shm_pool_create_buffer(pool,
                                           0,
                                           width,
                                           height,
                                           stride,
                                           format);
    wl_shm_pool_destroy(pool);
    close(fd);

    if (!*outBuffer) {
        qWarning() << "Failed to create wl_buffer";
        this->destroyShmBuffer(outBuffer, outData, outSize);

        return false;
    }

    return true;
}

void WlrootsDevPrivate::destroyShmBuffer(wl_buffer **buffer,
                                         void **data,
                                         int *size)
{
    if (*buffer) {
        wl_buffer_destroy(*buffer);
        *buffer = nullptr;
    }

    if (*data) {
        munmap(*data, *size);
        *data = nullptr;
        *size = 0;
    }
}

void WlrootsDevPrivate::registryGlobal(void *data,
                                       wl_registry *registry,
                                       uint32_t name,
                                       const char *interface,
                                       uint32_t version)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);

    if (strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm = reinterpret_cast<wl_shm *>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (strcmp(interface, ext_image_copy_capture_manager_v1_interface.name) == 0) {
        self->m_extCaptureManager = reinterpret_cast<ext_image_copy_capture_manager_v1 *>(
            wl_registry_bind(registry, name,
                             &ext_image_copy_capture_manager_v1_interface,
                             qMin<uint32_t>(version, 1)));
    } else if (strcmp(interface, ext_output_image_capture_source_manager_v1_interface.name) == 0) {
        self->m_extOutputSourceManager = reinterpret_cast<ext_output_image_capture_source_manager_v1 *>(
            wl_registry_bind(registry, name,
                             &ext_output_image_capture_source_manager_v1_interface,
                             qMin<uint32_t>(version, 1)));
    } else if (strcmp(interface, ext_toplevel_image_capture_source_manager_v1_interface.name) == 0) {
        self->m_extToplevelSourceManager = reinterpret_cast<ext_toplevel_image_capture_source_manager_v1 *>(
            wl_registry_bind(registry, name,
                             &ext_toplevel_image_capture_source_manager_v1_interface,
                             qMin<uint32_t>(version, 1)));
    } else if (strcmp(interface, ext_foreign_toplevel_list_v1_interface.name) == 0) {
        self->m_toplevelList = reinterpret_cast<ext_foreign_toplevel_list_v1 *>(
            wl_registry_bind(registry, name,
                             &ext_foreign_toplevel_list_v1_interface,
                             qMin<uint32_t>(version, 1)));
    } else if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        self->m_screencopyManager = reinterpret_cast<zwlr_screencopy_manager_v1 *>(
            wl_registry_bind(registry, name,
                             &zwlr_screencopy_manager_v1_interface,
                             qMin<uint32_t>(version, 3)));
    } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        self->m_xdgOutputManager = reinterpret_cast<zxdg_output_manager_v1 *>(
            wl_registry_bind(registry, name,
                             &zxdg_output_manager_v1_interface,
                             qMin<uint32_t>(version, 3)));
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        OutputInfo output;
        output.output = reinterpret_cast<wl_output *>(
            wl_registry_bind(registry, name, &wl_output_interface,
                             qMin<uint32_t>(version, 4)));
        self->m_outputs.insert(name, output);
    }
}

void WlrootsDevPrivate::registryGlobalRemove(void *data,
                                             wl_registry *registry,
                                             uint32_t name)
{
    Q_UNUSED(registry)
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);

    if (!self->m_outputs.contains(name))
        return;

    auto output = &self->m_outputs[name];

    if (self->m_currentOutputId == name) {
        self->m_currentOutputId = 0;

        if (self->m_run)
            QMetaObject::invokeMethod(self->self,
                                      "uninit",
                                      Qt::QueuedConnection);
    }

    self->destroyExtSession(name);

    if (output->captureSource)
        ext_image_capture_source_v1_destroy(output->captureSource);

    if (output->xdgOutput)
        zxdg_output_v1_destroy(output->xdgOutput);

    if (output->output)
        wl_output_destroy(output->output);

    self->m_outputs.remove(name);
}

void WlrootsDevPrivate::toplevelListToplevel(void *data,
                                             ext_foreign_toplevel_list_v1 *list,
                                             ext_foreign_toplevel_handle_v1 *toplevel)
{
    Q_UNUSED(list)
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);

    ToplevelInfo info;
    info.handle = toplevel;
    ext_foreign_toplevel_handle_v1_add_listener(toplevel,
                                                &toplevelHandleListener,
                                                self);
    self->m_toplevels.insert(QString::number(reinterpret_cast<quintptr>(toplevel)), info);
}

void WlrootsDevPrivate::toplevelListFinished(void *data,
                                             ext_foreign_toplevel_list_v1 *list)
{
    Q_UNUSED(list)
    Q_UNUSED(data)
}

void WlrootsDevPrivate::toplevelHandleClosed(void *data,
                                             ext_foreign_toplevel_handle_v1 *handle)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    QString key;

    for (auto it = self->m_toplevels.begin(); it != self->m_toplevels.end(); ++it)
        if (it.value().handle == handle) {
            key = it.key();

            break;
        }

    if (!key.isEmpty()) {
        if (self->m_currentToplevelId == key) {
            self->m_currentToplevelId.clear();

            if (self->m_run)
                QMetaObject::invokeMethod(self->self,
                                          "uninit",
                                          Qt::QueuedConnection);
        }

        self->destroyExtToplevelSession(key);
        self->m_toplevels.remove(key);
    }
}

void WlrootsDevPrivate::toplevelHandleDone(void *data,
                                           ext_foreign_toplevel_handle_v1 *handle)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
}

void WlrootsDevPrivate::toplevelHandleTitle(void *data,
                                            ext_foreign_toplevel_handle_v1 *handle,
                                            const char *title)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(title)
}

void WlrootsDevPrivate::toplevelHandleAppId(void *data,
                                            ext_foreign_toplevel_handle_v1 *handle,
                                            const char *app_id)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(app_id)
}

void WlrootsDevPrivate::toplevelHandleIdentifier(void *data,
                                                 ext_foreign_toplevel_handle_v1 *handle,
                                                 const char *identifier)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);

    if (!identifier)
        return;

    QString oldKey;
    ToplevelInfo info;

    for (auto it = self->m_toplevels.begin(); it != self->m_toplevels.end(); ++it)
        if (it.value().handle == handle) {
            oldKey = it.key();
            info = it.value();

            break;
        }

    if (!oldKey.isEmpty()) {
        auto newKey = QString::fromUtf8(identifier);

        if (oldKey != newKey) {
            self->m_toplevels.remove(oldKey);
            self->m_toplevels.insert(newKey, info);
        }
    }
}

void WlrootsDevPrivate::extSessionBufferSize(void *data,
                                             ext_image_copy_capture_session_v1 *session,
                                             uint32_t width,
                                             uint32_t height)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(session);

    if (s) {
        s->bufferWidth = static_cast<int>(width);
        s->bufferHeight = static_cast<int>(height);
        s->hasBufferSize = true;
    }
}

void WlrootsDevPrivate::extSessionShmFormat(void *data,
                                            ext_image_copy_capture_session_v1 *session,
                                            uint32_t format)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(session);

    if (s) {
        s->shmFormat = format;
        s->hasShmFormat = true;
    }
}

void WlrootsDevPrivate::extSessionDmabufDevice(void *data,
                                               ext_image_copy_capture_session_v1 *session,
                                               struct wl_array *device)
{
    Q_UNUSED(data)
    Q_UNUSED(session)
    Q_UNUSED(device)
}

void WlrootsDevPrivate::extSessionDmabufFormat(void *data,
                                               ext_image_copy_capture_session_v1 *session,
                                               uint32_t format,
                                               struct wl_array *modifiers)
{
    Q_UNUSED(data)
    Q_UNUSED(session)
    Q_UNUSED(format)
    Q_UNUSED(modifiers)
}

void WlrootsDevPrivate::extSessionDone(void *data,
                                       ext_image_copy_capture_session_v1 *session)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(session);

    if (s)
        s->constraintsDone = true;
}

void WlrootsDevPrivate::extSessionStopped(void *data,
                                          ext_image_copy_capture_session_v1 *session)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(session);

    if (s)
        s->stopped = true;
}

void WlrootsDevPrivate::extFrameTransform(void *data,
                                          ext_image_copy_capture_frame_v1 *frame,
                                          uint32_t transform)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(frame);

    if (s)
        s->transform = transform;
}

void WlrootsDevPrivate::extFrameDamage(void *data,
                                       ext_image_copy_capture_frame_v1 *frame,
                                       int32_t x, int32_t y,
                                       int32_t width, int32_t height)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(frame);

    if (s) {
        s->hasDamage = true;
        s->damageRect = QRect(x, y, width, height);
    }
}

void WlrootsDevPrivate::extFramePresentationTime(void *data,
                                                 ext_image_copy_capture_frame_v1 *frame,
                                                 uint32_t tv_sec_hi,
                                                 uint32_t tv_sec_lo,
                                                 uint32_t tv_nsec)
{
    Q_UNUSED(data)
    Q_UNUSED(frame)
    Q_UNUSED(tv_sec_hi)
    Q_UNUSED(tv_sec_lo)
    Q_UNUSED(tv_nsec)
}

void WlrootsDevPrivate::extFrameReady(void *data,
                                      ext_image_copy_capture_frame_v1 *frame)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(frame);

    if (s)
        s->frameReady = true;
}

void WlrootsDevPrivate::extFrameFailed(void *data,
                                       ext_image_copy_capture_frame_v1 *frame,
                                       uint32_t reason)
{
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    auto s = self->findSession(frame);

    if (s) {
        s->frameFailed = true;
        s->failureReason = reason;
    }
}

void WlrootsDevPrivate::screencopyFrameBuffer(void *data,
                                              zwlr_screencopy_frame_v1 *frame,
                                              uint32_t format,
                                              uint32_t width, uint32_t height,
                                              uint32_t stride)
{
    Q_UNUSED(frame)
    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    self->m_bufferFormat = format;
    self->m_bufferWidth = static_cast<int>(width);
    self->m_bufferHeight = static_cast<int>(height);
    self->m_bufferStride = static_cast<int>(stride);
}

void WlrootsDevPrivate::screencopyFrameFlags(void *data,
                                             zwlr_screencopy_frame_v1 *frame,
                                             uint32_t flags)
{
    Q_UNUSED(data)
    Q_UNUSED(frame)
    Q_UNUSED(flags)
}

void WlrootsDevPrivate::screencopyFrameReady(void *data,
                                             zwlr_screencopy_frame_v1 *frame,
                                             uint32_t tv_sec_hi,
                                             uint32_t tv_sec_lo,
                                             uint32_t tv_nsec)
{
    Q_UNUSED(frame)
    Q_UNUSED(tv_sec_hi)
    Q_UNUSED(tv_sec_lo)
    Q_UNUSED(tv_nsec)

    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    self->m_frameReady = true;
}

void WlrootsDevPrivate::screencopyFrameFailed(void *data,
                                              zwlr_screencopy_frame_v1 *frame)
{
    Q_UNUSED(frame)

    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    self->m_frameFailed = true;
}

void WlrootsDevPrivate::screencopyFrameDamage(void *data,
                                              zwlr_screencopy_frame_v1 *frame,
                                              uint32_t x, uint32_t y,
                                              uint32_t width, uint32_t height)
{
    Q_UNUSED(frame)

    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    self->m_hasFrameDamage = true;
    self->m_damageRect = QRect(static_cast<int>(x),
                               static_cast<int>(y),
                               static_cast<int>(width),
                               static_cast<int>(height));
}

void WlrootsDevPrivate::screencopyFrameLinuxDmabuf(void *data,
                                                   zwlr_screencopy_frame_v1 *frame,
                                                   uint32_t format,
                                                   uint32_t width, uint32_t height)
{
    Q_UNUSED(data)
    Q_UNUSED(frame)
    Q_UNUSED(format)
    Q_UNUSED(width)
    Q_UNUSED(height)
}

void WlrootsDevPrivate::screencopyFrameBufferDone(void *data,
                                                  zwlr_screencopy_frame_v1 *frame)
{
    Q_UNUSED(frame)

    auto self = reinterpret_cast<WlrootsDevPrivate *>(data);
    self->m_bufferDone = true;
}

AkVideoCaps::PixelFormat WlrootsDevPrivate::pixelFormat(uint32_t wlFormat) const
{
    switch (wlFormat) {
    case WL_SHM_FORMAT_XRGB8888:
        return AkVideoCaps::Format_xrgbpack;
    case WL_SHM_FORMAT_ARGB8888:
        return AkVideoCaps::Format_argbpack;
    case WL_SHM_FORMAT_RGB888:
        return AkVideoCaps::Format_rgb24;
    case WL_SHM_FORMAT_BGR888:
        return AkVideoCaps::Format_bgr24;
    case WL_SHM_FORMAT_RGB565:
        return AkVideoCaps::Format_rgb565;
    case WL_SHM_FORMAT_XBGR8888:
        return AkVideoCaps::Format_xbgrpack;
    case WL_SHM_FORMAT_ABGR8888:
        return AkVideoCaps::Format_abgrpack;
    default:
        return AkVideoCaps::Format_none;
    }
}

ExtCaptureSession *WlrootsDevPrivate::findSession(ext_image_copy_capture_session_v1 *session)
{
    for (auto it = this->m_extSessions.begin();
         it != this->m_extSessions.end();
         ++it) {
        if (it.value().session == session)
            return &it.value();
    }

    for (auto it = this->m_extToplevelSessions.begin();
         it != this->m_extToplevelSessions.end();
         ++it) {
        if (it.value().session == session)
            return &it.value();
    }

    return nullptr;
}

ExtCaptureSession *WlrootsDevPrivate::findSession(ext_image_copy_capture_frame_v1 *frame)
{
    for (auto it = this->m_extSessions.begin();
         it != this->m_extSessions.end();
         ++it) {
        if (it.value().currentFrame == frame)
            return &it.value();
    }

    for (auto it = this->m_extToplevelSessions.begin();
         it != this->m_extToplevelSessions.end();
         ++it) {
        if (it.value().currentFrame == frame)
            return &it.value();
    }

    return nullptr;
}

#include "moc_wlrootsdev.cpp"
