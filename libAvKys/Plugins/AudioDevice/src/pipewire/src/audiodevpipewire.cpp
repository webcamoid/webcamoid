/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include <QCoreApplication>
#include <QDir>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QtConcurrent>
#include <QtDebug>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/type-info.h>
#include <spa/utils/result.h>

#include "audiodevpipewire.h"

class SampleFormat
{
    public:
        spa_audio_format pwFormat;
        AkAudioCaps::SampleFormat format;
        bool planar;

        static const SampleFormat *table()
        {
            static const SampleFormat pipewireAudioSampleFormatsTable[] = {
                {SPA_AUDIO_FORMAT_S8     , AkAudioCaps::SampleFormat_s8   , false},
                {SPA_AUDIO_FORMAT_U8     , AkAudioCaps::SampleFormat_u8   , false},
                {SPA_AUDIO_FORMAT_S16_LE , AkAudioCaps::SampleFormat_s16le, false},
                {SPA_AUDIO_FORMAT_S16_BE , AkAudioCaps::SampleFormat_s16be, false},
                {SPA_AUDIO_FORMAT_U16_LE , AkAudioCaps::SampleFormat_u16le, false},
                {SPA_AUDIO_FORMAT_U16_BE , AkAudioCaps::SampleFormat_u16be, false},
                {SPA_AUDIO_FORMAT_S32_LE , AkAudioCaps::SampleFormat_s32le, false},
                {SPA_AUDIO_FORMAT_S32_BE , AkAudioCaps::SampleFormat_s32be, false},
                {SPA_AUDIO_FORMAT_U32_LE , AkAudioCaps::SampleFormat_u32le, false},
                {SPA_AUDIO_FORMAT_U32_BE , AkAudioCaps::SampleFormat_u32be, false},
                {SPA_AUDIO_FORMAT_F32_LE , AkAudioCaps::SampleFormat_fltle, false},
                {SPA_AUDIO_FORMAT_F32_BE , AkAudioCaps::SampleFormat_fltbe, false},
                {SPA_AUDIO_FORMAT_F64_LE , AkAudioCaps::SampleFormat_dblle, false},
                {SPA_AUDIO_FORMAT_F64_BE , AkAudioCaps::SampleFormat_dblbe, false},
                //{SPA_AUDIO_FORMAT_S8P    , AkAudioCaps::SampleFormat_s8   , true },
                //{SPA_AUDIO_FORMAT_U8P    , AkAudioCaps::SampleFormat_u8   , true },
                //{SPA_AUDIO_FORMAT_S16P   , AkAudioCaps::SampleFormat_s16  , true },
                //{SPA_AUDIO_FORMAT_S32P   , AkAudioCaps::SampleFormat_s32  , true },
                //{SPA_AUDIO_FORMAT_F32P   , AkAudioCaps::SampleFormat_flt  , true },
                //{SPA_AUDIO_FORMAT_F64P   , AkAudioCaps::SampleFormat_dbl  , true },
                {SPA_AUDIO_FORMAT_UNKNOWN, AkAudioCaps::SampleFormat_none , false},
            };

            return pipewireAudioSampleFormatsTable;
        }

        static const SampleFormat *byFormat(AkAudioCaps::SampleFormat format,
                                            bool planar)
        {
            auto item = table();

            for (; item->format != AkAudioCaps::SampleFormat_none; ++item)
                if (item->format == format && item->planar == planar)
                    return item;

            return item;
        }

        static const SampleFormat *byPwFormat(spa_audio_format pwFormat)
        {
            auto item = table();

            for (; item->format != AkAudioCaps::SampleFormat_none; ++item)
                if (item->pwFormat == pwFormat)
                    return item;

            return item;
        }

        static bool contains(spa_audio_format pwFormat)
        {
            for (auto item = table(); item->format != AkAudioCaps::SampleFormat_none; ++item)
                if (item->pwFormat == pwFormat)
                    return true;

            return false;
        }
};

struct SequenceParam
{
    uint32_t nodeId;
    uint32_t paramId;
};

struct AudioFormat
{
    AkAudioCaps::SampleFormat format;
    AkAudioCaps::ChannelLayout layout;
    bool planar;
};

