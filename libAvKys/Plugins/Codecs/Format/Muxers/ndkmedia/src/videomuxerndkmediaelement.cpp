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

#include <QMutex>
#include <QQueue>
#include <QTemporaryFile>
#include <QThread>
#include <QVariant>
#include <QWaitCondition>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akvideocaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>
#include <media/NdkMediaMuxer.h>

#include "videomuxerndkmediaelement.h"

// Custom audio codecs

#define AudioCodecID_amvorbis AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'V', 'O', 'R'))
#define AudioCodecID_amopus   AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'O', 'P', 'U'))
#define AudioCodecID_amaac    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'A', 'A', 'C'))

// Custom video codecs

#define VideoCodecID_amvp8  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xA, 'V', 'P', '8'))
#define VideoCodecID_amvp9  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xA, 'V', 'P', '9'))
#define VideoCodecID_amav1  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xA, 'A', 'V', '1'))
#define VideoCodecID_amh264 AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xA, 'A', 'V', 'C'))
#define VideoCodecID_amhevc AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xA, 'H', 'E', 'V'))

struct VideoMuxer
{
    const char *muxer;
    OutputFormat amType;
    const char *description;
    const char *extension;
    AkVideoMuxer::FormatID formatID;
    AkCompressedAudioCaps::AudioCodecID audioCodecs[16];
    AkCompressedVideoCaps::VideoCodecID videoCodecs[16];

    inline static const VideoMuxer *table()
    {
        static const VideoMuxer ndkmediaMuxerFormatsTable[] = {
            {"webm", AMEDIAMUXER_OUTPUT_FORMAT_WEBM, "Webm (Android Media)", "webm", AkVideoMuxer::FormatID_webm,
                {AudioCodecID_amvorbis,
                 AudioCodecID_amopus,
                 AkCompressedAudioCaps::AudioCodecID_unknown},
                {VideoCodecID_amvp8,
                 VideoCodecID_amvp9,
                 VideoCodecID_amav1,
                 AkCompressedVideoCaps::VideoCodecID_unknown}},
            {"mp4", AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4, "MP4 (Android Media)", "mp4", AkVideoMuxer::FormatID_mp4,
                {AudioCodecID_amaac,
                 AkCompressedAudioCaps::AudioCodecID_unknown},
                {VideoCodecID_amh264,
                 VideoCodecID_amhevc,
                 AkCompressedVideoCaps::VideoCodecID_unknown}},
            {nullptr, OutputFormat(-1), nullptr, nullptr, AkVideoMuxer::FormatID_unknown,
                {AkCompressedAudioCaps::AudioCodecID_unknown},
                {AkCompressedVideoCaps::VideoCodecID_unknown}},
        };

        return ndkmediaMuxerFormatsTable;
    }
};

class VideoMuxerNDKMediaElementPrivate
{
    public:
        VideoMuxerNDKMediaElement *self;
        QFile m_outputFile;
        QVector<VideoMuxer> m_muxers;
        AMediaMuxer *m_muxer {nullptr};
        QMutex m_mutex;
        bool m_initialized {false};
        bool m_paused {false};
        bool m_recordAudio {false};
        bool m_audioTrackReady {false};
        bool m_videoTrackReady {false};

        int64_t m_packetPos {0};
        ssize_t m_audioTrack {-1};
        ssize_t m_videoTrack {-1};
        QQueue<AkPacket> m_packets;
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerNDKMediaElementPrivate(VideoMuxerNDKMediaElement *self);
        ~VideoMuxerNDKMediaElementPrivate();
        static const char *errorToStr(media_status_t status);
        void listMuxers();
        bool addTrackStart();
        bool init();
        void uninit();
        void packetReady(const AkPacket &packet);
};

VideoMuxerNDKMediaElement::VideoMuxerNDKMediaElement():
    AkVideoMuxer()
{
    this->d = new VideoMuxerNDKMediaElementPrivate(this);
    this->d->listMuxers();
    this->setMuxer(this->muxers().value(0));
}

VideoMuxerNDKMediaElement::~VideoMuxerNDKMediaElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoMuxerNDKMediaElement::muxers() const
{
    QStringList muxers;

    for (auto muxer = VideoMuxer::table(); muxer->muxer; ++muxer)
        muxers << muxer->muxer;

    return muxers;
}

AkVideoMuxer::FormatID VideoMuxerNDKMediaElement::formatID(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const VideoMuxer &formatMuxer) -> bool {
        return formatMuxer.muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return AkVideoMuxer::FormatID_unknown;

    return it->formatID;
}

