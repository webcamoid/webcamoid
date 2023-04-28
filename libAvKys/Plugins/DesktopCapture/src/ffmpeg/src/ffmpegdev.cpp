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

#include <QApplication>
#include <QScreen>
#include <QTime>
#include <QtConcurrent>
#include <QThreadPool>
#include <QFuture>
#include <QMutex>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#ifdef HAVE_LIBAVDEVICE
#include <libavdevice/avdevice.h>
#endif
}

#include "ffmpegdev.h"

Q_GLOBAL_STATIC(QStringList, avfoundationDevices)

class FFmpegDevPrivate
{
    public:
        FFmpegDev *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCaps> m_devicesCaps;
        AVFormatContext *m_inputContext {nullptr};
        AVCodecContext *m_codecContext {nullptr};
        const AVCodec *m_codec {nullptr};
        AVDictionary *m_codecOptions {nullptr};
        AVStream *m_stream {nullptr};
        SwsContext *m_scaleContext {nullptr};
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        AkPacket m_curPacket;
        bool m_run {false};
        bool m_threadedRead {true};

        explicit FFmpegDevPrivate(FFmpegDev *self);
        QStringList listAVFoundationDevices() const;
        QSize screenSize(const QString &format, const QString &input) const;
        void setupGeometrySignals();
        AkFrac fps() const;
        AkFrac timeBase() const;
        AkVideoPacket convert(AVFrame *iFrame);
        void readPackets();
        void readPacket();
        void sendPacket(const AkPacket &packet);
        void updateDevices();
};

FFmpegDev::FFmpegDev():
    ScreenDev()
{
#ifdef HAVE_LIBAVDEVICE
    avdevice_register_all();
#endif

    this->d = new FFmpegDevPrivate(this);

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif

    this->d->setupGeometrySignals();
    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     [=]() {
                         this->d->setupGeometrySignals();
                         this->d->updateDevices();
                     });
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     [=]() {
                         this->d->setupGeometrySignals();
                         this->d->updateDevices();
                     });

    this->d->updateDevices();
}

FFmpegDev::~FFmpegDev()
{
    this->uninit();
    delete this->d;
}

AkFrac FFmpegDev::fps() const
{
    return this->d->m_fps;
}

QStringList FFmpegDev::medias()
{
    return this->d->m_devices;
}

QString FFmpegDev::media() const
{
    return this->d->m_device;
}

QList<int> FFmpegDev::streams() const
{
    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (!caps)
        return {};

    return {0};
}

int FFmpegDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString FFmpegDev::description(const QString &media)
{
    return this->d->m_descriptions.value(media);
}

AkVideoCaps FFmpegDev::caps(int stream)
{
    Q_UNUSED(stream)

    return this->d->m_devicesCaps.value(this->d->m_device);
}

void FFmpegDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);
}

void FFmpegDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void FFmpegDev::setMedia(const QString &media)
{
    if (this->d->m_device == media)
        return;

    this->d->m_device = media;
    emit this->mediaChanged(media);
}

void FFmpegDev::resetMedia()
{
    QString defaultMedia;

#ifdef Q_OS_WIN32
    auto screenSize = this->d->screenSize("gdigrab", "desktop");

    if (!screenSize.isEmpty())
        defaultMedia = "screen://desktop";
#elif defined(Q_OS_OSX)
    auto devices = this->d->listAVFoundationDevices();

    if (!devices.isEmpty())
        defaultMedia = QString("screen://%1").arg(devices.first());
#elif defined(Q_OS_UNIX)
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
    defaultMedia = QString("screen://%1").arg(screen);
#endif

    this->setMedia(defaultMedia);
}

void FFmpegDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void FFmpegDev::resetStreams()
{

}