using AudioFormats = QVector<AudioFormat>;

class AudioDevPipeWirePrivate
{
    public:
        AudioDevPipeWire *self;
        QString m_curDevice;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<uint32_t, QString> m_sinks;
        QMap<uint32_t, QString> m_sources;
        QMap<QString, AudioFormats> m_formats;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<uint32_t, QString> m_deviceIds;
        QMap<uint32_t, pw_node *> m_deviceNodes;
        QMap<int32_t, SequenceParam> m_sequenceParams;
        QMap<QString, spa_hook> m_nodeHooks;
        QMutex m_mutex;
        QMutex m_streamMutex;
        QWaitCondition m_bufferIsNotEmpty;
        QWaitCondition m_bufferIsNotFull;
        QThreadPool m_threadPool;
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
        AkAudioCaps m_deviceCaps;
        AkAudioCaps m_curCaps;
        QByteArray m_buffers;
        AkAudioConverter m_audioConvert;
        size_t m_maxBufferSize {0};
        bool m_isCapture {false};

        explicit AudioDevPipeWirePrivate(AudioDevPipeWire *self);
        static void sequenceDone(void *userData, uint32_t id, int seq);
        void readFormats(int seq, const spa_pod *param);
        static void nodeInfoChanged(void *userData,
                                    const struct pw_node_info *info);
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
        const spa_pod *buildFormat(struct spa_pod_builder *podBuilder,
                                   spa_audio_format format,
                                   int channels,
                                   int rate) const;
};

static const struct pw_core_events pipewireAudioCoreEvents = {
    .version = PW_VERSION_CORE_EVENTS               ,
    .done    = AudioDevPipeWirePrivate::sequenceDone,
};

static const struct pw_node_events pipewireAudioNodeEvents = {
    .version = PW_VERSION_NODE_EVENTS                   ,
    .info    = AudioDevPipeWirePrivate::nodeInfoChanged ,
    .param   = AudioDevPipeWirePrivate::nodeParamChanged,
};

static const struct pw_registry_events pipewireAudioDeviceEvents = {
    .version       = PW_VERSION_REGISTRY_EVENTS            ,
    .global        = AudioDevPipeWirePrivate::deviceAdded  ,
    .global_remove = AudioDevPipeWirePrivate::deviceRemoved,
};

static const struct pw_stream_events pipewireAudioStreamEvents = {
    .version       = PW_VERSION_STREAM_EVENTS               ,
    .param_changed = AudioDevPipeWirePrivate::onParamChanged,
    .process       = AudioDevPipeWirePrivate::onProcess     ,
};

AudioDevPipeWire::AudioDevPipeWire(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevPipeWirePrivate(this);

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
                          &AudioDevPipeWirePrivate::pipewireDevicesLoop,
                          this->d);
    Q_UNUSED(result)
}

AudioDevPipeWire::~AudioDevPipeWire()
{
    this->uninit();
    pw_main_loop_quit(this->d->m_pwDevicesLoop);
    this->d->m_threadPool.waitForDone();
    pw_deinit();
    delete this->d;
}

QString AudioDevPipeWire::error() const
{
    return this->d->m_error;
}

QString AudioDevPipeWire::defaultInput()
{
    this->d->m_mutex.lock();
    auto defaultSource = this->d->m_defaultSource;

    if (defaultSource.isEmpty())
        defaultSource = this->d->m_sources.values().value(0);

    this->d->m_mutex.unlock();

    return defaultSource;
}

QString AudioDevPipeWire::defaultOutput()
{
    this->d->m_mutex.lock();
    auto defaultSink = this->d->m_defaultSink;

    if (defaultSink.isEmpty())
        defaultSink = this->d->m_sinks.values().value(0);

    this->d->m_mutex.unlock();

    return defaultSink;
}

QStringList AudioDevPipeWire::inputs()
{
    this->d->m_mutex.lock();
    auto inputs = this->d->m_sources.values();
    this->d->m_mutex.unlock();

    return inputs;
}

