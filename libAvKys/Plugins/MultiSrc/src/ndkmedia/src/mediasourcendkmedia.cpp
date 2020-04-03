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
#include "stream.h"
#include "videostream.h"

using MediaExtractorPtr = QSharedPointer<AMediaExtractor>;

class MediaSourceNDKMediaPrivate
{
    public:
        MediaSourceNDKMedia *self;
        QString m_media;
        QList<int> m_streams;
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        MediaExtractorPtr m_mediaExtractor;
        QThreadPool m_threadPool;
        QVector<Stream> m_streamInfo;
        QMap<int, AbstractStreamPtr> m_streamsMap;
        Clock m_globalClock;
        qreal m_curClockTime {0.0};
        QFuture<void> m_multimediaLoopResult;
        AkElement::ElementState m_curState {AkElement::ElementStateNull};
        bool m_paused {false};
        bool m_loop {false};
        bool m_run {false};
        bool m_showLog {false};

        explicit MediaSourceNDKMediaPrivate(MediaSourceNDKMedia *self);
        AbstractStreamPtr createStream(AMediaExtractor *mediaExtractor,
                                       int index);
        void multimediaLoop();
        static AkCaps capsFromMediaFormat(AMediaFormat *mediaFormat);
        void updateStreams();
        void flush();
};