QString VideoMuxerNDKMediaElement::description(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const VideoMuxer &formatMuxer) -> bool {
        return formatMuxer.muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    return {it->description};
}

QString VideoMuxerNDKMediaElement::extension(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const VideoMuxer &formatMuxer) -> bool {
        return formatMuxer.muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    return {it->extension};
}

bool VideoMuxerNDKMediaElement::gapsAllowed(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return true;

    case AkCompressedCaps::CapsType_Video:
        return true;

    default:
        break;
    }

    return true;
}

QList<AkCodecID> VideoMuxerNDKMediaElement::supportedCodecs(const QString &muxer,
                                                            AkCodecType type) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const VideoMuxer &formatMuxer) -> bool {
        return formatMuxer.muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    QList<AkCodecID> codecs;

    if (type == AkCompressedCaps::CapsType_Audio
        || type == AkCompressedCaps::CapsType_Unknown) {
        for (auto codec = it->audioCodecs;
             *codec != AkCompressedAudioCaps::AudioCodecID::AudioCodecID_unknown;
             ++codec)
            codecs << *codec;
    }

    if (type == AkCompressedCaps::CapsType_Video
        || type == AkCompressedCaps::CapsType_Unknown) {
        for (auto codec = it->videoCodecs;
             *codec != AkCompressedVideoCaps::VideoCodecID::VideoCodecID_unknown;
             ++codec)
            codecs << *codec;
    }

    return codecs;
 }