QStringList AudioDevPipeWire::outputs()
{
    this->d->m_mutex.lock();
    auto outputs = this->d->m_sinks.values();
    this->d->m_mutex.unlock();

    return outputs;
}

QString AudioDevPipeWire::description(const QString &device)
{
    this->d->m_mutex.lock();
    auto description = this->d->m_pinDescriptionMap.value(device);
    this->d->m_mutex.unlock();

    return description;
}

AkAudioCaps AudioDevPipeWire::preferredFormat(const QString &device)
{
    auto sampleFormats = this->supportedFormats(device);

    if (sampleFormats.isEmpty())
        return {};

    auto format = sampleFormats.contains(AkAudioCaps::SampleFormat_s16)?
                      AkAudioCaps::SampleFormat_s16:
                      sampleFormats.first();

    auto channelLayouts = this->supportedChannelLayouts(device);

    if (channelLayouts.isEmpty())
        return {};

    AkAudioCaps caps;

    this->d->m_mutex.lock();

    if (this->d->m_sinks.values().contains(device)) {
        auto layout = channelLayouts.contains(AkAudioCaps::Layout_stereo)?
                          AkAudioCaps::Layout_stereo:
                          channelLayouts.first();
        caps = {format, layout, false, 48000};
    } else if (this->d->m_sources.values().contains(device)) {
        auto layout = channelLayouts.contains(AkAudioCaps::Layout_mono)?
                          AkAudioCaps::Layout_mono:
                          channelLayouts.first();
        caps = {format, layout, false, 8000};
    }

    this->d->m_mutex.unlock();

    return caps;
}

QList<AkAudioCaps::SampleFormat> AudioDevPipeWire::supportedFormats(const QString &device)
{
    QList<AkAudioCaps::SampleFormat> formats;

    this->d->m_mutex.lock();

    for (auto &format: this->d->m_formats.value(device))
        if (!formats.contains(format.format))
            formats << format.format;

    this->d->m_mutex.unlock();

    return formats;
}

QList<AkAudioCaps::ChannelLayout> AudioDevPipeWire::supportedChannelLayouts(const QString &device)
{
    QList<AkAudioCaps::ChannelLayout> layouts;

    this->d->m_mutex.lock();

    for (auto &format: this->d->m_formats.value(device))
        if (!layouts.contains(format.layout))
            layouts << format.layout;

    this->d->m_mutex.unlock();

    return layouts;
}

QList<int> AudioDevPipeWire::supportedSampleRates(const QString &device)
{
    Q_UNUSED(device)

    return this->commonSampleRates().toList();
}

