/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QFileInfo>
#include <QtConcurrent>
#include <QThreadPool>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akaudiopacket.h>
#include <akvideopacket.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkImage.h>

#include "mediasourcendkmedia.h"
#include "audiostream.h"
#include "clock.h"
#include "ndkerrormsg.h"
#include "videostream.h"

#if __ANDROID_API__ < 29
#define AMEDIAFORMAT_KEY_FRAME_COUNT "frame-count"
#endif

class Stream
{
    public:
        AkCaps caps;
        QString language;
        bool defaultStream;

        Stream()
        {
        }

        Stream(const AkCaps &caps,
               const QString &language,
               bool defaultStream):
            caps(caps),
            language(language),
            defaultStream(defaultStream)
        {
        }
};

using MediaExtractorPtr = QSharedPointer<AMediaExtractor>;

class MediaSourceNDKMediaPrivate
{
    public:
        MediaSourceNDKMedia *self;
        QFile m_mediaFile;
        QString m_media;
        QList<int> m_streams;
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        MediaExtractorPtr m_mediaExtractor;
        QThreadPool m_threadPool;
        QMutex m_extractMutex;
        QVector<Stream> m_streamInfo;
        QMap<int, AbstractStreamPtr> m_streamsMap;
        Clock m_globalClock;
        qreal m_curClockTime {0.0};
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_loop {false};
        bool m_sync {true};
        bool m_run {false};
        bool m_paused {false};
        bool m_eos {false};
        bool m_showLog {false};

        explicit MediaSourceNDKMediaPrivate(MediaSourceNDKMedia *self);
        qint64 packetQueueSize() const;
        AbstractStreamPtr createStream(int index);
        void readPackets();
        static AkCaps capsFromMediaFormat(AMediaFormat *mediaFormat);
        void updateStreams();
};

MediaSourceNDKMedia::MediaSourceNDKMedia(QObject *parent):
    MediaSource(parent)
{
    this->d = new MediaSourceNDKMediaPrivate(this);

    if (this->d->m_threadPool.maxThreadCount() < 4)
        this->d->m_threadPool.setMaxThreadCount(4);
}

MediaSourceNDKMedia::~MediaSourceNDKMedia()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList MediaSourceNDKMedia::medias() const
{
    QStringList medias;

    if (!this->d->m_media.isEmpty())
        medias << this->d->m_media;

    return medias;
}

QString MediaSourceNDKMedia::media() const
{
    return this->d->m_media;
}

QList<int> MediaSourceNDKMedia::streams() const
{
    return this->d->m_streams;
}

QList<int> MediaSourceNDKMedia::listTracks(AkCaps::CapsType type)
{
    QList<int> tracks;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (type == AkCaps::CapsAny || streamInfo.caps.type() == type)
            tracks << i;

        i++;
    }

    return tracks;
}

QString MediaSourceNDKMedia::streamLanguage(int stream)
{
    return this->d->m_streamInfo.value(stream, Stream()).language;
}

bool MediaSourceNDKMedia::loop() const
{
    return this->d->m_loop;
}

bool MediaSourceNDKMedia::sync() const
{
    return this->d->m_sync;
}

int MediaSourceNDKMedia::defaultStream(AkCaps::CapsType type)
{
    int defaultStream = -1;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (streamInfo.caps.type() == type) {
            if (streamInfo.defaultStream)
                return i;

            if (defaultStream < 0)
                defaultStream = i;
        }

        i++;
    }

    return defaultStream;
}

QString MediaSourceNDKMedia::description(const QString &media) const
{
    if (this->d->m_media != media)
        return {};

    return QFileInfo(media).baseName();
}

AkCaps MediaSourceNDKMedia::caps(int stream)
{
    return this->d->m_streamInfo.value(stream, {}).caps;
}