AkCodecID VideoMuxerNDKMediaElement::defaultCodec(const QString &muxer,
                                                AkCodecType type) const
{
    auto codecs = this->supportedCodecs(muxer, type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

AkPacket VideoMuxerNDKMediaElement::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerNDKMediaElement::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_paused = true;

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_paused = false;

            if (!this->d->init())
                return false;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_paused = false;

            if (!this->d->m_initialized && !this->d->init())
                return false;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->d->m_paused = true;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

VideoMuxerNDKMediaElementPrivate::VideoMuxerNDKMediaElementPrivate(VideoMuxerNDKMediaElement *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerNDKMediaElementPrivate::~VideoMuxerNDKMediaElementPrivate()
{

}

const char *VideoMuxerNDKMediaElementPrivate::errorToStr(media_status_t status)
{
    static const struct
    {
        media_status_t status;
        const char *str;
    } audioNkmEncErrorsStr[] = {
        {AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE, "INSUFFICIENT_RESOURCE"        },
        {AMEDIACODEC_ERROR_RECLAIMED            , "ERROR_RECLAIMED"              },
        {AMEDIA_ERROR_BASE                      , "ERROR_BASE"                   },
        {AMEDIA_ERROR_UNKNOWN                   , "ERROR_UNKNOWN"                },
        {AMEDIA_ERROR_MALFORMED                 , "ERROR_MALFORMED"              },
        {AMEDIA_ERROR_UNSUPPORTED               , "ERROR_UNSUPPORTED"            },
        {AMEDIA_ERROR_INVALID_OBJECT            , "ERROR_INVALID_OBJECT"         },
        {AMEDIA_ERROR_INVALID_PARAMETER         , "ERROR_INVALID_PARAMETER"      },
        {AMEDIA_ERROR_INVALID_OPERATION         , "ERROR_INVALID_OPERATION"      },
        {AMEDIA_ERROR_END_OF_STREAM             , "ERROR_END_OF_STREAM"          },
        {AMEDIA_ERROR_IO                        , "ERROR_IO"                     },
        {AMEDIA_ERROR_WOULD_BLOCK               , "ERROR_WOULD_BLOCK"            },
        {AMEDIA_DRM_ERROR_BASE                  , "DRM_ERROR_BASE"               },
        {AMEDIA_DRM_NOT_PROVISIONED             , "DRM_NOT_PROVISIONED"          },
        {AMEDIA_DRM_RESOURCE_BUSY               , "DRM_RESOURCE_BUSY"            },
        {AMEDIA_DRM_DEVICE_REVOKED              , "DRM_DEVICE_REVOKED"           },
        {AMEDIA_DRM_SHORT_BUFFER                , "DRM_SHORT_BUFFER"             },
        {AMEDIA_DRM_SESSION_NOT_OPENED          , "DRM_SESSION_NOT_OPENED"       },
        {AMEDIA_DRM_TAMPER_DETECTED             , "DRM_TAMPER_DETECTED"          },
        {AMEDIA_DRM_VERIFY_FAILED               , "DRM_VERIFY_FAILED"            },
        {AMEDIA_DRM_NEED_KEY                    , "DRM_NEED_KEY"                 },
        {AMEDIA_DRM_LICENSE_EXPIRED             , "DRM_LICENSE_EXPIRED"          },
        {AMEDIA_IMGREADER_ERROR_BASE            , "IMGREADER_ERROR_BASE"         },
        {AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE   , "IMGREADER_NO_BUFFER_AVAILABLE"},
        {AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED   , "IMGREADER_MAX_IMAGES_ACQUIRED"},
        {AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE     , "IMGREADER_CANNOT_LOCK_IMAGE"  },
        {AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE   , "IMGREADER_CANNOT_UNLOCK_IMAGE"},
        {AMEDIA_IMGREADER_IMAGE_NOT_LOCKED      , "IMGREADER_IMAGE_NOT_LOCKED"   },
        {AMEDIA_OK                              , "OK"                           },
    };

    auto errorStatus = audioNkmEncErrorsStr;

    for (; errorStatus->status != AMEDIA_OK; ++errorStatus)
        if (errorStatus->status == status)
            return errorStatus->str;

    return errorStatus->str;
}

void VideoMuxerNDKMediaElementPrivate::listMuxers()
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);

    if (!tempFile.open())
        return;

    for (auto videoMuxer = VideoMuxer::table(); videoMuxer->muxer; ++videoMuxer)
        if (auto muxer = AMediaMuxer_new(tempFile.handle(),
                                         videoMuxer->amType)) {
            this->m_muxers << *videoMuxer;
            AMediaMuxer_delete(muxer);
        }
}

bool VideoMuxerNDKMediaElementPrivate::addTrackStart()
{
    if (!this->m_muxer) {
        qCritical() << "The muxer is NULL";

        return false;
    }

    // Check if the audio track is required and the codec is available

    AkCompressedAudioCaps audioCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Audio);

    if (audioCaps) {
        auto audioCodecs = self->supportedCodecs(self->muxer(),
                                                 AkCompressedCaps::CapsType_Audio);

        if (!audioCodecs.contains(audioCaps.codec())) {
            qCritical() << "Audio codec not supported by this muxer:" << audioCaps.codec();

            return false;
        }
    }

    // Add the video track to the muxer

    auto videoHeaders = self->streamHeaders(AkCompressedCaps::CapsType_Video);
    auto videoMediaFormat =
            *reinterpret_cast<const AMediaFormat * const *>(videoHeaders.constData());

    if (!videoMediaFormat) {
        qCritical() << "Video format missing";

        return false;
    }

    qDebug() << "Adding the video track";
    this->m_videoTrack = AMediaMuxer_addTrack(this->m_muxer, videoMediaFormat);

    if (this->m_videoTrack < 0) {
        qCritical() << "Failed to add video stream:"
                    << errorToStr(media_status_t(this->m_videoTrack));

        return false;
    }

    // Add the audio track to the muxer

    if (audioCaps) {
        auto audioHeaders =
                self->streamHeaders(AkCompressedCaps::CapsType_Audio);
        auto audioMediaFormat =
                *reinterpret_cast<const AMediaFormat * const *>(audioHeaders.constData());

        if (!audioMediaFormat) {
            qCritical() << "Audio format missing";

            return false;
        }

        qDebug() << "Adding the audio track";
        this->m_audioTrack =
                AMediaMuxer_addTrack(this->m_muxer, audioMediaFormat);

        if (this->m_audioTrack < 0) {
            qCritical() << "Failed to add audio stream:"
                        << errorToStr(media_status_t(this->m_audioTrack));

            return false;
        }
    }

    qDebug() << "Starting the muxer";
    auto result = AMediaMuxer_start(this->m_muxer);

    if (result != AMEDIA_OK) {
        qCritical() << "Failed to start the muxing:"
                    << errorToStr(result);

        return false;
    }

    return true;
}