bool AudioDevPipeWire::init(const QString &device, const AkAudioCaps &caps)
{
    this->uninit();
    auto pwFormat = SampleFormat::byFormat(caps.format(), caps.planar());

    if (!pwFormat)
        return false;

    this->d->m_curDevice = device;
    this->d->m_curCaps = caps;
    this->d->m_curCaps = caps;

    this->d->m_mutex.lock();
    this->d->m_isCapture = std::find(this->d->m_sources.cbegin(),
                                     this->d->m_sources.cend(),
                                     device) != this->d->m_sources.cend();
    this->d->m_mutex.unlock();

    this->d->m_pwStreamLoop =
        pw_thread_loop_new("PipeWire audio loop", nullptr);

    if (!this->d->m_pwStreamLoop) {
        this->uninit();
        qCritical() << "Error creating PipeWire audio thread loop:" << strerror(errno);

        return false;
    }

    this->d->m_pwStreamContext =
        pw_context_new(pw_thread_loop_get_loop(this->d->m_pwStreamLoop),
                       nullptr,
                       0);

    if (!this->d->m_pwStreamContext) {
        this->uninit();
        qCritical() << "Error creating PipeWire context";

        return false;
    }

    if (pw_thread_loop_start(this->d->m_pwStreamLoop) < 0) {
        this->uninit();
        qCritical() << "Error starting PipeWire main loop";

        return false;
    }

    pw_thread_loop_lock(this->d->m_pwStreamLoop);

    this->d->m_pwStreamCore =
        pw_context_connect(this->d->m_pwStreamContext, nullptr, 0);

    if (!this->d->m_pwStreamCore) {
        pw_thread_loop_unlock(this->d->m_pwStreamLoop);
        this->uninit();
        qCritical() << "Error connecting to the PipeWire file descriptor:" << strerror(errno);

        return false;
    }

    this->d->m_pwStream =
            pw_stream_new(this->d->m_pwStreamCore,
                          this->d->m_isCapture?
                              "Webcamoid Audio Capture":
                              "Webcamoid Audio Playback",
                          pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                            PW_KEY_MEDIA_CATEGORY, this->d->m_isCapture?
                                                                       "Capture":
                                                                       "Playback",
                                            PW_KEY_MEDIA_ROLE, "Music",
#if PW_CHECK_VERSION(0, 3, 44)
                                            PW_KEY_TARGET_OBJECT, this->d->m_curDevice.toStdString().c_str(),
#endif
                                            nullptr));
    pw_stream_add_listener(this->d->m_pwStream,
                           &this->d->m_streamHook,
                           &pipewireAudioStreamEvents,
                           this->d);

    QVector<const spa_pod *>params;
    static const size_t bufferSize = 4096;
    uint8_t buffer[bufferSize];
    auto podBuilder = SPA_POD_BUILDER_INIT(buffer, bufferSize);

    params << this->d->buildFormat(&podBuilder,
                                   pwFormat->pwFormat,
                                   caps.channels(),
                                   caps.rate());

    pw_stream_connect(this->d->m_pwStream,
                      this->d->m_isCapture? PW_DIRECTION_INPUT: PW_DIRECTION_OUTPUT,
                      PW_ID_ANY,
                      pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT
                                      | PW_STREAM_FLAG_MAP_BUFFERS
                                      | PW_STREAM_FLAG_RT_PROCESS ),
                      params.data(),
                      params.size());
    pw_thread_loop_unlock(this->d->m_pwStreamLoop);

    return true;
}

QByteArray AudioDevPipeWire::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_pwStream)
        return {};

    if (this->d->m_buffers.isEmpty())
        if (!this->d->m_bufferIsNotEmpty.wait(&this->d->m_mutex, 1000))
            return {};

    auto buffers = this->d->m_buffers;
    this->d->m_buffers.clear();

    return buffers;
}

bool AudioDevPipeWire::write(const AkAudioPacket &packet)
{
    if (!packet)
        return false;

    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_pwStream)
        return false;

    if (this->d->m_buffers.size() >= this->d->m_maxBufferSize)
        if (!this->d->m_bufferIsNotFull.wait(&this->d->m_mutex, 1000))
            return false;

    auto audioPacket = this->d->m_audioConvert.convert(packet);

    if (!audioPacket)
        return false;

    this->d->m_buffers += QByteArray(audioPacket.constData(),
                                     audioPacket.size());

    return true;
}

bool AudioDevPipeWire::uninit()
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

    this->d->m_buffers.clear();

    return true;
}

AudioDevPipeWirePrivate::AudioDevPipeWirePrivate(AudioDevPipeWire *self):
    self(self)
{
}

void AudioDevPipeWirePrivate::sequenceDone(void *userData, uint32_t id, int seq)
{
    Q_UNUSED(id)

    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);
    self->m_sequenceParams.remove(seq - 1);
}

void AudioDevPipeWirePrivate::readFormats(int seq, const spa_pod *param)
{
    if (SPA_POD_TYPE(param) != SPA_TYPE_Object)
        return;

    spa_media_subtype mediaSubtype = SPA_MEDIA_SUBTYPE_unknown;
    spa_audio_format format = SPA_AUDIO_FORMAT_UNKNOWN;
    int32_t channels = 0;

    if (spa_pod_parse_object(param,
                             SPA_TYPE_OBJECT_Format   , nullptr,
                             SPA_FORMAT_mediaSubtype  , SPA_POD_Id(&mediaSubtype),
                             SPA_FORMAT_AUDIO_format  , SPA_POD_Id(&format),
                             SPA_FORMAT_AUDIO_channels, SPA_POD_Int(&channels)) < 0) {
        return;
    }

    if (!SampleFormat::contains(format))
        return;

    auto &nodeId = this->m_sequenceParams[seq].nodeId;
    auto &deviceId = this->m_deviceIds[nodeId];

    auto fmt = SampleFormat::byPwFormat(format);

    AudioFormat supportedFormat = {
        fmt->format,
        AkAudioCaps::defaultChannelLayout(channels),
        fmt->planar,
    };

    if (this->m_formats.contains(deviceId))
        this->m_formats[deviceId] << supportedFormat;
    else
        this->m_formats[deviceId] = {supportedFormat};
}

