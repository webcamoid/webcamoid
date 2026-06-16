/* Webcamoid, camera capture application.
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

#include <QCoreApplication>
#include <QLibrary>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QtDebug>
#include <akaudiopacket.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>

#include "audiodevpulseaudio.h"

using SampleFormatMap = QMap<pa_sample_format_t, AkAudioCaps::SampleFormat>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat {
        {PA_SAMPLE_U8       , AkAudioCaps::SampleFormat_u8   },
        {PA_SAMPLE_S16BE    , AkAudioCaps::SampleFormat_s16be},
        {PA_SAMPLE_S16LE    , AkAudioCaps::SampleFormat_s16le},
        {PA_SAMPLE_S32BE    , AkAudioCaps::SampleFormat_s32be},
        {PA_SAMPLE_S32LE    , AkAudioCaps::SampleFormat_s32le},
        {PA_SAMPLE_FLOAT32BE, AkAudioCaps::SampleFormat_fltbe},
        {PA_SAMPLE_FLOAT32LE, AkAudioCaps::SampleFormat_fltle},
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

class DevicesInfo
{
    public:
        QString defaultSink;
        QString defaultSource;
        QStringList sinks;
        QStringList sources;
        QMap<QString, QString> devicesDescriptions;
        QMap<QString, AkAudioCaps> devicesCaps;
        bool waitingForSinks {true};
        bool waitingForSources {true};
        bool waitingForServer {true};

        void clear()
        {
            this->defaultSink = {};
            this->defaultSource = {};
            this->sinks = {};
            this->sources = {};
            this->devicesDescriptions = {};
            this->devicesCaps = {};
            this->waitingForSinks = true;
            this->waitingForSources = true;
            this->waitingForServer = true;
        }
};

// Dynamic load type definitions

#ifdef USE_PULSEAUDIO_DYNLOAD

using PaContextConnectType = int (*)(pa_context *c,
                                     const char *server,
                                     pa_context_flags_t flags,
                                     const pa_spawn_api *api);
using PaContextDisconnectType = void (*)(pa_context *c);
using PaContextGetSinkInfoListType = pa_operation *(*)(pa_context *c,
                                                       pa_sink_info_cb_t cb,
                                                       void *userdata);
using PaContextGetSourceInfoListType = pa_operation *(*)(pa_context *c,
                                                         pa_source_info_cb_t cb,
                                                         void *userdata);
using PaContextGetServerInfoType = pa_operation *(*)(pa_context *c,
                                                     pa_server_info_cb_t cb,
                                                     void *userdata);
using PaContextGetStateType = pa_context_state_t (*)(const pa_context *c);
using PaContextNewType = pa_context *(*)(pa_mainloop_api *mainloop,
                                         const char *name);
using PaContextSetStateCallbackType = void (*)(pa_context *c,
                                               pa_context_notify_cb_t cb,
                                               void *userdata);
using PaContextUnrefType = void (*)(pa_context *c);
using PaMainloopFreeType = void (*)(pa_mainloop *m);
using PaMainloopGetApiType = pa_mainloop_api *(*)(pa_mainloop *m);
using PaMainloopIterateType = int (*)(pa_mainloop *m,
                                      int block,
                                      int *retval);
using PaMainloopNewType = pa_mainloop *(*)();
using PaSimpleDrainType = int (*)(pa_simple *s, int *error);
using PaSimpleFreeType = void (*)(pa_simple *s);
using PaSimpleGetLatencyType = pa_usec_t (*)(pa_simple *s, int *error);
using PaSimpleNewType = pa_simple *(*)(const char *server,
                                       const char *name,
                                       pa_stream_direction_t dir,
                                       const char *dev,
                                       const char *stream_name,
                                       const pa_sample_spec *ss,
                                       const pa_channel_map *map,
                                       const pa_buffer_attr *attr,
                                       int *error);
using PaSimpleReadType = int (*)(pa_simple *s,
                                  void *data,
                                  size_t bytes,
                                  int *error);
using PaSimpleWriteType = int (*)(pa_simple *s,
                                   const void *data,
                                   size_t bytes,
                                   int *error);
using PaStrerrorType = const char *(*)(int error);

#endif // USE_PULSEAUDIO_DYNLOAD

class AudioDevPulseAudioPrivate
{
    public:
        AudioDevPulseAudio *self;
        QString m_error;
        pa_simple *m_paSimple {nullptr};
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_devicesDescriptions;
        QMap<QString, AkAudioCaps> m_devicesCaps;
        QMutex m_mutex;
        QMutex m_streamMutex;
        AkAudioCaps m_curCaps;
        QByteArray m_readBuffer;
        DevicesInfo m_devicesInfo;

#ifdef USE_PULSEAUDIO_DYNLOAD
        QLibrary m_pulseAudioLib {"pulse"};
        QLibrary m_pulseAudioSimpleLib {"pulse-simple"};

        PaContextConnectType           m_paContextConnect           {nullptr};
        PaContextDisconnectType        m_paContextDisconnect        {nullptr};
        PaContextGetSinkInfoListType   m_paContextGetSinkInfoList   {nullptr};
        PaContextGetSourceInfoListType m_paContextGetSourceInfoList {nullptr};
        PaContextGetServerInfoType     m_paContextGetServerInfo     {nullptr};
        PaContextGetStateType          m_paContextGetState          {nullptr};
        PaContextNewType               m_paContextNew               {nullptr};
        PaContextSetStateCallbackType  m_paContextSetStateCallback  {nullptr};
        PaContextUnrefType             m_paContextUnref             {nullptr};
        PaMainloopFreeType             m_paMainloopFree             {nullptr};
        PaMainloopGetApiType           m_paMainloopGetApi           {nullptr};
        PaMainloopIterateType          m_paMainloopIterate          {nullptr};
        PaMainloopNewType              m_paMainloopNew              {nullptr};
        PaSimpleDrainType              m_paSimpleDrain              {nullptr};
        PaSimpleFreeType               m_paSimpleFree               {nullptr};
        PaSimpleGetLatencyType         m_paSimpleGetLatency         {nullptr};
        PaSimpleNewType                m_paSimpleNew                {nullptr};
        PaSimpleReadType               m_paSimpleRead               {nullptr};
        PaSimpleWriteType              m_paSimpleWrite              {nullptr};
        PaStrerrorType                 m_paStrerror                 {nullptr};
#endif

        explicit AudioDevPulseAudioPrivate(AudioDevPulseAudio *self);
        void updateDevices();
        static void contextStateCallback(pa_context *context,
                                         void *userData);
        static void serverInfoCallback(pa_context *context,
                                       const pa_server_info *info,
                                       void *userData);
        static void sourceInfoCallback(pa_context *context,
                                       const pa_source_info *info,
                                       int isLast,
                                       void *userData);
        static void sinkInfoCallback(pa_context *context,
                                     const pa_sink_info *info,
                                     int isLast,
                                     void *userData);

        // PulseAudio function wrappers

        inline int paContextConnect(pa_context *c,
                                    const char *server,
                                    pa_context_flags_t flags,
                                    const pa_spawn_api *api) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextConnect)
                return this->m_paContextConnect(c, server, flags, api);

            return -1;
#else
            return pa_context_connect(c, server, flags, api);
#endif
        }

        inline void paContextDisconnect(pa_context *c) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextDisconnect)
                this->m_paContextDisconnect(c);
#else
            pa_context_disconnect(c);
#endif
        }

        inline pa_operation *paContextGetSinkInfoList(pa_context *c,
                                                      pa_sink_info_cb_t cb,
                                                      void *userdata) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextGetSinkInfoList)
                return this->m_paContextGetSinkInfoList(c, cb, userdata);

            return nullptr;
#else
            return pa_context_get_sink_info_list(c, cb, userdata);
#endif
        }

        inline pa_operation *paContextGetSourceInfoList(pa_context *c,
                                                        pa_source_info_cb_t cb,
                                                        void *userdata) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextGetSourceInfoList)
                return this->m_paContextGetSourceInfoList(c, cb, userdata);

            return nullptr;
#else
            return pa_context_get_source_info_list(c, cb, userdata);
#endif
        }

        inline pa_operation *paContextGetServerInfo(pa_context *c,
                                                    pa_server_info_cb_t cb,
                                                    void *userdata) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextGetServerInfo)
                return this->m_paContextGetServerInfo(c, cb, userdata);

            return nullptr;
#else
            return pa_context_get_server_info(c, cb, userdata);
#endif
        }

        inline pa_context_state_t paContextGetState(const pa_context *c) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextGetState)
                return this->m_paContextGetState(c);

            return PA_CONTEXT_FAILED;
#else
            return pa_context_get_state(c);
#endif
        }

        inline pa_context *paContextNew(pa_mainloop_api *mainloop,
                                        const char *name) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextNew)
                return this->m_paContextNew(mainloop, name);

            return nullptr;
#else
            return pa_context_new(mainloop, name);
#endif
        }

        inline void paContextSetStateCallback(pa_context *c,
                                              pa_context_notify_cb_t cb,
                                              void *userdata) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextSetStateCallback)
                this->m_paContextSetStateCallback(c, cb, userdata);
#else
            pa_context_set_state_callback(c, cb, userdata);
#endif
        }

        inline void paContextUnref(pa_context *c) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paContextUnref)
                this->m_paContextUnref(c);
#else
            pa_context_unref(c);
#endif
        }

        inline void paMainloopFree(pa_mainloop *m) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paMainloopFree)
                this->m_paMainloopFree(m);
#else
            pa_mainloop_free(m);
#endif
        }

        inline pa_mainloop_api *paMainloopGetApi(pa_mainloop *m) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paMainloopGetApi)
                return this->m_paMainloopGetApi(m);

            return nullptr;
#else
            return pa_mainloop_get_api(m);
#endif
        }

        inline int paMainloopIterate(pa_mainloop *m,
                                     int block,
                                     int *retval) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paMainloopIterate)
                return this->m_paMainloopIterate(m, block, retval);

            return -1;
#else
            return pa_mainloop_iterate(m, block, retval);
#endif
        }

        inline pa_mainloop *paMainloopNew() const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paMainloopNew)
                return this->m_paMainloopNew();

            return nullptr;
#else
            return pa_mainloop_new();
#endif
        }

        inline int paSimpleDrain(pa_simple *s, int *error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleDrain)
                return this->m_paSimpleDrain(s, error);

            return -1;
#else
            return pa_simple_drain(s, error);
#endif
        }

        inline void paSimpleFree(pa_simple *s) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleFree)
                this->m_paSimpleFree(s);
#else
            pa_simple_free(s);
#endif
        }

        inline pa_usec_t paSimpleGetLatency(pa_simple *s, int *error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleGetLatency)
                return this->m_paSimpleGetLatency(s, error);

            return 0;
#else
            return pa_simple_get_latency(s, error);
#endif
        }

        inline pa_simple *paSimpleNew(const char *server,
                                      const char *name,
                                      pa_stream_direction_t dir,
                                      const char *dev,
                                      const char *stream_name,
                                      const pa_sample_spec *ss,
                                      const pa_channel_map *map,
                                      const pa_buffer_attr *attr,
                                      int *error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleNew)
                return this->m_paSimpleNew(server,
                                           name,
                                           dir,
                                           dev,
                                           stream_name,
                                           ss,
                                           map,
                                           attr,
                                           error);

            return nullptr;
#else
            return pa_simple_new(server,
                                 name,
                                 dir,
                                 dev,
                                 stream_name,
                                 ss,
                                 map,
                                 attr,
                                 error);
#endif
        }

        inline int paSimpleRead(pa_simple *s,
                                void *data,
                                size_t bytes,
                                int *error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleRead)
                return this->m_paSimpleRead(s, data, bytes, error);

            return -1;
#else
            return pa_simple_read(s, data, bytes, error);
#endif
        }

        inline int paSimpleWrite(pa_simple *s,
                                 const void *data,
                                 size_t bytes,
                                 int *error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paSimpleWrite)
                return this->m_paSimpleWrite(s, data, bytes, error);

            return -1;
#else
            return pa_simple_write(s, data, bytes, error);
#endif
        }

        inline const char *paStrerror(int error) const
        {
#ifdef USE_PULSEAUDIO_DYNLOAD
            if (this->m_paStrerror)
                return this->m_paStrerror(error);

            return "";
#else
            return pa_strerror(error);
#endif
        }
};

AudioDevPulseAudio::AudioDevPulseAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevPulseAudioPrivate(this);
    this->d->updateDevices();
}

AudioDevPulseAudio::~AudioDevPulseAudio()
{
    this->uninit();
    delete this->d;
}

QString AudioDevPulseAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevPulseAudio::defaultInput()
{
    this->d->m_mutex.lock();
    auto defaultSource = this->d->m_defaultSource;
    this->d->m_mutex.unlock();

    return defaultSource;
}

QString AudioDevPulseAudio::defaultOutput()
{
    this->d->m_mutex.lock();
    auto defaultSink = this->d->m_defaultSink;
    this->d->m_mutex.unlock();

    return defaultSink;
}

QStringList AudioDevPulseAudio::inputs()
{
    this->d->m_mutex.lock();
    auto inputs = this->d->m_sources;
    this->d->m_mutex.unlock();

    return inputs;
}

QStringList AudioDevPulseAudio::outputs()
{
    this->d->m_mutex.lock();
    auto outputs = this->d->m_sinks;
    this->d->m_mutex.unlock();

    return outputs;
}

QString AudioDevPulseAudio::description(const QString &device)
{
    this->d->m_mutex.lock();
    auto description = this->d->m_devicesDescriptions.value(device);
    this->d->m_mutex.unlock();

    return description;
}

AkAudioCaps AudioDevPulseAudio::preferredFormat(const QString &device)
{
    this->d->m_mutex.lock();
    auto caps = this->d->m_devicesCaps.value(device);
    this->d->m_mutex.unlock();

    return caps;
}

QList<AkAudioCaps::SampleFormat> AudioDevPulseAudio::supportedFormats(const QString &device)
{
    Q_UNUSED(device)

    return sampleFormats->values();
}

QList<AkAudioCaps::ChannelLayout> AudioDevPulseAudio::supportedChannelLayouts(const QString &device)
{
    Q_UNUSED(device)

    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
}

QList<int> AudioDevPulseAudio::supportedSampleRates(const QString &device)
{
    Q_UNUSED(device)

    return this->commonSampleRates().toList();
}

bool AudioDevPulseAudio::init(const QString &device, const AkAudioCaps &caps)
{
    qDebug() << "Initializing PulseAudio device" << device << caps;

    if (!this->d->m_sources.contains(device)
        && !this->d->m_sinks.contains(device))
        return false;

    this->d->m_streamMutex.lock();

    pa_sample_spec ss;
    ss.format = sampleFormats->key(caps.format());
    ss.channels = uint8_t(caps.channels());
    ss.rate = uint32_t(caps.rate());

    if (this->d->m_curCaps != caps) {
        this->d->m_curCaps = caps;
        this->d->m_streamMutex.unlock();
        emit this->negotiatedCapsChanged(this->d->m_curCaps);
        this->d->m_streamMutex.lock();
    }

    bool isInput = this->d->m_sources.contains(device);

    int error;
    this->d->m_paSimple =
            this->d->paSimpleNew(nullptr,
                                 QCoreApplication::applicationName().toStdString().c_str(),
                                 isInput? PA_STREAM_RECORD: PA_STREAM_PLAYBACK,
                                 device.isEmpty()? nullptr: device.toStdString().c_str(),
                                 QCoreApplication::organizationName().toStdString().c_str(),
                                 &ss,
                                 nullptr,
                                 nullptr,
                                 &error);

    if (!this->d->m_paSimple) {
        this->d->m_error = QString(this->d->paStrerror(error));
        this->d->m_streamMutex.unlock();
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    auto paLatency = this->d->paSimpleGetLatency(this->d->m_paSimple, nullptr);
    int samples = qMax(qMax<int>(1000 * this->latency(), paLatency) * caps.rate() / 1000000, 1);
    this->d->m_readBuffer.resize(samples
                                 * this->d->m_curCaps.bps()
                                 * this->d->m_curCaps.channels()
                                 / 8);

    this->d->m_streamMutex.unlock();

    qDebug() << "PulseAudio device initialized";

    return true;
}

AkAudioCaps AudioDevPulseAudio::negotiatedCaps() const
{
    return this->d->m_curCaps;
}

QByteArray AudioDevPulseAudio::read()
{
    this->d->m_streamMutex.lock();

    if (!this->d->m_paSimple) {
        this->d->m_streamMutex.unlock();

        return {};
    }

    int error;

    if (this->d->paSimpleRead(this->d->m_paSimple,
                              this->d->m_readBuffer.data(),
                              size_t(this->d->m_readBuffer.size()),
                              &error) < 0) {
        this->d->m_error = QString(this->d->paStrerror(error));
        this->d->m_streamMutex.unlock();
        qCritical() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return {};
    }

    this->d->m_streamMutex.unlock();

    return this->d->m_readBuffer;
}

bool AudioDevPulseAudio::write(const AkAudioPacket &packet)
{
    this->d->m_streamMutex.lock();

    if (!this->d->m_paSimple) {
        this->d->m_streamMutex.unlock();

        return false;
    }

    int error;

    if (this->d->paSimpleWrite(this->d->m_paSimple,
                               packet.constData(),
                               packet.size(),
                               &error) < 0) {
        this->d->m_error = QString(this->d->paStrerror(error));
        this->d->m_streamMutex.unlock();
        qCritical() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    this->d->m_streamMutex.unlock();

    return true;
}

bool AudioDevPulseAudio::uninit()
{
    qDebug() << "Uninitializing PulseAudio device";

    QString errorStr;
    bool ok = true;

    this->d->m_streamMutex.lock();

    if (this->d->m_paSimple) {
        int error;

        if (this->d->paSimpleDrain(this->d->m_paSimple, &error) < 0) {
            errorStr = QString(this->d->paStrerror(error));
            ok = false;
        }

        this->d->paSimpleFree(this->d->m_paSimple);
    } else {
        ok = false;
    }

    this->d->m_paSimple = nullptr;
    this->d->m_curCaps = AkAudioCaps();
    this->d->m_streamMutex.unlock();

    if (!errorStr.isEmpty()) {
        this->d->m_error = errorStr;
        emit this->errorChanged(this->d->m_error);
    }

    this->d->m_readBuffer.clear();

    qDebug() << "PulseAudio device Uninitialized";

    return ok;
}

void AudioDevPulseAudio::updateDevices()
{
    this->d->updateDevices();
}

AudioDevPulseAudioPrivate::AudioDevPulseAudioPrivate(AudioDevPulseAudio *self):
    self(self)
{
#ifdef USE_PULSEAUDIO_DYNLOAD
    // libpulse-simple.so may be a separate library in some distributions
    // We attempt to load both; the symbols for simple can be in either one..
    bool pulseLoaded = this->m_pulseAudioLib.load();
    bool simpleLoaded = this->m_pulseAudioSimpleLib.load();

    if (!pulseLoaded && !simpleLoaded) {
        qWarning() << "Could not load libpulse or libpulse-simple";

        return;
    }

    // Helper: look for the symbol in pulse-simple first, then in pulse
    auto resolve = [&](const char *symbol) -> QFunctionPointer {
        QFunctionPointer ptr = nullptr;

        if (simpleLoaded)
            ptr = this->m_pulseAudioSimpleLib.resolve(symbol);

        if (!ptr && pulseLoaded)
            ptr = this->m_pulseAudioLib.resolve(symbol);

        if (!ptr)
            qWarning() << "PulseAudio: could not resolve" << symbol;

        return ptr;
    };

    this->m_paContextConnect =
        reinterpret_cast<PaContextConnectType>(resolve("pa_context_connect"));
    this->m_paContextDisconnect =
        reinterpret_cast<PaContextDisconnectType>(resolve("pa_context_disconnect"));
    this->m_paContextGetSinkInfoList =
        reinterpret_cast<PaContextGetSinkInfoListType>(resolve("pa_context_get_sink_info_list"));
    this->m_paContextGetSourceInfoList =
        reinterpret_cast<PaContextGetSourceInfoListType>(resolve("pa_context_get_source_info_list"));
    this->m_paContextGetServerInfo =
        reinterpret_cast<PaContextGetServerInfoType>(resolve("pa_context_get_server_info"));
    this->m_paContextGetState =
        reinterpret_cast<PaContextGetStateType>(resolve("pa_context_get_state"));
    this->m_paContextNew =
        reinterpret_cast<PaContextNewType>(resolve("pa_context_new"));
    this->m_paContextSetStateCallback =
        reinterpret_cast<PaContextSetStateCallbackType>(resolve("pa_context_set_state_callback"));
    this->m_paContextUnref =
        reinterpret_cast<PaContextUnrefType>(resolve("pa_context_unref"));
    this->m_paMainloopFree =
        reinterpret_cast<PaMainloopFreeType>(resolve("pa_mainloop_free"));
    this->m_paMainloopGetApi =
        reinterpret_cast<PaMainloopGetApiType>(resolve("pa_mainloop_get_api"));
    this->m_paMainloopIterate =
        reinterpret_cast<PaMainloopIterateType>(resolve("pa_mainloop_iterate"));
    this->m_paMainloopNew =
        reinterpret_cast<PaMainloopNewType>(resolve("pa_mainloop_new"));
    this->m_paSimpleDrain =
        reinterpret_cast<PaSimpleDrainType>(resolve("pa_simple_drain"));
    this->m_paSimpleFree =
        reinterpret_cast<PaSimpleFreeType>(resolve("pa_simple_free"));
    this->m_paSimpleGetLatency =
        reinterpret_cast<PaSimpleGetLatencyType>(resolve("pa_simple_get_latency"));
    this->m_paSimpleNew =
        reinterpret_cast<PaSimpleNewType>(resolve("pa_simple_new"));
    this->m_paSimpleRead =
        reinterpret_cast<PaSimpleReadType>(resolve("pa_simple_read"));
    this->m_paSimpleWrite =
        reinterpret_cast<PaSimpleWriteType>(resolve("pa_simple_write"));
    this->m_paStrerror =
        reinterpret_cast<PaStrerrorType>(resolve("pa_strerror"));
#endif
}

void AudioDevPulseAudioPrivate::updateDevices()
{
    this->m_devicesInfo.clear();
    auto mainloop = this->paMainloopNew();

    if (!mainloop) {
        qWarning() << "PulseAudio: failed to create mainloop";

        return;
    }

    auto context = this->paContextNew(this->paMainloopGetApi(mainloop),
                                      "audio-device-monitor");

    if (!context) {
        qWarning() << "PulseAudio: failed to create context";
        this->paMainloopFree(mainloop);

        return;
    }

    this->paContextSetStateCallback(context, contextStateCallback, this);
    this->paContextConnect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    while (this->m_devicesInfo.waitingForSinks
           || this->m_devicesInfo.waitingForSources
           || this->m_devicesInfo.waitingForServer) {
        this->paMainloopIterate(mainloop, 1, nullptr);
    }

    this->paContextDisconnect(context);
    this->paContextUnref(context);
    this->paMainloopFree(mainloop);

    QMutexLocker locker(&this->m_mutex);

    this->m_devicesDescriptions = this->m_devicesInfo.devicesDescriptions;
    this->m_devicesCaps = this->m_devicesInfo.devicesCaps;

    if (this->m_devicesInfo.sources != this->m_sources) {
        this->m_sources = this->m_devicesInfo.sources;
        this->m_mutex.unlock();
        emit self->inputsChanged(this->m_sources);
        this->m_mutex.lock();
    }

    if (this->m_devicesInfo.sinks != this->m_sinks) {
        this->m_sinks = this->m_devicesInfo.sinks;
        this->m_mutex.unlock();
        emit self->outputsChanged(this->m_sinks);
        this->m_mutex.lock();
    }

    if (this->m_devicesInfo.defaultSource != this->m_defaultSource) {
        this->m_defaultSource = this->m_devicesInfo.defaultSource;
        this->m_mutex.unlock();
        emit self->defaultInputChanged(this->m_defaultSource);
        this->m_mutex.lock();
    }

    if (this->m_devicesInfo.defaultSink != this->m_defaultSink) {
        this->m_defaultSink = this->m_devicesInfo.defaultSink;
        this->m_mutex.unlock();
        emit self->defaultOutputChanged(this->m_defaultSink);
        this->m_mutex.lock();
    }
}

void AudioDevPulseAudioPrivate::contextStateCallback(pa_context *context,
                                                     void *userData)
{
    auto self = static_cast<AudioDevPulseAudioPrivate *>(userData);

    switch (self->paContextGetState(context)) {
        case PA_CONTEXT_READY:
            // Request information
            self->paContextGetServerInfo(context, serverInfoCallback, userData);
            self->paContextGetSourceInfoList(context, sourceInfoCallback, userData);
            self->paContextGetSinkInfoList(context, sinkInfoCallback, userData);

            break;

        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            qWarning() << "PulseAudio context failed or terminated";
            self->m_devicesInfo.waitingForSinks = false;
            self->m_devicesInfo.waitingForSources = false;
            self->m_devicesInfo.waitingForServer = false;

            break;

        default:
            break;
    }
}

void AudioDevPulseAudioPrivate::serverInfoCallback(pa_context *context,
                                                   const pa_server_info *info,
                                                   void *userData)
{
    Q_UNUSED(context)
    auto self = static_cast<AudioDevPulseAudioPrivate *>(userData);

    if (info) {
        if (info->default_sink_name)
            self->m_devicesInfo.defaultSink =
                    QString::fromUtf8(info->default_sink_name);

        if (info->default_source_name)
            self->m_devicesInfo.defaultSource =
                    QString::fromUtf8(info->default_source_name);
    }

    self->m_devicesInfo.waitingForServer = false;
}

void AudioDevPulseAudioPrivate::sourceInfoCallback(pa_context *context,
                                                   const pa_source_info *info,
                                                   int isLast,
                                                   void *userData)
{
    Q_UNUSED(context)
    auto self = static_cast<AudioDevPulseAudioPrivate *>(userData);

    if (isLast > 0) {
        self->m_devicesInfo.waitingForSources = false;

        return;
    }

    if (!info)
        return;

    auto name = QString::fromUtf8(info->name);
    auto description = QString::fromUtf8(info->description);

    if (description.isEmpty())
        description = name;

    self->m_devicesInfo.sources << name;
    self->m_devicesInfo.devicesDescriptions[name] = description;
    self->m_devicesInfo.devicesCaps[name] =
            AkAudioCaps(sampleFormats->value(info->sample_spec.format),
                        AkAudioCaps::defaultChannelLayout(info->sample_spec.channels),
                        false,
                        int(info->sample_spec.rate));
}

void AudioDevPulseAudioPrivate::sinkInfoCallback(pa_context *context,
                                                 const pa_sink_info *info,
                                                 int isLast,
                                                 void *userData)
{
    Q_UNUSED(context)
    auto self = static_cast<AudioDevPulseAudioPrivate *>(userData);

    if (isLast > 0) {
        self->m_devicesInfo.waitingForSinks = false;

        return;
    }

    if (!info)
        return;

    auto name = QString::fromUtf8(info->name);
    auto description = QString::fromUtf8(info->description);

    if (description.isEmpty())
        description = name;

    self->m_devicesInfo.sinks << name;
    self->m_devicesInfo.devicesDescriptions[name] = description;
    self->m_devicesInfo.devicesCaps[name] =
            AkAudioCaps(sampleFormats->value(info->sample_spec.format),
                        AkAudioCaps::defaultChannelLayout(info->sample_spec.channels),
                        false,
                        int(info->sample_spec.rate));
}

#include "moc_audiodevpulseaudio.cpp"