MediaSourceNDKMedia::MediaSourceNDKMedia(QObject *parent):
    MediaSource(parent)
{
    this->d = new MediaSourceNDKMediaPrivate(this);

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
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

QList<int> MediaSourceNDKMedia::listTracks(const QString &mimeType)
{
    QList<int> tracks;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (mimeType.isEmpty() || streamInfo.caps.mimeType() == mimeType)
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

int MediaSourceNDKMedia::defaultStream(const QString &mimeType)
{
    int defaultStream = -1;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (streamInfo.caps.mimeType() == mimeType) {
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
        return QString();

    return QFileInfo(media).baseName();
}

AkCaps MediaSourceNDKMedia::caps(int stream)
{
    return this->d->m_streamInfo.value(stream, Stream()).caps;
}

qint64 MediaSourceNDKMedia::durationMSecs()
{
    bool isStopped = this->d->m_curState == AkElement::ElementStateNull;

    if (isStopped)
        this->setState(AkElement::ElementStatePaused);

    int64_t duration = 0;

    if (this->d->m_mediaExtractor) {
        auto extractor = this->d->m_mediaExtractor.data();
        size_t numtracks =
                AMediaExtractor_getTrackCount(extractor);

        for (size_t i = 0; i < numtracks; i++) {
            auto format = AMediaExtractor_getTrackFormat(extractor, i);
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

void MediaSourceNDKMedia::seek(qint64 mSecs,
                               MultiSrcElement::SeekPosition position)
{
    if (this->d->m_curState == AkElement::ElementStateNull)
        return;

    bool isPlaying = this->d->m_curState == AkElement::ElementStatePlaying;

    if (isPlaying)
        this->setState(AkElement::ElementStatePaused);

    int64_t pts = mSecs;

    switch (position) {
    case MultiSrcElement::SeekCur:
        pts += this->currentTimeMSecs();

        break;

    case MultiSrcElement::SeekEnd:
        pts += this->durationMSecs();

        break;

    default:
        break;
    }

    this->d->flush();
    pts = qBound<qint64>(0, pts, this->durationMSecs()) * 1000;
    AMediaExtractor_seekTo(this->d->m_mediaExtractor.data(),
                           pts,
                           AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
    this->d->m_globalClock.setClock(qreal(pts) / 1e6);

    if (isPlaying)
        this->setState(AkElement::ElementStatePlaying);
}

void MediaSourceNDKMedia::setMedia(const QString &media)
{
    if (media == this->d->m_media)
        return;

    bool isRunning = this->d->m_run;
    this->setState(AkElement::ElementStateNull);
    this->d->m_media = media;
    this->d->updateStreams();

    if (isRunning && !this->d->m_media.isEmpty())
        this->setState(AkElement::ElementStatePlaying);

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
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

bool MediaSourceNDKMedia::setState(AkElement::ElementState state)
{
    switch (this->d->m_curState) {
    case AkElement::ElementStateNull: {
        if (state == AkElement::ElementStatePaused
            || state == AkElement::ElementStatePlaying) {
            this->d->m_mediaExtractor =
                    MediaExtractorPtr(AMediaExtractor_new(),
                                      [] (AMediaExtractor *mediaExtractor) {
                                        AMediaExtractor_delete(mediaExtractor);
                                      });

            if (AMediaExtractor_setDataSource(this->d->m_mediaExtractor.data(),
                                              this->d->m_media.toStdString().c_str()) != AMEDIA_OK) {
                return false;
            }

            QList<int> filterStreams;

            if (this->d->m_streams.isEmpty())
                filterStreams << this->defaultStream("audio/x-raw")
                              << this->defaultStream("video/x-raw");
            else
                filterStreams = this->d->m_streams;

            for (auto &i: filterStreams) {
                auto stream =
                        this->d->createStream(this->d->m_mediaExtractor.data(),
                                              i);

                if (stream) {
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
                                     SIGNAL(eof()),
                                     this,
                                     SLOT(doLoop()));

                    stream->init();
                }
            }

            if (state == AkElement::ElementStatePaused)
                this->d->m_curClockTime = 0.;

            this->d->m_globalClock.setClock(0.);
            this->d->m_run = true;
            this->d->m_paused = state == AkElement::ElementStatePaused;
            this->d->m_multimediaLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &MediaSourceNDKMediaPrivate::multimediaLoop);
            this->d->m_curState = state;

            return true;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_globalClock.setClock(this->d->m_curClockTime);
            this->d->m_run = false;
            this->d->m_threadPool.waitForDone();
            this->d->m_streamsMap.clear();
            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_globalClock.setClock(this->d->m_curClockTime);
            this->d->m_paused = false;
            this->d->m_curState = state;

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
            this->d->m_multimediaLoopResult.waitForFinished();
            this->d->m_streamsMap.clear();
            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePaused: {
            this->d->m_curClockTime = this->d->m_globalClock.clock();
            this->d->m_paused = true;
            this->d->m_curState = state;

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
    this->setState(AkElement::ElementStatePlaying);
}

void MediaSourceNDKMedia::packetConsumed()
{
}

void MediaSourceNDKMedia::log()
{
    if (!this->d->m_showLog)
        return;

    AbstractStreamPtr audioStream;
    AbstractStreamPtr videoStream;

    for (auto &stream: this->d->m_streamsMap) {
        auto mimeType = stream->mimeType();

        if (mimeType == "audio/x-raw" && !audioStream)
            audioStream = stream;

        if (mimeType == "video/x-raw" && !videoStream)
            videoStream = stream;

        if (audioStream && videoStream)
            break;
    }

    QString diffType;
    qreal diff;

    if (audioStream && videoStream) {
        diffType = "A-V";
        diff = audioStream->clockDiff() - videoStream->clockDiff();
    } else if (audioStream) {
        diffType = "M-A";
        diff = -audioStream->clockDiff();
    } else if (videoStream) {
        diffType = "M-V";
        diff = -videoStream->clockDiff();
    } else
        return;

    QString logFmt("%1 %2: %3");
    QString log = logFmt.arg(this->d->m_globalClock.clock(), 7, 'f', 2)
                        .arg(diffType)
                        .arg(diff, 7, 'f', 3);

    qDebug() << log.toStdString().c_str();
}

MediaSourceNDKMediaPrivate::MediaSourceNDKMediaPrivate(MediaSourceNDKMedia *self):
    self(self)
{

}

AbstractStreamPtr MediaSourceNDKMediaPrivate::createStream(AMediaExtractor *mediaExtractor,
                                                           int index)
{
    auto type = AbstractStream::mimeType(mediaExtractor, uint(index));
    AbstractStreamPtr stream;
    auto id = Ak::id();

    if (type == "video/x-raw")
        stream = AbstractStreamPtr(new VideoStream(mediaExtractor,
                                                   uint(index),
                                                   id,
                                                   &this->m_globalClock));
    else if (type == "audio/x-raw")
        stream = AbstractStreamPtr(new AudioStream(mediaExtractor,
                                                   uint(index),
                                                   id,
                                                   &this->m_globalClock));
    else
        stream = AbstractStreamPtr(new AbstractStream(mediaExtractor,
                                                      uint(index),
                                                      id,
                                                      &this->m_globalClock));

    return stream;
}

void MediaSourceNDKMediaPrivate::multimediaLoop()
{
    bool eos = false;

    while (this->m_run) {
        if (this->m_paused) {
            QThread::msleep(500);

            continue;
        }

        if (!eos) {
            auto streamIndex =
                    AMediaExtractor_getSampleTrackIndex(this->m_mediaExtractor.data());

            if (streamIndex < 0) {
                for (auto &stream: this->m_streamsMap)
                    stream->packetEnqueue(true);
            } else {
                if (this->m_streamsMap.contains(streamIndex)
                    && (this->m_streams.isEmpty()
                        || this->m_streams.contains(streamIndex))) {
                    this->m_streamsMap[streamIndex]->packetEnqueue();
                }
            }
        }

        for (auto &stream: this->m_streamsMap)
            stream->decodeData();

        if (!eos)
            eos = !AMediaExtractor_advance(this->m_mediaExtractor.data());
    }

    for (auto &stream: this->m_streamsMap)
        stream->uninit();

    this->m_streamsMap.clear();
}

AkCaps MediaSourceNDKMediaPrivate::capsFromMediaFormat(AMediaFormat *mediaFormat)
{
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
        caps = AkAudioCaps(sampleFormat, layout, rate);
    } else if (QString(mime).startsWith("video/")) {
        int32_t width = 0;
        AMediaFormat_getInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, &width);
        int32_t height = 0;
        AMediaFormat_getInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, &height);
        int32_t frameRate;
        AMediaFormat_getInt32(mediaFormat,
                              AMEDIAFORMAT_KEY_FRAME_RATE,
                              &frameRate);
        caps = AkVideoCaps(AkVideoCaps::Format_rgb24,
                           width,
                           height,
                           AkFrac(frameRate, 1));
    }

    return caps;
}

void MediaSourceNDKMediaPrivate::updateStreams()
{
    this->m_streamInfo.clear();

    if (this->m_media.isEmpty())
        return;

    auto extractor = AMediaExtractor_new();

    if (AMediaExtractor_setDataSource(extractor,
                                      this->m_media.toStdString().c_str()) == AMEDIA_OK) {
        for (size_t i = 0; i < AMediaExtractor_getTrackCount(extractor); i++) {
            auto format = AMediaExtractor_getTrackFormat(extractor, i);
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
    }

    AMediaExtractor_delete(extractor);
}

void MediaSourceNDKMediaPrivate::flush()
{
    for (auto &stream: this->m_streamsMap)
        stream->flush();
}

#include "moc_mediasourcendkmedia.cpp"