void AudioDevPipeWirePrivate::nodeInfoChanged(void *userData,
                                              const pw_node_info *info)
{
    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);

    for (uint32_t i = 0; i < info->n_params; i++) {
        if (!(info->params[i].flags & SPA_PARAM_INFO_READ))
            continue;

        auto &id = info->params[i].id;

        switch (id) {
        case SPA_PARAM_EnumFormat: {
            auto node = self->m_deviceNodes.value(info->id);

            if (!node)
                return;

            auto &deviceId = self->m_deviceIds[info->id];

            if (!self->m_formats.contains(deviceId))
                self->m_formats[deviceId] = {};

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

void AudioDevPipeWirePrivate::nodeParamChanged(void *userData,
                                               int seq,
                                               uint32_t id,
                                               uint32_t index,
                                               uint32_t next,
                                               const spa_pod *param)
{
    Q_UNUSED(id)
    Q_UNUSED(index)
    Q_UNUSED(next)

    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);

    switch (self->m_sequenceParams[seq].paramId) {
    case SPA_PARAM_EnumFormat:
        self->readFormats(seq, param);

        break;

    default:
        break;
    }
}

void AudioDevPipeWirePrivate::deviceAdded(void *userData,
                                          uint32_t id,
                                          uint32_t permissions,
                                          const char *type,
                                          uint32_t version,
                                          const spa_dict *props)
{
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);

    if (QString(type) != PW_TYPE_INTERFACE_Node)
        return;

    if (!props)
        return;

    auto mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);

    if (!mediaClass)
        return;

    static const char *pipewireAudioSupportedClasses[] = {
        "Stream/Output/Audio",
        "Stream/Input/Audio/Internal",
        "Audio/Source",
        "Audio/Sink",
        nullptr
    };

    bool classSupported = false;

    for (auto cls = pipewireAudioSupportedClasses; *cls; ++cls)
        if (!strncmp(mediaClass, *cls, 1024)) {
            classSupported = true;

            break;
        }

    if (!classSupported)
        return;

    auto node =
        reinterpret_cast<pw_node *>(pw_registry_bind(self->m_pwRegistry,
                                                     id,
                                                     type,
                                                     PW_VERSION_NODE,
                                                     0));

    if (!node)
        return;

    auto nodeName = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    auto description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    bool isSink = QString(mediaClass) == "Audio/Sink";

    if (isSink)
        self->m_sinks[id] = nodeName;
    else
        self->m_sources[id] = nodeName;

    self->m_pinDescriptionMap[nodeName] = description;
    self->m_formats[nodeName] = {};
    self->m_deviceIds[id] = nodeName;
    self->m_deviceNodes[id] = node;
    self->m_nodeHooks[nodeName] = {};
    auto &hook = self->m_nodeHooks[nodeName];
    pw_proxy_add_object_listener(reinterpret_cast<pw_proxy *>(node),
                                 &hook,
                                 &pipewireAudioNodeEvents,
                                 self);

    if (isSink)
        emit self->self->outputsChanged(self->m_sinks.values());
    else
        emit self->self->inputsChanged(self->m_sources.values());

    if (self->m_defaultSource.isEmpty()
        && QString(mediaClass) == "Audio/Source") {
        self->m_defaultSource = nodeName;
        emit self->self->defaultInputChanged(self->m_defaultSource);
    }

    if (self->m_defaultSink.isEmpty()
        && QString(mediaClass) == "Audio/Sink") {
        self->m_defaultSink = nodeName;
        emit self->self->defaultOutputChanged(self->m_defaultSink);
    }
}