bool FFmpegDev::init()
{
    auto device = this->d->m_device;
    device.remove("screen://");

#ifdef Q_OS_UNIX
    device = ":" + device;
#endif

#ifdef Q_OS_WIN32
    auto inputFormat = av_find_input_format("gdigrab");
#elif defined(Q_OS_OSX)
    auto inputFormat = av_find_input_format("avfoundation");
#elif defined(Q_OS_UNIX)
    auto inputFormat = av_find_input_format("x11grab");
#else
    const AVInputFormat *inputFormat = nullptr;
#endif

    if (!inputFormat)
        return false;

    AVDictionary *inputOptions = nullptr;

    this->d->m_mutex.lock();
    auto fps = this->d->m_fps;
    this->d->m_mutex.unlock();
    av_dict_set(&inputOptions, "framerate", fps.toString().toStdString().c_str(), 0);

    static const bool showCursor = false;
    char showCursorStr[8];

    if (showCursor)
        snprintf(showCursorStr, 8, "%s", "1");
    else
        snprintf(showCursorStr, 8, "%s", "0");

#ifdef Q_OS_WIN32
    av_dict_set(&inputOptions, "draw_mouse", showCursorStr, 0);
#elif defined(Q_OS_OSX)
    av_dict_set(&inputOptions, "capture_cursor", showCursorStr, 0);
#elif defined(Q_OS_UNIX)
    av_dict_set(&inputOptions, "draw_mouse", showCursorStr, 0);
#endif

    avformat_open_input(&this->d->m_inputContext,
                        device.toStdString().c_str(),
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 16, 100)
                        inputFormat,
#else
                        const_cast<AVInputFormat *>(inputFormat),
#endif
                        &inputOptions);

    if (inputOptions)
        av_dict_free(&inputOptions);

    if (!this->d->m_inputContext)
        return false;

    if (avformat_find_stream_info(this->d->m_inputContext, nullptr) >= 0)
        for (uint i = 0; i < this->d->m_inputContext->nb_streams; i++) {
            auto stream = this->d->m_inputContext->streams[i];
            auto codecParams = stream->codecpar;

            if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
                this->d->m_codecContext = avcodec_alloc_context3(nullptr);

                if (avcodec_parameters_to_context(this->d->m_codecContext,
                                                  codecParams) < 0) {
                    avcodec_free_context(&this->d->m_codecContext);
                    avformat_close_input(&this->d->m_inputContext);

                    return false;
                }

                this->d->m_codecContext->workaround_bugs = 1;
                this->d->m_codecContext->idct_algo = FF_IDCT_AUTO;
                this->d->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

#ifdef CODEC_FLAG_EMU_EDGE
                if (this->d->m_codec->capabilities & CODEC_CAP_DR1)
                    this->d->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

                av_dict_set(&this->d->m_codecOptions, "refcounted_frames", "0", 0);
                this->d->m_codec = avcodec_find_decoder(this->d->m_codecContext->codec_id);

                if (avcodec_open2(this->d->m_codecContext,
                                  this->d->m_codec,
                                  &this->d->m_codecOptions) < 0) {
                    av_dict_free(&this->d->m_codecOptions);
                    avcodec_free_context(&this->d->m_codecContext);
                    avformat_close_input(&this->d->m_inputContext);

                    return false;
                }

                this->d->m_stream = stream;

                break;
            }
        }

    this->d->m_id = Ak::id();
    this->d->m_run = true;
    auto result = QtConcurrent::run(&this->d->m_threadPool,
                                    this->d,
                                    &FFmpegDevPrivate::readPackets);
    Q_UNUSED(result)

    return true;
}

bool FFmpegDev::uninit()
{
    this->d->m_run = false;
    this->d->m_threadPool.waitForDone();

    if (this->d->m_scaleContext) {
        sws_freeContext(this->d->m_scaleContext);
        this->d->m_scaleContext = nullptr;
    }

    if (this->d->m_codecOptions) {
        av_dict_free(&this->d->m_codecOptions);
        this->d->m_codecOptions = nullptr;
    }

    if (this->d->m_codecContext) {
        avcodec_free_context(&this->d->m_codecContext);
        this->d->m_codecContext = nullptr;
    }

    if (this->d->m_inputContext) {
        avformat_close_input(&this->d->m_inputContext);
        this->d->m_inputContext = nullptr;
    }

    return true;
}

FFmpegDevPrivate::FFmpegDevPrivate(FFmpegDev *self):
    self(self)
{
}

