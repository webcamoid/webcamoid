/* Webcamoid, webcam capture application.
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

class PipewireScreenDevPrivate
{
    public:
        PipewireScreenDev *self;
        QDBusInterface *m_screenCastInterface {nullptr};
        PortalOperation m_currentOp {NoOperation};
        QString m_sessionHandle;
        QVector<StreamInfo> m_streams;
        pw_thread_loop *m_pwThreadLoop {nullptr};
        pw_context *m_pwContext {nullptr};
        pw_core *m_pwCore {nullptr};
        pw_stream *m_pwStream {nullptr};
        spa_hook m_streamListener;
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        AkPacket m_curPacket;
        AkVideoCaps m_curCaps;
        int m_pipewireFd {-1};
        bool m_threadedRead {true};

        explicit PipewireScreenDevPrivate(PipewireScreenDev *self);
        void sendPacket(const AkPacket &packet);
        inline QString token() const;
        void createSession();
        void selectSources(const QString &sessionHandle);
        void startStream();
        void openPipeWireRemote();
        void updateStreams(const QDBusArgument &streamsInfo);
        void initPipewire();
        void uninitPipewire();
        static void streamParamChangedEvent(void *userData,
                                            uint32_t id,
                                            const struct spa_pod *param);
        static void streamProcessEvent(void *userData);
};

static const struct pw_stream_events pipewireStreamEvents = {
    .version       = PW_VERSION_STREAM_EVENTS                         ,
    .destroy       = nullptr,
    .state_changed = nullptr,
    .control_info  = nullptr,
    .io_changed    = nullptr,
    .param_changed = PipewireScreenDevPrivate::streamParamChangedEvent,
    .add_buffer    = nullptr,
    .remove_buffer = nullptr,
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
}

PipewireScreenDev::~PipewireScreenDev()
{
    this->uninit();
    pw_deinit();
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
        {"cursor_mode" , CURSOR_MODE_HIDDEN         },
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
    this->initPipewire();
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

void PipewireScreenDevPrivate::initPipewire()
{
    if (this->m_streams.isEmpty()) {
        this->uninitPipewire();
        qInfo() << "Screams information is empty";

        return;
    }

    auto streamInfo = this->m_streams[0];

    this->m_pwThreadLoop = pw_thread_loop_new("PipeWire thread loop", nullptr);

    if (!this->m_pwThreadLoop) {
        this->uninitPipewire();
        qInfo() << "Error creating PipeWire thread loop";

        return;
    }

    this->m_pwContext =
            pw_context_new(pw_thread_loop_get_loop(this->m_pwThreadLoop),
                           nullptr,
                           0);

    if (!this->m_pwContext) {
        this->uninitPipewire();
        qInfo() << "Error creating PipeWire context";

        return;
    }

    if (pw_thread_loop_start(this->m_pwThreadLoop) < 0) {
        this->uninitPipewire();
        qInfo() << "Error starting PipeWire main loop";

        return;
    }

    pw_thread_loop_lock(this->m_pwThreadLoop);

    this->m_pwCore = pw_context_connect_fd(this->m_pwContext,
                                           fcntl(this->m_pipewireFd,
                                                 F_DUPFD_CLOEXEC,
                                                 5),
                                           nullptr,
                                           0);

    if (!this->m_pwCore) {
        pw_thread_loop_unlock(this->m_pwThreadLoop);
        this->uninitPipewire();
        qInfo() << "Error connecting to the PipeWire file descriptor:" << strerror(errno);

        return;
    }

    this->m_pwStream =
            pw_stream_new(this->m_pwCore,
                          "Webcamoid Screen Capture",
                          pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                                            PW_KEY_MEDIA_CATEGORY, "Capture",
                                            PW_KEY_MEDIA_ROLE, "Screen",
                                            nullptr));
    pw_stream_add_listener(this->m_pwStream,
                           &this->m_streamListener,
                           &pipewireStreamEvents,
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
                                             SPA_TYPE_OBJECT_Format    , SPA_PARAM_EnumFormat,
                                             SPA_FORMAT_mediaType      , SPA_POD_Id(SPA_MEDIA_TYPE_video),
                                             SPA_FORMAT_mediaSubtype   , SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                             SPA_FORMAT_VIDEO_format   , SPA_POD_CHOICE_ENUM_Id(6,
                                                                                                SPA_VIDEO_FORMAT_RGB,
                                                                                                SPA_VIDEO_FORMAT_BGR,
                                                                                                SPA_VIDEO_FORMAT_RGBA,
                                                                                                SPA_VIDEO_FORMAT_BGRA,
                                                                                                SPA_VIDEO_FORMAT_RGBx,
                                                                                                SPA_VIDEO_FORMAT_BGRx),
                                             SPA_FORMAT_VIDEO_size     , SPA_POD_CHOICE_RANGE_Rectangle(&defFrameSize,
                                                                                                        &minFrameSize,
                                                                                                        &maxFrameSize),
                                             SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defFps,
                                                                                                       &minFps,
                                                                                                       &maxFps)));

    pw_stream_connect(this->m_pwStream,
                      PW_DIRECTION_INPUT,
                      streamInfo.nodeId,
                      pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT
                                      | PW_STREAM_FLAG_MAP_BUFFERS),
                      params.data(),
                      params.size());
    pw_thread_loop_unlock(this->m_pwThreadLoop);
}

void PipewireScreenDevPrivate::uninitPipewire()
{
    if (this->m_pwThreadLoop) {
        pw_thread_loop_wait(this->m_pwThreadLoop);
        pw_thread_loop_stop(this->m_pwThreadLoop);
    }

    if (this->m_pwStream) {
        pw_stream_disconnect(this->m_pwStream);
        pw_stream_destroy(this->m_pwStream);
        this->m_pwStream = nullptr;
    }

    if (this->m_pwContext) {
        pw_context_destroy(this->m_pwContext);
        this->m_pwContext = nullptr;
    }

    if (this->m_pwThreadLoop) {
        pw_thread_loop_destroy(this->m_pwThreadLoop);
        this->m_pwThreadLoop = nullptr;
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
        {SPA_VIDEO_FORMAT_RGB , AkVideoCaps::Format_bgr24},
        {SPA_VIDEO_FORMAT_BGR , AkVideoCaps::Format_rgb24},
        {SPA_VIDEO_FORMAT_RGBA, AkVideoCaps::Format_abgrpack},
        {SPA_VIDEO_FORMAT_BGRA, AkVideoCaps::Format_argbpack},
        {SPA_VIDEO_FORMAT_RGBx, AkVideoCaps::Format_0bgrpack},
        {SPA_VIDEO_FORMAT_BGRx, AkVideoCaps::Format_0rgbpack},
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
    auto buffer = pw_stream_dequeue_buffer(self->m_pwStream);

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
                                  self,
                                  &PipewireScreenDevPrivate::sendPacket,
                                  self->m_curPacket);
    }

    pw_stream_queue_buffer(self->m_pwStream, buffer);
}

#include "moc_pipewirescreendev.cpp"