void AudioDevPipeWirePrivate::deviceRemoved(void *userData, uint32_t id)
{
    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);
    auto name = self->m_deviceIds.value(id);

    if (name.isEmpty())
        return;

    bool isSink = self->m_sinks.contains(id);

    if (isSink)
        self->m_sinks.remove(id);
    else
        self->m_sources.remove(id);

    self->m_pinDescriptionMap.remove(name);
    self->m_formats.remove(name);
    self->m_deviceIds.remove(id);
    self->m_deviceNodes.remove(id);
    auto &hook = self->m_nodeHooks[name];
    spa_hook_remove(&hook);
    self->m_nodeHooks.remove(name);

    if (isSink)
        emit self->self->outputsChanged(self->m_sinks.values());
    else
        emit self->self->inputsChanged(self->m_sources.values());

    if (self->m_defaultSource == name) {
        self->m_defaultSource = self->m_sources.values().value(0);
        emit self->self->defaultInputChanged(self->m_defaultSource);
    }

    if (self->m_defaultSink == name) {
        self->m_defaultSink = self->m_sinks.values().value(0);
        emit self->self->defaultOutputChanged(self->m_defaultSink);
    }
}

void AudioDevPipeWirePrivate::onParamChanged(void *userData,
                                             uint32_t id,
                                             const spa_pod *param)
{
    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);

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

        if (mediaType != SPA_MEDIA_TYPE_audio ||
            mediaSubtype != SPA_MEDIA_SUBTYPE_raw)
            return;

        spa_audio_info_raw info;

        if (spa_format_audio_raw_parse(param, &info) < 0)
            return;

        auto fmt = SampleFormat::byPwFormat(info.format);
        self->m_deviceCaps = {fmt->format,
                              AkAudioCaps::defaultChannelLayout(info.channels),
                              fmt->planar,
                              int(info.rate)};
        self->m_maxBufferSize = self->self->latency()
                                * self->m_deviceCaps.bps()
                                * info.channels
                                * info.rate
                                / 4000;
        self->m_audioConvert.setOutputCaps(self->m_deviceCaps);
        self->m_audioConvert.reset();

        break;
    }

    default:
        break;
    }
}

void AudioDevPipeWirePrivate::onProcess(void *userData)
{
    auto self = reinterpret_cast<AudioDevPipeWirePrivate *>(userData);
    auto buffer = pw_stream_dequeue_buffer(self->m_pwStream);

    if (!buffer)
        return;

    if (!buffer->buffer->datas[0].data)
        return;

    auto data = reinterpret_cast<quint8 *>(buffer->buffer->datas[0].data);

    QMutexLocker mutexLocker(&self->m_mutex);

    if (self->m_isCapture) {
        auto dataSize = buffer->buffer->datas[0].chunk->size;
        QByteArray buffer(reinterpret_cast<char *>(data), dataSize);

        if (dataSize >= self->m_maxBufferSize) {
            self->m_buffers = buffer;
        } else {
            auto totalSize = qMin<size_t>(self->m_buffers.size() + dataSize,
                                          self->m_maxBufferSize);
            auto prevSize = totalSize - dataSize;
            self->m_buffers =
                    self->m_buffers.mid(self->m_buffers.size() - int(prevSize),
                                        int(prevSize))
                    + buffer;
        }

        self->m_bufferIsNotEmpty.wakeAll();
    } else {
        auto dataSize = buffer->buffer->datas[0].maxsize;
        auto copySize = qMin<qsizetype>(dataSize, self->m_buffers.size());

        if (copySize > 0)
            memcpy(data, self->m_buffers.constData(), copySize);

        auto remainingSize = self->m_buffers.size() - copySize;

        if (remainingSize > 0)
            self->m_buffers = self->m_buffers.mid(copySize, remainingSize);
        else
            self->m_buffers.clear();

        if (self->m_buffers.size() < self->m_maxBufferSize)
            self->m_bufferIsNotFull.wakeAll();

        if (copySize > 0) {
            auto chunk = buffer->buffer->datas[0].chunk;
            chunk->offset = 0;
            chunk->stride = self->m_deviceCaps.bps() * self->m_deviceCaps.channels() / 8;
            chunk->size = copySize;
        }
    }

    pw_stream_queue_buffer(self->m_pwStream, buffer);
}