bool VideoMuxerNDKMediaElementPrivate::init()
{
    this->uninit();
    qDebug() << "Starting the NDK muxer";

    if (!this->m_packetSync)
        return false;

    auto fit = std::find_if(this->m_muxers.begin(),
                            this->m_muxers.end(),
                            [this] (const VideoMuxer &muxer) -> bool {
        return muxer.muxer == self->muxer();
    });

    if (fit == this->m_muxers.constEnd()) {
        qCritical() << "Invalid muxer:" << self->muxer();

        return false;
    }

    AkCompressedVideoCaps videoCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Video);

    if (!videoCaps) {
        qCritical() << "No valid video format set";

        return false;
    }

    auto videoCodecs = self->supportedCodecs(self->muxer(),
                                             AkCompressedCaps::CapsType_Video);

    if (!videoCodecs.contains(videoCaps.codec())) {
        qCritical() << "Video codec not supported by this muxer:" << videoCaps.codec();

        return false;
    }

    this->m_outputFile.setFileName(self->location());

    if (!this->m_outputFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return false;

    qDebug() << "Creating the muxer";
    this->m_muxer = AMediaMuxer_new(this->m_outputFile.handle(), fit->amType);

    if (!this->m_muxer) {
        this->m_outputFile.close();

        return false;
    }

    AMediaMuxer_setOrientationHint(this->m_muxer, 0);
    this->m_audioTrackReady = false;
    this->m_videoTrackReady = false;
    this->m_recordAudio =
            bool(self->streamCaps(AkCompressedCaps::CapsType_Audio));

    this->m_packetSync->setProperty("audioEnabled", this->m_recordAudio);
    this->m_packetSync->setProperty("discardLast", false);
    this->m_packetSync->setState(AkElement::ElementStatePlaying);
    this->m_packetPos = 0;

    qInfo() << "Starting NDKMedia muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerNDKMediaElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    this->m_packetSync->setState(AkElement::ElementStateNull);

    if (this->m_muxer) {
        AMediaMuxer_stop(this->m_muxer);
        AMediaMuxer_delete(this->m_muxer);
        this->m_muxer = nullptr;
    }

    this->m_audioTrack = -1;
    this->m_videoTrack = -1;
    this->m_paused = false;
}

void VideoMuxerNDKMediaElementPrivate::packetReady(const AkPacket &packet)
{
    if (!this->m_muxer)
        return;

    if (this->m_packets.isEmpty()
        && this->m_videoTrackReady
        && (!this->m_recordAudio || this->m_audioTrackReady)) {
        bool isAudio = packet.type() == AkPacket::PacketAudio
                       || packet.type() == AkPacket::PacketAudioCompressed;
        uint64_t track = isAudio? this->m_audioTrack: this->m_videoTrack;

        if (track < 0)
            return;

        bool isKey = true;

        if (packet.type() == AkPacket::PacketVideoCompressed)
            isKey = AkCompressedVideoPacket(packet).flags()
                    & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame;

        auto result =
                AMediaMuxer_writeSampleData(this->m_muxer,
                                            track,
                                            reinterpret_cast<const uint8_t *>(packet.data()),
                                            reinterpret_cast<const AMediaCodecBufferInfo *>(packet.extraData().constData()));

        if (result != AMEDIA_OK) {
            if (isAudio)
                qCritical() << "Failed to write the audio packet:" << errorToStr(result);
            else
                qCritical() << "Failed to write the video packet:" << errorToStr(result);
        }

        this->m_packetPos++;
    } else {
        this->m_packets << packet;

        switch (packet.type()) {
        case AkPacket::PacketAudioCompressed:
            this->m_audioTrackReady = true;

            break;

        case AkPacket::PacketVideoCompressed:
            this->m_videoTrackReady = true;

            break;

        default:
            break;
        }

        if (!this->m_videoTrackReady
            || (this->m_recordAudio && !this->m_audioTrackReady)) {
            return;
        }

        if (!this->addTrackStart()) {
            qCritical() << "Failed adding the tracks and starting the muxer";
            AMediaMuxer_delete(this->m_muxer);
            this->m_muxer = nullptr;

            return;
        }

        while (!this->m_packets.isEmpty()) {
            auto packet = this->m_packets.takeFirst();
            bool isAudio = packet.type() == AkPacket::PacketAudio
                           || packet.type() == AkPacket::PacketAudioCompressed;
            uint64_t track = isAudio? this->m_audioTrack: this->m_videoTrack;

            if (track < 0)
                return;

            bool isKey = true;

            if (packet.type() == AkPacket::PacketVideoCompressed)
                isKey = AkCompressedVideoPacket(packet).flags()
                        & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame;

            auto result =
                    AMediaMuxer_writeSampleData(this->m_muxer,
                                                track,
                                                reinterpret_cast<const uint8_t *>(packet.data()),
                                                reinterpret_cast<const AMediaCodecBufferInfo *>(packet.extraData().constData()));

            if (result != AMEDIA_OK) {
                if (isAudio)
                    qCritical() << "Failed to write the audio packet:" << errorToStr(result);
                else
                    qCritical() << "Failed to write the video packet:" << errorToStr(result);
            }

            this->m_packetPos++;
        }
    }
}

#include "moc_videomuxerndkmediaelement.cpp"