qint64 MediaSourceNDKMedia::durationMSecs()
{
    bool isStopped = this->d->m_state == AkElement::ElementStateNull;

    if (isStopped)
        this->setState(AkElement::ElementStatePaused);

    int64_t duration = 0;

    if (this->d->m_mediaExtractor) {
        auto extractor = this->d->m_mediaExtractor.data();
        size_t numtracks =
                AMediaExtractor_getTrackCount(extractor);

        for (size_t i = 0; i < numtracks; i++) {
            auto format = AMediaExtractor_getTrackFormat(extractor, i);

            if (!format)
                continue;

            int64_t streamDuration = 0;
            AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &streamDuration);
            duration = qMax(duration, streamDuration);
        }
    }

    if (isStopped)
        this->setState(AkElement::ElementStateNull);

    return duration / 1000;
}

qint64 MediaSourceNDKMedia::currentTimeMSecs()
{
    return qRound64(1e3 * this->d->m_globalClock.clock());
}

qint64 MediaSourceNDKMedia::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

bool MediaSourceNDKMedia::showLog() const
{
    return this->d->m_showLog;
}

AkElement::ElementState MediaSourceNDKMedia::state() const
{
    return this->d->m_state;
}

void MediaSourceNDKMedia::seek(qint64 mSecs,
                               SeekPosition position)
{
    if (this->d->m_state == AkElement::ElementStateNull)
        return;

    int64_t pts = mSecs;

    switch (position) {
    case SeekCur:
        pts += this->currentTimeMSecs();

        break;

    case SeekEnd:
        pts += this->durationMSecs();

        break;

    default:
        break;
    }

    pts = qBound<qint64>(0, pts, this->durationMSecs()) * 1000;

    this->d->m_extractMutex.lock();

    for (auto &stream: this->d->m_streamsMap)
        stream->flush();

    AMediaExtractor_seekTo(this->d->m_mediaExtractor.data(),
                           pts,
                           AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
    this->d->m_globalClock.setClock(qreal(pts) / 1e6);
    this->d->m_extractMutex.unlock();
}

void MediaSourceNDKMedia::setMedia(const QString &media)
{
    if (media == this->d->m_media)
        return;

    auto state = this->d->m_state;
    this->setState(AkElement::ElementStateNull);
    this->d->m_media = media;
    this->d->updateStreams();

    if (!this->d->m_media.isEmpty())
        this->setState(state);

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
    emit this->durationMSecsChanged(this->durationMSecs());
    emit this->mediaLoaded(media);
}

void MediaSourceNDKMedia::setStreams(const QList<int> &streams)
{
    if (this->d->m_streams == streams)
        return;

    this->d->m_streams = streams;
    emit this->streamsChanged(streams);
}

void MediaSourceNDKMedia::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSourceNDKMedia::setShowLog(bool showLog)
{
    if (this->d->m_showLog == showLog)
        return;

    this->d->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void MediaSourceNDKMedia::setLoop(bool loop)
{
    if (this->d->m_loop == loop)
        return;

    this->d->m_loop = loop;
    emit this->loopChanged(loop);
}

void MediaSourceNDKMedia::setSync(bool sync)
{
    if (this->d->m_sync == sync)
        return;

    this->d->m_sync = sync;
    emit this->syncChanged(sync);

    for (auto &stream: this->d->m_streamsMap)
        stream->setSync(sync);
}

void MediaSourceNDKMedia::resetMedia()
{
    this->setMedia("");
}

void MediaSourceNDKMedia::resetStreams()
{
    if  (this->d->m_streams.isEmpty())
        return;

    this->d->m_streams.clear();
    emit this->streamsChanged(this->d->m_streams);
}

void MediaSourceNDKMedia::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSourceNDKMedia::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSourceNDKMedia::resetLoop()
{
    this->setLoop(false);
}

void MediaSourceNDKMedia::resetSync()
{
    this->setSync(true);
}

bool MediaSourceNDKMedia::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
    case AkElement::ElementStateNull: {
        if (state == AkElement::ElementStatePaused
            || state == AkElement::ElementStatePlaying) {
        auto mediaExtractor = AMediaExtractor_new();

            if (!mediaExtractor) {
                qDebug() << "Can't create MediaExtractor";

                return false;
            }

            this->d->m_mediaExtractor =
                    MediaExtractorPtr(mediaExtractor,
                                      [] (AMediaExtractor *mediaExtractor) {
                                        AMediaExtractor_delete(mediaExtractor);
                                      });

            media_status_t status = AMEDIA_ERROR_UNKNOWN;

            if (QFileInfo(this->d->m_media).isFile()
                && QFileInfo::exists(this->d->m_media)) {
                this->d->m_mediaFile.setFileName(this->d->m_media);

                if (!this->d->m_mediaFile.open(QIODevice::ReadOnly)) {
                    this->d->m_mediaExtractor.clear();
                    qDebug() << "Failed to open"
                             << this->d->m_mediaFile.fileName()
                             << ":"
                             << this->d->m_mediaFile.errorString();

                    return false;
                }

                status = AMediaExtractor_setDataSourceFd(this->d->m_mediaExtractor.data(),
                                                         this->d->m_mediaFile.handle(),
                                                         0,
                                                         this->d->m_mediaFile.size());
            } else {
                status = AMediaExtractor_setDataSource(this->d->m_mediaExtractor.data(),
                                                       this->d->m_media.toStdString().c_str());
            }

            if (status != AMEDIA_OK) {
                this->d->m_mediaExtractor.clear();
                this->d->m_mediaFile.close();
                qDebug() << "Failed to set data source to"
                         << this->d->m_media
                         << ":"
                         << mediaStatusToStr(status, "Unknown");

                return false;
            }

            QList<int> filterStreams;

            if (this->d->m_streams.isEmpty())
                filterStreams << this->defaultStream(AkCaps::CapsAudio)
                              << this->defaultStream(AkCaps::CapsVideo);
            else
                filterStreams = this->d->m_streams;

            for (auto &i: filterStreams) {
                auto stream = this->d->createStream(i);

                if (!stream)
                    continue;

                this->d->m_streamsMap[i] = stream;

                QObject::connect(stream.data(),
                                 SIGNAL(oStream(AkPacket)),
                                 this,
                                 SIGNAL(oStream(AkPacket)),
                                 Qt::DirectConnection);
                QObject::connect(stream.data(),
                                 SIGNAL(oStream(AkPacket)),
                                 this,
                                 SLOT(log()));
                QObject::connect(stream.data(),
                                 SIGNAL(eosReached()),
                                 this,
                                 SLOT(doLoop()));

                stream->setState(state);
            }

            this->d->m_curClockTime = 0.0;
            this->d->m_globalClock.setClock(0.0);
            this->d->m_run = true;
            this->d->m_paused = state == AkElement::ElementStatePaused;
            this->d->m_eos = false;
            auto result =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &MediaSourceNDKMediaPrivate::readPackets);
            Q_UNUSED(result)
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_run = false;
            this->d->m_threadPool.waitForDone();

            for (auto &stream: this->d->m_streamsMap)
                stream->setState(state);

            this->d->m_streamsMap.clear();
            this->d->m_mediaExtractor.clear();
            this->d->m_mediaFile.close();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_globalClock.setClock(this->d->m_curClockTime);

            for (auto &stream: this->d->m_streamsMap)
                stream->setState(state);

            this->d->m_paused = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_run = false;
            this->d->m_threadPool.waitForDone();

            for (auto &stream: this->d->m_streamsMap)
                stream->setState(state);

            this->d->m_streamsMap.clear();
            this->d->m_mediaExtractor.clear();
            this->d->m_mediaFile.close();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePaused: {
            this->d->m_curClockTime = this->d->m_globalClock.clock();
            this->d->m_paused = true;

            for (auto &stream: this->d->m_streamsMap)
                stream->setState(state);

            this->d->m_state = state;
            emit this->stateChanged(state);

            break;
        }
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void MediaSourceNDKMedia::doLoop()
{
    this->setState(AkElement::ElementStateNull);

    if (this->d->m_loop)
        this->setState(AkElement::ElementStatePlaying);
}

void MediaSourceNDKMedia::log()
{
    if (!this->d->m_showLog)
        return;

    AbstractStreamPtr audioStream;
    AbstractStreamPtr videoStream;

    for (auto &stream: this->d->m_streamsMap) {
        auto type = stream->type();

        if (type == AkCaps::CapsAudio && !audioStream)
            audioStream = stream;

        if (type == AkCaps::CapsVideo && !videoStream)
            videoStream = stream;

        if (audioStream && videoStream)
            break;
    }

    QString diffType;
    qreal diff;
    qint64 audioQueueSize = 0;
    qint64 videoQueueSize = 0;

    if (audioStream && videoStream) {
        diffType = "A-V";
        diff = audioStream->clockDiff() - videoStream->clockDiff();
        audioQueueSize = audioStream->queueSize();
        videoQueueSize = videoStream->queueSize();
    } else if (audioStream) {
        diffType = "M-A";
        diff = -audioStream->clockDiff();
        audioQueueSize = audioStream->queueSize();
    } else if (videoStream) {
        diffType = "M-V";
        diff = -videoStream->clockDiff();
        videoQueueSize = videoStream->queueSize();
    } else
        return;

    QString logFmt("%1 %2: %3 aq=%4KB vq=%5KB");
    QString log = logFmt.arg(this->d->m_globalClock.clock(), 7, 'f', 2)
                        .arg(diffType)
                        .arg(diff, 7, 'f', 3)
                        .arg(audioQueueSize / 1024, 5)
                        .arg(videoQueueSize / 1024, 5);
    qDebug() << log.toStdString().c_str();
}

MediaSourceNDKMediaPrivate::MediaSourceNDKMediaPrivate(MediaSourceNDKMedia *self):
    self(self)
{

}

qint64 MediaSourceNDKMediaPrivate::packetQueueSize() const
{
    qint64 size = 0;

    for (auto &stream: this->m_streamsMap)
        size += stream->queueSize();

    return size;
}

AbstractStreamPtr MediaSourceNDKMediaPrivate::createStream(int index)
{
    auto mediaExtractor = this->m_mediaExtractor.data();

    if (!mediaExtractor)
        return {};

    auto type = AbstractStream::type(mediaExtractor, uint(index));
    AbstractStreamPtr stream;
    auto id = Ak::id();

    switch (type) {
    case AkCaps::CapsVideo:
        stream = AbstractStreamPtr(new VideoStream(mediaExtractor,
                                                   uint(index),
                                                   id,
                                                   &this->m_globalClock,
                                                   this->m_sync));

        break;

    case AkCaps::CapsAudio:
        stream = AbstractStreamPtr(new AudioStream(mediaExtractor,
                                                   uint(index),
                                                   id,
                                                   &this->m_globalClock,
                                                   this->m_sync));

        break;

    default:
        break;
    }

    return stream;
}

void MediaSourceNDKMediaPrivate::readPackets()
{
    while (this->m_run) {
        if (this->m_paused) {
            QThread::msleep(500);

            continue;
        }

        if (!this->m_eos) {
            auto streamIndex =
                    AMediaExtractor_getSampleTrackIndex(this->m_mediaExtractor.data());

            if (streamIndex < 0) {
                for (auto &stream: this->m_streamsMap) {
                    stream->packetEnqueue(true);

                    while (this->m_run) {
                        auto result = stream->decodeData();

                        if (result != AbstractStream::EnqueueOk)
                            break;
                    }
                }
            } else if (this->m_streamsMap.contains(streamIndex)) {
                auto stream = this->m_streamsMap[streamIndex];
                stream->packetEnqueue();

                while (this->m_run) {
                    auto result = stream->decodeData();

                    if (result != AbstractStream::EnqueueOk)
                        break;
                }
            }

            AMediaExtractor_advance(this->m_mediaExtractor.data());

            for (auto &stream: this->m_streamsMap)
                this->m_eos |= stream->eos();
        }
    }
}

AkCaps MediaSourceNDKMediaPrivate::capsFromMediaFormat(AMediaFormat *mediaFormat)
{
    if (!mediaFormat)
        return {};

    AkCaps caps;
    const char *mime = nullptr;
    AMediaFormat_getString(mediaFormat, AMEDIAFORMAT_KEY_MIME, &mime);

    if (QString(mime).startsWith("audio/")) {
        AkAudioCaps::SampleFormat sampleFormat = AkAudioCaps::SampleFormat_s16;
#if __ANDROID_API__ >= 28
        int32_t pcmEncoding = 0;

        if (AMediaFormat_getInt32(mediaFormat,
                                  AMEDIAFORMAT_KEY_PCM_ENCODING,
                                  &pcmEncoding))
            sampleFormat = AudioStream::sampleFormatFromEncoding(pcmEncoding);
#endif
        AkAudioCaps::ChannelLayout layout = AkAudioCaps::Layout_none;
        int32_t channelMask = 0;

        if (AMediaFormat_getInt32(mediaFormat,
                                  AMEDIAFORMAT_KEY_CHANNEL_MASK,
                                  &channelMask)) {
            layout = AudioStream::layoutFromChannelMask(channelMask);
        } else {
            int32_t channels = 0;
            AMediaFormat_getInt32(mediaFormat,
                                  AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                                  &channels);
            layout = AkAudioCaps::defaultChannelLayout(channels);
        }

        int32_t rate = 0;
        AMediaFormat_getInt32(mediaFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, &rate);
        caps = AkAudioCaps(sampleFormat, layout, false, rate);
    } else if (QString(mime).startsWith("video/")) {
        int32_t width = 0;
        AMediaFormat_getInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, &width);
        int32_t height = 0;
        AMediaFormat_getInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, &height);
        float frameRate = 0.0f;
        AMediaFormat_getFloat(mediaFormat,
                              AMEDIAFORMAT_KEY_FRAME_RATE,
                              &frameRate);

        if (frameRate < 1.0f) {
            int64_t duration = 0;
            AMediaFormat_getInt64(mediaFormat,
                                  AMEDIAFORMAT_KEY_DURATION,
                                  &duration);
            int64_t frameCount = 0;
            AMediaFormat_getInt64(mediaFormat,
                                  AMEDIAFORMAT_KEY_FRAME_COUNT,
                                  &frameCount);
            frameRate = duration > 0.0f?
                            1.0e6f * frameCount / duration:
                            0.0f;
        }

        if (frameRate < 1.0f)
            frameRate = DEFAULT_FRAMERATE;

        caps = AkVideoCaps(AkVideoCaps::Format_rgb24,
                           width,
                           height,
                           AkFrac(qRound64(1000 * frameRate), 1000));
    }

    return caps;
}

void MediaSourceNDKMediaPrivate::updateStreams()
{
    this->m_streamInfo.clear();

    if (this->m_media.isEmpty())
        return;

    auto extractor = AMediaExtractor_new();

    if (!extractor)
        return;

    QFile mediaFile;
    media_status_t status = AMEDIA_ERROR_UNKNOWN;

    if (QFileInfo(this->m_media).isFile()
        && QFileInfo::exists(this->m_media)) {
        mediaFile.setFileName(this->m_media);

        if (!mediaFile.open(QIODevice::ReadOnly)) {
            AMediaExtractor_delete(extractor);
            qDebug() << "Failed to open"
                     << mediaFile.fileName()
                     << ":"
                     << mediaFile.errorString();

            return;
        }

        status = AMediaExtractor_setDataSourceFd(extractor,
                                                 mediaFile.handle(),
                                                 0,
                                                 mediaFile.size());
    } else {
        status = AMediaExtractor_setDataSource(extractor,
                                               this->m_media.toStdString().c_str());
    }

    if (status != AMEDIA_OK) {
        AMediaExtractor_delete(extractor);
        qDebug() << "Failed to set data source to"
                 << this->m_media
                 << ":"
                 << mediaStatusToStr(status, "Unknown");

        return;
    }

    for (size_t i = 0; i < AMediaExtractor_getTrackCount(extractor); i++) {
        auto format = AMediaExtractor_getTrackFormat(extractor, i);

        if (!format)
            continue;

        auto caps = MediaSourceNDKMediaPrivate::capsFromMediaFormat(format);

        if (caps) {
            int32_t isDefault = false;
            AMediaFormat_getInt32(format,
                                  AMEDIAFORMAT_KEY_IS_DEFAULT,
                                  &isDefault);
            const char *language = nullptr;
            AMediaFormat_getString(format,
                                   AMEDIAFORMAT_KEY_LANGUAGE,
                                   &language);
            this->m_streamInfo << Stream(caps,
                                         language?
                                             QString(language): QString(),
                                         isDefault);
        }

        AMediaFormat_delete(format);
    }

    AMediaExtractor_delete(extractor);
}

#include "moc_mediasourcendkmedia.cpp"