void AudioDevPipeWirePrivate::pipewireDevicesLoop()
{
    this->m_pwDevicesLoop = pw_main_loop_new(nullptr);

    if (!this->m_pwDevicesLoop)
        return;

    auto pwContext =
        pw_context_new(pw_main_loop_get_loop(this->m_pwDevicesLoop),
                                             nullptr,
                                             0);

    if (!pwContext) {
        pw_main_loop_destroy(this->m_pwDevicesLoop);

        return;
    }

    this->m_pwDeviceCore = pw_context_connect(pwContext, nullptr, 0);

    if (!this->m_pwDeviceCore) {
        pw_context_destroy(pwContext);
        pw_main_loop_destroy(this->m_pwDevicesLoop);

        return;
    }

    memset(&this->m_coreHook, 0, sizeof(spa_hook));
    pw_core_add_listener(this->m_pwDeviceCore,
                         &this->m_coreHook,
                         &pipewireAudioCoreEvents,
                         this);
    this->m_pwRegistry = pw_core_get_registry(this->m_pwDeviceCore,
                                                PW_VERSION_REGISTRY,
                                                0);

    if (!this->m_pwRegistry) {
        pw_core_disconnect(this->m_pwDeviceCore);
        pw_context_destroy(pwContext);
        pw_main_loop_destroy(this->m_pwDevicesLoop);

        return;
    }

    memset(&this->m_deviceHook, 0, sizeof(spa_hook));
    pw_registry_add_listener(this->m_pwRegistry,
                             &this->m_deviceHook,
                             &pipewireAudioDeviceEvents,
                             this);
    pw_main_loop_run(this->m_pwDevicesLoop);
    pw_proxy_destroy((struct pw_proxy *) this->m_pwRegistry);
    pw_core_disconnect(this->m_pwDeviceCore);
    pw_context_destroy(pwContext);
    pw_main_loop_destroy(this->m_pwDevicesLoop);
}

const spa_pod *AudioDevPipeWirePrivate::buildFormat(spa_pod_builder *podBuilder,
                                                    spa_audio_format format,
                                                    int channels,
                                                    int rate) const
{
    struct spa_pod_frame podFrame[2];
    spa_pod_builder_push_object(podBuilder,
                                &podFrame[0],
                                SPA_TYPE_OBJECT_Format,
                                SPA_PARAM_EnumFormat);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_mediaType,
                        SPA_POD_Id(SPA_MEDIA_TYPE_audio),
                        0);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_mediaSubtype,
                        SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                        0);
    spa_pod_builder_add(podBuilder,
                        SPA_FORMAT_AUDIO_format,
                        SPA_POD_Id(format),
                        0);
    spa_pod_builder_push_choice(podBuilder, &podFrame[1], SPA_CHOICE_Enum, 0);

    int i = 0;

    for (auto fmt = SampleFormat::table(); fmt->format != AkAudioCaps::SampleFormat_none; ++fmt, ++i) {
        if (i == 0)
             spa_pod_builder_id(podBuilder, fmt->pwFormat);

        spa_pod_builder_id(podBuilder, fmt->pwFormat);
    }

    spa_pod_builder_pop(podBuilder, &podFrame[1]);
    spa_pod_builder_add(podBuilder, SPA_FORMAT_AUDIO_channels,
                        SPA_POD_CHOICE_RANGE_Int(
                            channels,
                            1,
                            2),
                        0);
    auto sampleRates = self->commonSampleRates();
    spa_pod_builder_add(podBuilder, SPA_FORMAT_AUDIO_rate,
                        SPA_POD_CHOICE_RANGE_Int(
                            rate,
                            sampleRates.first(),
                            sampleRates.last()),
                        0);
    auto pod = spa_pod_builder_pop(podBuilder, &podFrame[0]);

    return reinterpret_cast<const spa_pod *>(pod);
}

#include "moc_audiodevpipewire.cpp"