QStringList FFmpegDevPrivate::listAVFoundationDevices() const
{
    auto inputFormat = av_find_input_format("avfoundation");

    if (!inputFormat)
        return {};

    avfoundationDevices->clear();
    AVFormatContext *inputContext = nullptr;
    AVDictionary *inputOptions = nullptr;
    av_dict_set(&inputOptions, "list_devices", "true", 0);

    av_log_set_callback([] (void *avcl,
                            int level,
                            const char *fmt,
                            va_list vl) {
        Q_UNUSED(avcl)

        if (level == AV_LOG_INFO) {
            static const size_t logSize = 1024;
            char log[logSize];
            vsnprintf(log, logSize, fmt, vl);

            QRegularExpression re("^.*\\[(\\d+)\\] Capture screen \\d+.*$");
            auto match = re.match(log);

            if (match.hasMatch())
                *avfoundationDevices << match.captured(1);
        }
    });

    avformat_open_input(&inputContext, "", inputFormat, &inputOptions);
    av_log_set_callback(av_log_default_callback);
    av_dict_free(&inputOptions);

    if (inputContext)
        avformat_close_input(&inputContext);

    return *avfoundationDevices;
}

QSize FFmpegDevPrivate::screenSize(const QString &format, const QString &input) const
{
    QSize screenSize;
    auto inputFormat = av_find_input_format(format.toStdString().c_str());

    if (!inputFormat)
        return {};

    AVFormatContext *inputContext = nullptr;
    AVDictionary *inputOptions = nullptr;
    avformat_open_input(&inputContext,
                        input.toStdString().c_str(),
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 16, 100)
                        inputFormat,
#else
                        const_cast<AVInputFormat *>(inputFormat),
#endif
                        &inputOptions);

    if (inputOptions)
        av_dict_free(&inputOptions);

    if (inputContext) {
        if (avformat_find_stream_info(inputContext, nullptr) >= 0)
            for (uint i = 0; i < inputContext->nb_streams; i++) {
                auto codecParams = inputContext->streams[i]->codecpar;

                if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
                    screenSize = {codecParams->width, codecParams->height};

                    break;
                }
            }

        avformat_close_input(&inputContext);
    }

    return screenSize;
}

void FFmpegDevPrivate::setupGeometrySignals()
{
    size_t i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         [=]() { this->updateDevices(); });
        i++;
    }
}

AkFrac FFmpegDevPrivate::fps() const
{
    if (!this->m_stream)
        return {};

    AkFrac fps;

    if (this->m_stream->avg_frame_rate.num
        && this->m_stream->avg_frame_rate.den)
        fps = AkFrac(this->m_stream->avg_frame_rate.num,
                     this->m_stream->avg_frame_rate.den);
    else
        fps = AkFrac(this->m_stream->r_frame_rate.num,
                     this->m_stream->r_frame_rate.den);

    return fps;
}

AkFrac FFmpegDevPrivate::timeBase() const
{
    if (!this->m_stream)
        return {};

    return {this->m_stream->time_base.num,
            this->m_stream->time_base.den};
}

AkVideoPacket FFmpegDevPrivate::convert(AVFrame *iFrame)
{
    static const AVPixelFormat outPixFormat = AV_PIX_FMT_RGB24;

    // Initialize rescaling context.
    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                iFrame->width,
                                                iFrame->height,
                                                AVPixelFormat(iFrame->format),
                                                iFrame->width,
                                                iFrame->height,
                                                outPixFormat,
                                                SWS_FAST_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    if (!this->m_scaleContext)
        return {};

    // Create oPicture
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_image_alloc(oFrame.data,
                       oFrame.linesize,
                       iFrame->width,
                       iFrame->height,
                       outPixFormat,
                       1) < 1)
        return {};

    // Convert picture format
    sws_scale(this->m_scaleContext,
              iFrame->data,
              iFrame->linesize,
              0,
              iFrame->height,
              oFrame.data,
              oFrame.linesize);

    // Create packet
    auto nPlanes = av_pix_fmt_count_planes(AVPixelFormat(iFrame->format));
    AkVideoCaps caps(AkVideoCaps::Format_rgb24,
                     iFrame->width,
                     iFrame->height,
                     this->fps());
    AkVideoPacket oPacket(caps);

    for (int plane = 0; plane < nPlanes; ++plane) {
        auto planeData = oFrame.data[plane];
        auto oLineSize = oFrame.linesize[plane];
        auto lineSize = qMin<size_t>(oPacket.lineSize(plane), oLineSize);
        auto heightDiv = oPacket.heightDiv(plane);

        for (int y = 0; y < iFrame->height; ++y) {
            auto ys = y >> heightDiv;
            memcpy(oPacket.line(plane, y),
                   planeData + ys * oLineSize,
                   lineSize);
        }
    }

    oPacket.setId(this->m_id);
    oPacket.setPts(iFrame->pts);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(0);
    av_freep(&oFrame.data[0]);

    return oPacket;
}

