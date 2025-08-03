/* Webcamoid, camera capture application.
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

#include <QApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFuture>
#include <QMutex>
#include <QScreen>
#include <QThreadPool>
#include <QTime>
#include <QTimer>
#include <QWindow>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <fcntl.h>
#include <unistd.h>
#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/param/video/format-utils.h>

#include "pipewirescreendev.h"

#define SCREEN_SORCE_TYPE_MONITOR 1U
#define SCREEN_SORCE_TYPE_WINDOW  2U
#define SCREEN_SORCE_TYPE_VIRTUAL 4U

#define CURSOR_MODE_HIDDEN   1U
#define CURSOR_MODE_EMBEDDED 2U
#define CURSOR_MODE_METADATA 4U

#define SESSION_MODE_NOPERSIST                0U
#define SESSION_MODE_PERSIST_UNTIL_APP_CLOSED 1U
#define SESSION_MODE_PERSIST_UNTIL_REVOKED    2U

enum PortalOperation {
    NoOperation,
    CreateSession,
    SelectSources,
    StartStream,
    OpenPipeWireRemote,
};

struct StreamInfo
{
    quint32 nodeId;
    quint32 sourceType;
    QRect rect;
};

#ifdef USE_PIPEWIRE_DYNLOAD
using PwContextConnectFdType = pw_core *(*)(pw_context *context,
                                            int fd,
                                            pw_properties *properties,
                                            size_t userDataSize);
using PwContextDestroyType = void (*)(pw_context *context);
using PwContextNewType = pw_context *(*)(pw_loop *mainLoop,
                                         pw_properties *props,
                                         size_t userDataSize);
using PwDeinitType = void (*)();
using PwInitType = void (*)(int *argc, char **argv[]);
using PwPropertiesNewDictType = pw_properties *(*)(const spa_dict *dict);
using PwStreamAddListenerType = void (*)(pw_stream *stream,
                                         spa_hook *listener,
                                         const pw_stream_events *events,
                                         void *data);
using PwStreamConnectType = int (*)(pw_stream *stream,
                                    pw_direction direction,
                                    uint32_t targetId,
                                    pw_stream_flags flags,
                                    const spa_pod **params,
                                    uint32_t nParams);
using PwStreamDequeueBufferType = pw_buffer *(*)(pw_stream *stream);
using PwStreamDestroyType = void (*)(pw_stream *stream);
using PwStreamDisconnectType = int (*)(pw_stream *stream);
using PwStreamNewType = pw_stream *(*)(pw_core *core,
                                       const char *name,
                                       pw_properties *props);
using PwStreamQueueBufferType = int (*)(pw_stream *stream, pw_buffer *buffer);

using PwThreadLoopDestroyType = void (*)(pw_thread_loop *loop);
using PwThreadLoopGetLoopType = pw_loop *(*)(pw_thread_loop *loop);
using PwThreadLoopLockType = void (*)(pw_thread_loop *loop);
using PwThreadLoopNewType = pw_thread_loop *(*)(const char *name,
                                                const spa_dict *props);
using PwThreadLoopStartType = int (*)(pw_thread_loop *loop);
using PwThreadLoopStopType = void (*)(pw_thread_loop *loop);
using PwThreadLoopUnlockType = void (*)(pw_thread_loop *loop);
using PwThreadLoopWaitType = void (*)(pw_thread_loop *loop);
#endif

class PipewireScreenDevPrivate
{
    public:
        PipewireScreenDev *self;
        QDBusInterface *m_screenCastInterface {nullptr};
        PortalOperation m_currentOp {NoOperation};
        QString m_sessionHandle;
        QVector<StreamInfo> m_streams;
        pw_thread_loop *m_pwStreamLoop {nullptr};
        pw_context *m_pwStreamContext {nullptr};
        pw_core *m_pwStreamCore {nullptr};
        pw_stream *m_pwStream {nullptr};
        spa_hook m_streamHook;
        AkFrac m_fps {30000, 1001};
        bool m_showCursor {false};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        AkPacket m_curPacket;
        AkVideoCaps m_curCaps;
        int m_pipewireFd {-1};
        bool m_run {false};
        bool m_threadedRead {true};

        // PipeWire function pointers

#ifdef USE_PIPEWIRE_DYNLOAD
        QLibrary m_pipeWireLib {"pipewire-0.3"};

        PwContextConnectFdType m_pwContextConnectFd {nullptr};
        PwContextDestroyType m_pwContextDestroy {nullptr};
        PwContextNewType m_pwContextNew {nullptr};
        PwDeinitType m_pwDeinit {nullptr};
        PwInitType m_pwInit {nullptr};
        PwPropertiesNewDictType m_pwPropertiesNewDict {nullptr};
        PwStreamAddListenerType m_pwStreamAddListener {nullptr};
        PwStreamConnectType m_pwStreamConnect {nullptr};
        PwStreamDequeueBufferType m_pwStreamDequeueBuffer {nullptr};
        PwStreamDestroyType m_pwStreamDestroy {nullptr};
        PwStreamDisconnectType m_pwStreamDisconnect {nullptr};
        PwStreamNewType m_pwStreamNew {nullptr};
        PwStreamQueueBufferType m_pwStreamQueueBuffer {nullptr};
        PwThreadLoopDestroyType m_pwThreadLoopDestroy {nullptr};
        PwThreadLoopGetLoopType m_pwThreadLoopGetLoop {nullptr};
        PwThreadLoopLockType m_pwThreadLoopLock {nullptr};
        PwThreadLoopNewType m_pwThreadLoopNew {nullptr};
        PwThreadLoopStartType m_pwThreadLoopStart {nullptr};
        PwThreadLoopStopType m_pwThreadLoopStop {nullptr};
        PwThreadLoopUnlockType m_pwThreadLoopUnlock {nullptr};
        PwThreadLoopWaitType m_pwThreadLoopWait {nullptr};
#endif

        explicit PipewireScreenDevPrivate(PipewireScreenDev *self);
        void sendPacket(const AkPacket &packet);
        inline QString token() const;
        void createSession();
        void selectSources(const QString &sessionHandle);
        void startStream();
        void openPipeWireRemote();
        void updateStreams(const QDBusArgument &streamsInfo);
        void initPipewire(int pipewireFd);
        void uninitPipewire();
        static void streamParamChangedEvent(void *userData,
                                            uint32_t id,
                                            const struct spa_pod *param);
        static void streamProcessEvent(void *userData);

        // PipeWire functions wrappers

        inline pw_core *pwContextConnectFd(pw_context *context,
                                           int fd,
                                           pw_properties *properties,
                                           size_t userDataSize) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwContextConnectFd)
                return this->m_pwContextConnectFd(context,
                                                  fd,
                                                  properties,
                                                  userDataSize);

            return nullptr;
#else
            return pw_context_connect_fd(context, fd, properties, userDataSize);
#endif
        }

        inline void pwContextDestroy(pw_context *context) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwContextDestroy)
                this->m_pwContextDestroy(context);
#else
            pw_context_destroy(context);
#endif
        }

        inline pw_context *pwContextNew(pw_loop *mainLoop,
                                        pw_properties *props,
                                        size_t userDataSize) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwContextNew)
                return this->m_pwContextNew(mainLoop,
                                            props,
                                            userDataSize);

            return nullptr;
#else
            return pw_context_new(mainLoop, props, userDataSize);
#endif
        }


        inline void pwDeinit() const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwDeinit)
                this->m_pwDeinit();
#else
            pw_deinit();
#endif
        }

        inline void pwInit(int *argc, char **argv[]) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwInit)
                this->m_pwInit(argc, argv);
#else
            pw_init(argc, argv);
#endif
        }

        inline pw_properties *pwPropertiesNewDict(const spa_dict *dict) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwPropertiesNewDict)
                return this->m_pwPropertiesNewDict(dict);

            return nullptr;
#else
            return pw_properties_new_dict(dict);
#endif
        }

        inline void pwStreamAddListener(pw_stream *stream,
                                        spa_hook *listener,
                                        const pw_stream_events *events,
                                        void *data) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamAddListener)
                this->m_pwStreamAddListener(stream,
                                            listener,
                                            events,
                                            data);
#else
            pw_stream_add_listener(stream, listener, events, data);
#endif
        }

        inline int pwStreamConnect(pw_stream *stream,
                                   pw_direction direction,
                                   uint32_t targetId,
                                   pw_stream_flags flags,
                                   const spa_pod **params,
                                   uint32_t nParams) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamConnect)
                return this->m_pwStreamConnect(stream,
                                               direction,
                                               targetId,
                                               flags,
                                               params,
                                               nParams);

            return 0;
#else
            return pw_stream_connect(stream,
                                     direction,
                                     targetId,
                                     flags,
                                     params,
                                     nParams);
#endif
        }

        inline pw_buffer *pwStreamDequeueBuffer(pw_stream *stream) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamDequeueBuffer)
                return this->m_pwStreamDequeueBuffer(stream);

            return nullptr;
#else
            return pw_stream_dequeue_buffer(stream);
#endif
        }

        inline void pwStreamDestroy(pw_stream *stream) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamDestroy)
                this->m_pwStreamDestroy(stream);
#else
            pw_stream_destroy(stream);
#endif
        }

        inline int pwStreamDisconnect(pw_stream *stream) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamDisconnect)
                return this->m_pwStreamDisconnect(stream);

            return 0;
#else
            return pw_stream_disconnect(stream);
#endif
        }

        inline pw_stream *pwStreamNew(pw_core *core,
                                      const char *name,
                                      pw_properties *props) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamNew)
                return this->m_pwStreamNew(core, name, props);

            return nullptr;
#else
            return pw_stream_new(core, name, props);
#endif
        }

        inline int pwStreamQueueBuffer(pw_stream *stream,
                                       pw_buffer *buffer) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwStreamQueueBuffer)
                return this->m_pwStreamQueueBuffer(stream, buffer);

            return 0;
#else
            return pw_stream_queue_buffer(stream, buffer);
#endif
        }

        inline void pwThreadLoopDestroy(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopDestroy)
                this->m_pwThreadLoopDestroy(loop);
#else
            pw_thread_loop_destroy(loop);
#endif
        }

        inline pw_loop *pwThreadLoopGetLoop(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopGetLoop)
                return this->m_pwThreadLoopGetLoop(loop);

            return nullptr;
#else
            return pw_thread_loop_get_loop(loop);
#endif
        }

        inline void pwThreadLoopLock(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopLock)
                this->m_pwThreadLoopLock(loop);
#else
            pw_thread_loop_lock(loop);
#endif
        }

        inline pw_thread_loop *pwThreadLoopNew(const char *name,
                                               const spa_dict *props) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopNew)
                return this->m_pwThreadLoopNew(name, props);

            return nullptr;
#else
            return pw_thread_loop_new(name, props);
#endif
        }

        inline int pwThreadLoopStart(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopStart)
                return this->m_pwThreadLoopStart(loop);

            return 0;
#else
            return pw_thread_loop_start(loop);
#endif
        }

        inline void pwThreadLoopStop(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopStop)
                this->m_pwThreadLoopStop(loop);
#else
            pw_thread_loop_stop(loop);
#endif
        }

        inline void pwThreadLoopUnlock(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopUnlock)
                this->m_pwThreadLoopUnlock(loop);
#else
            pw_thread_loop_unlock(loop);
#endif
        }

        inline void pwThreadLoopWait(pw_thread_loop *loop) const
        {
#ifdef USE_PIPEWIRE_DYNLOAD
            if (this->m_pwThreadLoopWait)
                this->m_pwThreadLoopWait(loop);
#else
            pw_thread_loop_wait(loop);
#endif
        }
};

static const struct pw_stream_events pipewireDesktopStreamEvents = {
    .version       = PW_VERSION_STREAM_EVENTS                         ,
    .destroy       = nullptr                                          ,
    .state_changed = nullptr                                          ,
    .control_info  = nullptr                                          ,
    .io_changed    = nullptr                                          ,
    .param_changed = PipewireScreenDevPrivate::streamParamChangedEvent,
    .add_buffer    = nullptr                                          ,
    .remove_buffer = nullptr                                          ,
    .process       = PipewireScreenDevPrivate::streamProcessEvent     ,
};

PipewireScreenDev::PipewireScreenDev():
    ScreenDev()
{
    this->d = new PipewireScreenDevPrivate(this);
    size_t i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         this,
                         [=]() { this->srceenResized(int(i)); });
        i++;
    }

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &PipewireScreenDev::screenAdded);
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &PipewireScreenDev::screenRemoved);

    this->d->pwInit(nullptr, nullptr);
}

PipewireScreenDev::~PipewireScreenDev()
{
    this->uninit();
    this->d->pwDeinit();
    delete this->d;
}

AkFrac PipewireScreenDev::fps() const
{
    return this->d->m_fps;
}

QStringList PipewireScreenDev::medias()
{
    return {"screen://pipewire"};
}

QString PipewireScreenDev::media() const
{
    return {"screen://pipewire"};
}

QList<int> PipewireScreenDev::streams() const
{
    return {0};
}

int PipewireScreenDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString PipewireScreenDev::description(const QString &media)
{
    if (media != "screen://pipewire")
        return {};

    return {tr("PipeWire Screen")};
}

AkVideoCaps PipewireScreenDev::caps(int stream)
{
    if (stream != 0)
        return {};

    auto screen = QGuiApplication::primaryScreen();

    if (!screen)
        return {};

    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       screen->size().width(),
                       screen->size().height(),
                       this->d->m_fps);
}

bool PipewireScreenDev::canCaptureCursor() const
{
    return true;
}

bool PipewireScreenDev::canChangeCursorSize() const
{
    return false;
}

bool PipewireScreenDev::showCursor() const
{
    return this->d->m_showCursor;
}

int PipewireScreenDev::cursorSize() const
{
    return 0;
}

void PipewireScreenDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);
}

void PipewireScreenDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void PipewireScreenDev::setMedia(const QString &media)
{
    Q_UNUSED(media)
}

void PipewireScreenDev::setShowCursor(bool showCursor)
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

void PipewireScreenDev::setCursorSize(int cursorSize)
{
    Q_UNUSED(cursorSize)
}

void PipewireScreenDev::resetMedia()
{
}

void PipewireScreenDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void PipewireScreenDev::resetStreams()
{

}

void PipewireScreenDev::resetShowCursor()
{
    this->setShowCursor(false);
}

void PipewireScreenDev::resetCursorSize()
{

}

bool PipewireScreenDev::init()
{
    this->d->m_id = Ak::id();
    this->d->m_screenCastInterface =
            new QDBusInterface("org.freedesktop.portal.Desktop",
                               "/org/freedesktop/portal/desktop",
                               "org.freedesktop.portal.ScreenCast",
                               QDBusConnection::sessionBus());
    this->d->m_screenCastInterface->connection()
            .connect("org.freedesktop.portal.Desktop",
                     "",
                     "org.freedesktop.portal.Request",
                     "Response",
                     this,
                     SLOT(responseReceived(quint32,QVariantMap)));

#if 0
    auto availableSourceTypes = this->d->m_screenCastInterface->property("AvailableSourceTypes").toUInt();
    auto availableCursorModes = this->d->m_screenCastInterface->property("AvailableCursorModes").toUInt();

    qDebug() << "AvailableSourceTypes" << availableSourceTypes;
    qDebug() << "AvailableCursorModes" << availableCursorModes;
#endif

    this->d->createSession();

    return true;
}

bool PipewireScreenDev::uninit()
{
    this->d->m_threadStatus.waitForFinished();

    this->d->uninitPipewire();

    if (this->d->m_screenCastInterface) {
        delete this->d->m_screenCastInterface;
        this->d->m_screenCastInterface = nullptr;
    }

    return true;
}

void PipewireScreenDev::screenAdded(QScreen *screen)
{
    Q_UNUSED(screen)
    size_t i = 0;

    for (auto &screen_: QGuiApplication::screens()) {
        if (screen_ == screen)
            QObject::connect(screen_,
                             &QScreen::geometryChanged,
                             this,
                             [=]() { this->srceenResized(int(i)); });

        i++;
    }

    emit this->mediasChanged(this->medias());
}

void PipewireScreenDev::screenRemoved(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void PipewireScreenDev::srceenResized(int screen)
{
    auto screens = QGuiApplication::screens();

    if (screen < 0 || screen >= screens.size())
        return;

    auto widget = screens[screen];

    if (!widget)
        return;

    emit this->sizeChanged("screen://pipewire", widget->size());
}

void PipewireScreenDev::responseReceived(quint32 response,
                                         const QVariantMap &results)
{
    if (response != 0) {
        static const QMap<PortalOperation, QString> opStr {
            {NoOperation       , "NoOperation"       },
            {CreateSession     , "CreateSession"     },
            {SelectSources     , "SelectSources"     },
            {StartStream       , "StartStream"       },
            {OpenPipeWireRemote, "OpenPipeWireRemote"},
        };

        qDebug() << "Operation"
                 << opStr[this->d->m_currentOp]
                 << "failed with result"
                 << response;
        this->d->m_currentOp = NoOperation;

        return;
    }

    switch (this->d->m_currentOp) {
    case CreateSession:
        this->d->selectSources(results["session_handle"].toString());
        break;

    case SelectSources:
        this->d->startStream();
        break;

    case StartStream:
        this->d->updateStreams(results["streams"].value<QDBusArgument>());
        this->d->openPipeWireRemote();
        this->d->m_currentOp = NoOperation;
        break;

    case OpenPipeWireRemote:
        this->d->m_currentOp = NoOperation;
        break;

    default:
        break;
    }
}

PipewireScreenDevPrivate::PipewireScreenDevPrivate(PipewireScreenDev *self):
    self(self)
{
#ifdef USE_PIPEWIRE_DYNLOAD
        if (this->m_pipeWireLib.load()) {
            this->m_pwContextConnectFd = reinterpret_cast<PwContextConnectFdType>(this->m_pipeWireLib.resolve("pw_context_connect_fd"));
            this->m_pwContextDestroy = reinterpret_cast<PwContextDestroyType>(this->m_pipeWireLib.resolve("pw_context_destroy"));
            this->m_pwContextNew = reinterpret_cast<PwContextNewType>(this->m_pipeWireLib.resolve("pw_context_new"));
            this->m_pwDeinit = reinterpret_cast<PwDeinitType>(this->m_pipeWireLib.resolve("pw_deinit"));
            this->m_pwInit = reinterpret_cast<PwInitType>(this->m_pipeWireLib.resolve("pw_init"));
            this->m_pwPropertiesNewDict = reinterpret_cast<PwPropertiesNewDictType>(this->m_pipeWireLib.resolve("pw_properties_new_dict"));
            this->m_pwStreamAddListener = reinterpret_cast<PwStreamAddListenerType>(this->m_pipeWireLib.resolve("pw_stream_add_listener"));
            this->m_pwStreamConnect = reinterpret_cast<PwStreamConnectType>(this->m_pipeWireLib.resolve("pw_stream_connect"));
            this->m_pwStreamDequeueBuffer = reinterpret_cast<PwStreamDequeueBufferType>(this->m_pipeWireLib.resolve("pw_stream_dequeue_buffer"));
            this->m_pwStreamDestroy = reinterpret_cast<PwStreamDestroyType>(this->m_pipeWireLib.resolve("pw_stream_destroy"));
            this->m_pwStreamDisconnect = reinterpret_cast<PwStreamDisconnectType>(this->m_pipeWireLib.resolve("pw_stream_disconnect"));
            this->m_pwStreamNew = reinterpret_cast<PwStreamNewType>(this->m_pipeWireLib.resolve("pw_stream_new"));
            this->m_pwStreamQueueBuffer = reinterpret_cast<PwStreamQueueBufferType>(this->m_pipeWireLib.resolve("pw_stream_queue_buffer"));
            this->m_pwThreadLoopDestroy = reinterpret_cast<PwThreadLoopDestroyType>(this->m_pipeWireLib.resolve("pw_thread_loop_destroy"));
            this->m_pwThreadLoopGetLoop = reinterpret_cast<PwThreadLoopGetLoopType>(this->m_pipeWireLib.resolve("pw_thread_loop_get_loop"));
            this->m_pwThreadLoopLock = reinterpret_cast<PwThreadLoopLockType>(this->m_pipeWireLib.resolve("pw_thread_loop_lock"));
            this->m_pwThreadLoopNew = reinterpret_cast<PwThreadLoopNewType>(this->m_pipeWireLib.resolve("pw_thread_loop_new"));
            this->m_pwThreadLoopStart = reinterpret_cast<PwThreadLoopStartType>(this->m_pipeWireLib.resolve("pw_thread_loop_start"));
            this->m_pwThreadLoopStop = reinterpret_cast<PwThreadLoopStopType>(this->m_pipeWireLib.resolve("pw_thread_loop_stop"));
            this->m_pwThreadLoopUnlock = reinterpret_cast<PwThreadLoopUnlockType>(this->m_pipeWireLib.resolve("pw_thread_loop_unlock"));
            this->m_pwThreadLoopWait = reinterpret_cast<PwThreadLoopWaitType>(this->m_pipeWireLib.resolve("pw_thread_loop_wait"));
        }
#endif
}

void PipewireScreenDevPrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
}

QString PipewireScreenDevPrivate::token() const
{
    return QString("u%1").arg(Ak::id());
}

void PipewireScreenDevPrivate::createSession()
{
    qInfo() << "Creating screen cast session";

    this->m_currentOp = CreateSession;
    QVariantMap options {
        {"handle_token"        , this->token()},
        {"session_handle_token", this->token()},
    };
    QDBusMessage reply =
            this->m_screenCastInterface->call("CreateSession", options);

    if (!reply.errorMessage().isEmpty())
        qInfo() << "Error:" << reply.errorName() << ":" << reply.errorMessage();
}

void PipewireScreenDevPrivate::selectSources(const QString &sessionHandle)
{
    qInfo() << "Selecting sources";

    this->m_sessionHandle = sessionHandle;
    this->m_currentOp = SelectSources;
    QVariantMap options = {
        {"handle_token", this->token()              },
        {"types"       , SCREEN_SORCE_TYPE_MONITOR
                         | SCREEN_SORCE_TYPE_WINDOW
                         | SCREEN_SORCE_TYPE_VIRTUAL},
        {"multiple"    , false                      },
        {"cursor_mode" , this->m_showCursor?
                            CURSOR_MODE_EMBEDDED:
                            CURSOR_MODE_HIDDEN      },
        {"persist_mode", SESSION_MODE_NOPERSIST     },
    };
    QDBusMessage reply =
            this->m_screenCastInterface->call("SelectSources",
                                              QDBusObjectPath(sessionHandle),
                                              options);

    if (!reply.errorMessage().isEmpty())
        qInfo() << "Error:" << reply.errorName() << ":" << reply.errorMessage();
}

void PipewireScreenDevPrivate::startStream()
{
    qInfo() << "Starting stream";

    this->m_currentOp = StartStream;
    QVariantMap options = {
        {"handle_token", this->token()},
    };
    QDBusMessage reply =
            this->m_screenCastInterface->call("Start",
                                              QDBusObjectPath(this->m_sessionHandle),
                                              "",
                                              options);

    if (!reply.errorMessage().isEmpty())
        qInfo() << "Error:" << reply.errorName() << ":" << reply.errorMessage();
}

void PipewireScreenDevPrivate::openPipeWireRemote()
{
    qInfo() << "Open PipeWire remote file descriptor";

    this->m_currentOp = OpenPipeWireRemote;
    QVariantMap options;
    QDBusReply<QDBusUnixFileDescriptor> reply =
            this->m_screenCastInterface->call("OpenPipeWireRemote",
                                              QDBusObjectPath(this->m_sessionHandle),
                                              options);

    if (!reply.isValid()) {
        qInfo() << "Error" << reply.error();

        return;
    }

    this->m_pipewireFd = reply.value().fileDescriptor();
    this->initPipewire(this->m_pipewireFd);
}

void PipewireScreenDevPrivate::updateStreams(const QDBusArgument &streamsInfo)
{
    this->m_streams.clear();

    streamsInfo.beginStructure();
    streamsInfo.beginArray();

    while (!streamsInfo.atEnd()) {
        quint32 nodeId = 0;
        streamsInfo >> nodeId;
        QVariantMap properties;
        streamsInfo >> properties;

        auto position = properties["position"].value<QDBusArgument>();
        position.beginStructure();
        position.beginArray();
        qint32 x = 0;
        position >> x;
        qint32 y = 0;
        position >> y;
        position.endArray();
        position.endStructure();

        auto size = properties["size"].value<QDBusArgument>();
        size.beginStructure();
        size.beginArray();
        qint32 width = 0;
        size >> width;
        qint32 height = 0;
        size >> height;
        size.endArray();
        size.endStructure();

        auto sourceType = properties["source_type"].toUInt();

        StreamInfo streamInfo;
        streamInfo.nodeId = nodeId;
        streamInfo.sourceType = sourceType;
        streamInfo.rect = {x, y, width, height};
        this->m_streams << streamInfo;
    }

    streamsInfo.endArray();
    streamsInfo.endStructure();
}

void PipewireScreenDevPrivate::initPipewire(int pipewireFd)
{
    if (this->m_streams.isEmpty()) {
        this->uninitPipewire();
        qInfo() << "Screen information is empty";

        return;
    }

    auto streamInfo = this->m_streams[0];
    this->m_pwStreamLoop =
        this->pwThreadLoopNew("PipeWire desktop capture thread loop", nullptr);

    if (!this->m_pwStreamLoop) {
        this->uninitPipewire();
        qInfo() << "Error creating PipeWire desktop capture thread loop";

        return;
    }

    this->m_pwStreamContext =
            this->pwContextNew(this->pwThreadLoopGetLoop(this->m_pwStreamLoop),
                               nullptr,
                               0);

    if (!this->m_pwStreamContext) {
        this->uninitPipewire();
        qInfo() << "Error creating PipeWire context";

        return;
    }

    if (this->pwThreadLoopStart(this->m_pwStreamLoop) < 0) {
        this->uninitPipewire();
        qInfo() << "Error starting PipeWire main loop";

        return;
    }

    this->pwThreadLoopLock(this->m_pwStreamLoop);

    this->m_pwStreamCore =
        this->pwContextConnectFd(this->m_pwStreamContext,
                                 fcntl(pipewireFd,
                                       F_DUPFD_CLOEXEC,
                                       5),
                                 nullptr,
                                 0);

    if (!this->m_pwStreamCore) {
        this->pwThreadLoopUnlock(this->m_pwStreamLoop);
        this->uninitPipewire();
        qInfo() << "Error connecting to the PipeWire file descriptor:" << strerror(errno);

        return;
    }

    spa_dict_item items[] = {
        {PW_KEY_MEDIA_TYPE, "Video"},
        {PW_KEY_MEDIA_CATEGORY, "Capture"},
        {PW_KEY_MEDIA_ROLE, "Screen"},
    };

    spa_dict dict = {SPA_DICT_FLAG_SORTED,
                     sizeof(items) / sizeof(items[0]),
                     items};

    this->m_pwStream =
            this->pwStreamNew(this->m_pwStreamCore,
                              "Webcamoid Screen Capture",
                              this->pwPropertiesNewDict(&dict));
    this->pwStreamAddListener(this->m_pwStream,
                              &this->m_streamHook,
                              &pipewireDesktopStreamEvents,
                              this);

    QVector<const spa_pod *> params;
    quint8 buffer[4096];
    auto podBuilder = SPA_POD_BUILDER_INIT(buffer, 4096);

    auto defFrameSize = SPA_RECTANGLE(quint32(streamInfo.rect.width()),
                                      quint32(streamInfo.rect.height()));
    auto minFrameSize = SPA_RECTANGLE(1, 1);
    auto maxFrameSize = SPA_RECTANGLE(8192, 4320);

    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

    auto defFps = SPA_FRACTION(quint32(fps.num()), quint32(fps.den()));
    auto minFps = SPA_FRACTION(0, 1);
    auto maxFps = SPA_FRACTION(1000, 1);

    params << reinterpret_cast<const spa_pod *>(
                  spa_pod_builder_add_object(&podBuilder,
                                             SPA_TYPE_OBJECT_Format ,
                                                 SPA_PARAM_EnumFormat,
                                             SPA_FORMAT_mediaType,
                                                 SPA_POD_Id(SPA_MEDIA_TYPE_video),
                                             SPA_FORMAT_mediaSubtype,
                                                 SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                             SPA_FORMAT_VIDEO_format,
                                                 SPA_POD_CHOICE_ENUM_Id(6,
                                                                        SPA_VIDEO_FORMAT_RGB,
                                                                        SPA_VIDEO_FORMAT_BGR,
                                                                        SPA_VIDEO_FORMAT_RGBA,
                                                                        SPA_VIDEO_FORMAT_BGRA,
                                                                        SPA_VIDEO_FORMAT_RGBx,
                                                                        SPA_VIDEO_FORMAT_BGRx),
                                             SPA_FORMAT_VIDEO_size,
                                                 SPA_POD_CHOICE_RANGE_Rectangle(&defFrameSize,
                                                                                &minFrameSize,
                                                                                &maxFrameSize),
                                             SPA_FORMAT_VIDEO_framerate,
                                                 SPA_POD_CHOICE_RANGE_Fraction(&defFps,
                                                                               &minFps,
                                                                               &maxFps)));

    this->pwStreamConnect(this->m_pwStream,
                          PW_DIRECTION_INPUT,
                          streamInfo.nodeId,
                          pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT
                                          | PW_STREAM_FLAG_MAP_BUFFERS),
                          params.data(),
                          params.size());
    this->pwThreadLoopUnlock(this->m_pwStreamLoop);
    this->m_run = true;
}

void PipewireScreenDevPrivate::uninitPipewire()
{
    this->m_run = false;

    if (this->m_pwStreamLoop) {
        this->pwThreadLoopWait(this->m_pwStreamLoop);
        this->pwThreadLoopStop(this->m_pwStreamLoop);
    }

    if (this->m_pwStream) {
        this->pwStreamDisconnect(this->m_pwStream);
        this->pwStreamDestroy(this->m_pwStream);
        this->m_pwStream = nullptr;
    }

    if (this->m_pwStreamContext) {
        this->pwContextDestroy(this->m_pwStreamContext);
        this->m_pwStreamContext = nullptr;
    }

    if (this->m_pwStreamLoop) {
        this->pwThreadLoopDestroy(this->m_pwStreamLoop);
        this->m_pwStreamLoop = nullptr;
    }

    if (this->m_pipewireFd > 0) {
        close(this->m_pipewireFd);
        this->m_pipewireFd = -1;
    }
}

void PipewireScreenDevPrivate::streamParamChangedEvent(void *userData,
                                                       uint32_t id,
                                                       const spa_pod *param)
{
    qInfo() << "Stream parameters changed";
    auto self = reinterpret_cast<PipewireScreenDevPrivate *>(userData);

    if (!param || id != SPA_PARAM_Format)
        return;

    uint32_t mediaType = 0;
    uint32_t mediaSubtype = 0;

    if (spa_format_parse(param,
                         &mediaType,
                         &mediaSubtype) < 0)
        return;

    if (mediaType != SPA_MEDIA_TYPE_video ||
        mediaSubtype != SPA_MEDIA_SUBTYPE_raw)
        return;

    spa_video_info_raw videoInfo;
    memset(&videoInfo, 0, sizeof(spa_video_info_raw));

    if (spa_format_video_raw_parse(param, &videoInfo) < 0)
        return;

    static const QMap<spa_video_format, AkVideoCaps::PixelFormat> spaFmtToAk {
        {SPA_VIDEO_FORMAT_RGB , AkVideoCaps::Format_bgr24   },
        {SPA_VIDEO_FORMAT_BGR , AkVideoCaps::Format_rgb24   },
        {SPA_VIDEO_FORMAT_RGBA, AkVideoCaps::Format_abgrpack},
        {SPA_VIDEO_FORMAT_BGRA, AkVideoCaps::Format_argbpack},
        {SPA_VIDEO_FORMAT_RGBx, AkVideoCaps::Format_xbgrpack},
        {SPA_VIDEO_FORMAT_BGRx, AkVideoCaps::Format_xrgbpack},
    };

    if (spaFmtToAk.contains(videoInfo.format)) {
        AkFrac fps(videoInfo.framerate.num, videoInfo.framerate.denom);

        if (qCeil(fps.value()) < 1) {
            self->m_mutex.lock();
            fps = self->m_fps;
            self->m_mutex.unlock();
        }

        self->m_curCaps = {spaFmtToAk[videoInfo.format],
                           int(videoInfo.size.width),
                           int(videoInfo.size.height),
                           fps};
    } else {
        self->m_curCaps = AkVideoCaps();
    }

    qInfo() << "Stream format:" << self->m_curCaps;
}

void PipewireScreenDevPrivate::streamProcessEvent(void *userData)
{
    auto self = reinterpret_cast<PipewireScreenDevPrivate *>(userData);
    auto buffer = self->pwStreamDequeueBuffer(self->m_pwStream);

    if (!buffer)
        return;

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
    packet.setDuration(1);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(self->m_id);

    if (!self->m_threadedRead) {
        emit self->self->oStream(packet);

        return;
    }

    if (!self->m_threadStatus.isRunning()) {
        self->m_curPacket = packet;

        self->m_threadStatus =
                QtConcurrent::run(&self->m_threadPool,
                                  &PipewireScreenDevPrivate::sendPacket,
                                  self,
                                  self->m_curPacket);
    }

    self->pwStreamQueueBuffer(self->m_pwStream, buffer);
}

#include "moc_pipewirescreendev.cpp"