void FFmpegDevPrivate::readPackets()
{
    while (this->m_run)
        this->readPacket();
}

void FFmpegDevPrivate::readPacket()
{
    auto packet = av_packet_alloc();

    if (av_read_frame(this->m_inputContext, packet) >= 0) {
        avcodec_send_packet(this->m_codecContext, packet);

        while (this->m_run) {
            auto iFrame = av_frame_alloc();
            int r = avcodec_receive_frame(this->m_codecContext, iFrame);

            if (r >= 0) {
                auto packet = this->convert(iFrame);

                if (this->m_threadedRead) {
                    if (!this->m_threadStatus.isRunning()) {
                        this->m_curPacket = packet;

                        this->m_threadStatus =
                            QtConcurrent::run(&this->m_threadPool,
                                              this,
                                              &FFmpegDevPrivate::sendPacket,
                                              this->m_curPacket);
                    }
                } else {
                    emit self->oStream(packet);
                }
            }

            av_frame_free(&iFrame);

            if (r < 0)
                break;
        }

        av_packet_unref(packet);
        av_packet_free(&packet);
    }
}

void FFmpegDevPrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
}

void FFmpegDevPrivate::updateDevices()
{
    decltype(this->m_device) device;
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

#ifdef Q_OS_WIN32
    auto screenSize = this->screenSize("gdigrab", "desktop");

    if (!screenSize.isEmpty()) {
        auto deviceId = "screen://desktop";
        devices << deviceId;
        device = deviceId;
        descriptions[deviceId] = QString("Screen 0");
        devicesCaps[deviceId] = AkVideoCaps(AkVideoCaps::Format_rgb24,
                                            screenSize.width(),
                                            screenSize.height(),
                                            this->m_fps);
    }
#elif defined(Q_OS_OSX)
    int i = 0;

    for (auto &dev: this->listAVFoundationDevices()) {
        auto screenSize = this->screenSize("avfoundation", dev);

        if (!screenSize.isEmpty()) {
            auto deviceId = QString("screen://%1").arg(dev);
            devices << deviceId;
            descriptions[deviceId] = QString("Screen %1").arg(i);
            devicesCaps[deviceId] = AkVideoCaps(AkVideoCaps::Format_rgb24,
                                                screenSize.width(),
                                                screenSize.height(),
                                                this->m_fps);

            if (device.isEmpty())
                device = deviceId;

            i++;
        }
    }
#elif defined(Q_OS_UNIX)
    int i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        auto deviceId = QString("screen://%1.0").arg(i);
        devices << deviceId;
        descriptions[deviceId] = QString("Screen %1").arg(i);
        devicesCaps[deviceId] = AkVideoCaps(AkVideoCaps::Format_rgb24,
                                            screen->size().width(),
                                            screen->size().height(),
                                            this->m_fps);

        if (screen == QGuiApplication::primaryScreen())
            device = deviceId;

        i++;
    }
#endif

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->mediasChanged(devices);
    }

    if (!this->m_devices.contains(this->m_device)) {
        this->m_device = device;
        emit self->mediaChanged(device);
    }
}

#include "moc_ffmpegdev.cpp"
