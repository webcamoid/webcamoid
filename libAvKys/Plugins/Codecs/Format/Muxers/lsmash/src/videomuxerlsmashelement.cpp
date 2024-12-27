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
#include <QMutex>
#include <QQmlContext>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <iak/akelement.h>
#include <lsmash.h>

#include "videomuxerlsmashelement.h"

struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    lsmash_codec_type_t mp4CodecID;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable lsmashAudioCodecsTable[] {
            {AkCompressedAudioCaps::AudioCodecID_aac    , ISOM_CODEC_TYPE_MP4A_AUDIO},
            {AkCompressedAudioCaps::AudioCodecID_unknown, 0                         },
        };

        return lsmashAudioCodecsTable;
    }

    inline static const AudioCodecsTable *byCodecID(AkCompressedAudioCaps::AudioCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }

    inline static QList<AkCodecID> codecs()
    {
        QList<AkCodecID> codecs;
        auto item = table();

        for (; item->codecID; ++item)
            codecs << item->codecID;

        return codecs;
    }
};

struct VideoCodecsTable
{
    AkCompressedVideoCaps::VideoCodecID codecID;
    lsmash_codec_type_t mp4CodecID;

    inline static const VideoCodecsTable *table()
    {
        static const VideoCodecsTable lsmashVideoCodecsTable[] {
            {AkCompressedVideoCaps::VideoCodecID_h264   , ISOM_CODEC_TYPE_AVC1_VIDEO},
            {AkCompressedVideoCaps::VideoCodecID_unknown, 0                         },
        };

        return lsmashVideoCodecsTable;
    }

    inline static const VideoCodecsTable *byCodecID(AkCompressedVideoCaps::VideoCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }

    inline static QList<AkCodecID> codecs()
    {
        QList<AkCodecID> codecs;
        auto item = table();

        for (; item->codecID; ++item)
            codecs << item->codecID;

        return codecs;
    }
};

struct TrackInfo
{
    uint32_t track;
    int sampleEntry {0};
    bool firstPacket {true};
    bool firstWrittenPacket {true};
    int64_t startOffset {0};
    uint64_t firstCts {0};
    int64_t largestPts {0};
    int64_t secondLargestPts {0};
};

class VideoMuxerLSmashElementPrivate
{
    public:
        VideoMuxerLSmashElement *self;
        bool m_optimize {false};
        lsmash_root_t *m_root {nullptr};
        lsmash_file_parameters_t m_fileParams;
        uint32_t m_globalTimeScale {90000};
        QVector<TrackInfo> m_trackInfo;
        bool m_initialized {false};
        bool m_paused {false};
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerLSmashElementPrivate(VideoMuxerLSmashElement *self);
        ~VideoMuxerLSmashElementPrivate();
        static const char *errorToString(int error);
        bool init();
        void uninit();
        uint32_t addAudioTrack(lsmash_root_t *root,
                               lsmash_codec_type_t codecID,
                               const AkCompressedAudioCaps &audioCaps,
                               QByteArray &privateData,
                               int *sampleEntry) const;
        uint32_t addVideoTrack(lsmash_root_t *root,
                               lsmash_codec_type_t codecID,
                               const AkCompressedVideoCaps &videoCaps,
                               QByteArray &privateData,
                               int *sampleEntry) const;
        bool addH264SpecificData(lsmash_summary_t *summary,
                                 QByteArray &privateData) const;
        void packetReady(const AkPacket &packet);
};

VideoMuxerLSmashElement::VideoMuxerLSmashElement():
    AkVideoMuxer()
{
    this->d = new VideoMuxerLSmashElementPrivate(this);
}

VideoMuxerLSmashElement::~VideoMuxerLSmashElement()
{
    this->d->uninit();
    delete this->d;
}

AkVideoMuxer::FormatID VideoMuxerLSmashElement::formatID() const
{
    return FormatID_mp4;
}

QString VideoMuxerLSmashElement::extension() const
{
    return {"mp4"};
}

bool VideoMuxerLSmashElement::gapsAllowed(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return true;

    case AkCompressedCaps::CapsType_Video:
        return false;

    default:
        break;
    }

    return true;
}

QList<AkCodecID> VideoMuxerLSmashElement::supportedCodecs(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return AudioCodecsTable::codecs();

    case AkCompressedCaps::CapsType_Video:
        return VideoCodecsTable::codecs();

    case AkCompressedCaps::CapsType_Unknown:
        return AudioCodecsTable::codecs() + VideoCodecsTable::codecs();

    default:
        break;
    }

    return {};
}

AkCodecID VideoMuxerLSmashElement::defaultCodec(AkCodecType type) const
{
    auto codecs = this->supportedCodecs(type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

bool VideoMuxerLSmashElement::optimize() const
{
    return this->d->m_optimize;
}

QString VideoMuxerLSmashElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoMuxerLSmash/share/qml/main.qml");
}

void VideoMuxerLSmashElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoMuxerLSmash", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void VideoMuxerLSmashElement::setOptimize(bool optimize)
{
    if (optimize == this->d->m_optimize)
        return;

    this->d->m_optimize = optimize;
    emit this->optimizeChanged(optimize);
}

void VideoMuxerLSmashElement::resetOptimize()
{
    this->setOptimize(false);
}

void VideoMuxerLSmashElement::resetOptions()
{
    AkVideoMuxer::resetOptions();
    this->resetOptimize();
}

AkPacket VideoMuxerLSmashElement::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerLSmashElement::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_paused = state == AkElement::ElementStatePaused;
        case AkElement::ElementStatePlaying:
            if (!this->d->init()) {
                this->d->m_paused = false;

                return false;
            }

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

VideoMuxerLSmashElementPrivate::VideoMuxerLSmashElementPrivate(VideoMuxerLSmashElement *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerLSmashElementPrivate::~VideoMuxerLSmashElementPrivate()
{

}

const char *VideoMuxerLSmashElementPrivate::errorToString(int error)
{
    static const struct ErrorCodesStr
    {
        int code;
        const char *str;
    } lsmashEncErrorCodes[] = {
        {LSMASH_ERR_NAMELESS      , "Nameless error"         },
        {LSMASH_ERR_MEMORY_ALLOC  , "Error allocating memory"},
        {LSMASH_ERR_INVALID_DATA  , "Invalid data"           },
        {LSMASH_ERR_FUNCTION_PARAM, "Invalid parameter"      },
        {LSMASH_ERR_PATCH_WELCOME , "Not implemented"        },
        {LSMASH_ERR_UNKNOWN       , "Unknown error"          },
        {LSMASH_ERR_IO            , "I/O error"              },
        {0                        , ""                       },
    };

    auto ec = lsmashEncErrorCodes;

    for (; ec->code; ++ec)
        if (ec->code == error)
            return ec->str;

    static char lsmashEncErrorStr[1024];
    snprintf(lsmashEncErrorStr, 1024, "%d", error);

    return lsmashEncErrorStr;
}

bool VideoMuxerLSmashElementPrivate::init()
{
    this->uninit();

    if (!this->m_packetSync)
        return false;

    AkCompressedVideoCaps videoCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Video);

    if (!videoCaps) {
        qCritical() << "No valid video format set";

        return false;
    }

    auto vcodec = VideoCodecsTable::byCodecID(videoCaps.codec());

    if (!vcodec->codecID) {
        qCritical() << "Video codec not supported by this muxer:" << videoCaps.codec();

        return false;
    }

    AkCompressedAudioCaps audioCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Audio);

    const AudioCodecsTable *acodec = nullptr;

    if (audioCaps) {
        acodec = AudioCodecsTable::byCodecID(audioCaps.codec());

        if (!acodec->codecID) {
            qCritical() << "Audio codec not supported by this muxer:" << audioCaps.codec();

            return false;
        }
    }

    // Create the file

    auto location = self->location();

    if (lsmash_open_file(location.toStdString().c_str(),
                         0,
                         &this->m_fileParams) < 0) {
        qCritical() << "Failed to open an output file";

        return false;
    }

    this->m_root = lsmash_create_root();

    QVector<lsmash_brand_type> brands {
        ISOM_BRAND_TYPE_MP42,
        ISOM_BRAND_TYPE_MP41,
        ISOM_BRAND_TYPE_ISOM,
    };

    this->m_fileParams.major_brand = brands[0];
    this->m_fileParams.brands = brands.data();
    this->m_fileParams.brand_count = brands.size();
    this->m_fileParams.minor_version = 0x00000000;

    if (!lsmash_set_file(this->m_root, &this->m_fileParams)) {
        qCritical() << "Failed to add an output file into a ROOT";
        lsmash_destroy_root(this->m_root);
        this->m_root = nullptr;

        return false;
    }

    lsmash_movie_parameters_t movieParams;
    lsmash_initialize_movie_parameters(&movieParams);
    auto result = lsmash_set_movie_parameters(this->m_root, &movieParams);

    if (result) {
        qCritical() << "Failed to set movie parameters:" << errorToString(result);
        lsmash_destroy_root(this->m_root);
        this->m_root = nullptr;

        return false;
    }

    this->m_trackInfo.clear();

    // Read private headers

    QByteArray privateData;

    for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Video))
        privateData += QByteArray(header.constData(), header.size());

    // Add video Track
    qInfo() << "Adding video track with format:" << videoCaps;

    int sampleEntry  = 0;
    auto videoTrack = this->addVideoTrack(this->m_root,
                                          vcodec->mp4CodecID,
                                          videoCaps,
                                          privateData,
                                          &sampleEntry);

    if (videoTrack < 1) {
        qCritical() << "Failed to create the video track";
        lsmash_destroy_root(this->m_root);
        this->m_root = nullptr;

        return false;
    }

    this->m_trackInfo << TrackInfo();
    this->m_trackInfo.last().track = videoTrack;
    this->m_trackInfo.last().sampleEntry = sampleEntry;

    // Add the audio track

    uint32_t audioTrack = 0;

    if (audioCaps) {
        qInfo() << "Adding audio track with format:" << audioCaps;

        privateData.clear();

        for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Audio))
            privateData += QByteArray(header.constData(), header.size());

        int sampleEntry  = 0;
        audioTrack = this->addAudioTrack(this->m_root,
                                         acodec->mp4CodecID,
                                         audioCaps,
                                         privateData,
                                         &sampleEntry);

        if (audioTrack < 1) {
            qCritical() << "Failed to create the audio track";
            lsmash_destroy_root(this->m_root);
            this->m_root = nullptr;

            return false;
        }

        this->m_trackInfo << TrackInfo();
        this->m_trackInfo.last().track = audioTrack;
        this->m_trackInfo.last().sampleEntry = sampleEntry;
    }

    this->m_packetSync->setProperty("audioEnabled", audioTrack > 0);
    this->m_packetSync->setProperty("discardLast", false);
    this->m_packetSync->setState(AkElement::ElementStatePlaying);

    qInfo() << "Starting MP4 muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerLSmashElementPrivate::uninit()
{
    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    this->m_packetSync->setState(AkElement::ElementStateNull);

    lsmash_movie_parameters_t movieParameters;
    auto result = lsmash_get_movie_parameters(this->m_root, &movieParameters);

    if (result) {
        qCritical() << "Failed to flush the rest of samples:" << errorToString(result);

        return;
    }

    // Flush the rest of samples and add the last sample_delta.

    for (uint32_t track; track < movieParameters.number_of_tracks; track++) {
        auto trackId = lsmash_get_track_ID(this->m_root, track);

        if (trackId < 1)
            continue;

        auto &trackInfo = this->m_trackInfo[0];
        uint32_t lastDelta = trackInfo.largestPts - trackInfo.secondLargestPts;
        result = lsmash_flush_pooled_samples(this->m_root,
                                             trackId,
                                             lastDelta);

        if (result) {
            qCritical() << "Failed to flush the rest of samples:" << errorToString(result);

            return;
        }

        lsmash_media_parameters_t params;
        result = lsmash_get_media_parameters(this->m_root, trackId, &params);

        if (result) {
            qCritical() << "Failed to read the stream parameters:" << errorToString(result);

            return;
        }

        uint64_t actualDuration =
                (trackInfo.largestPts + lastDelta) / params.timescale;

        lsmash_edit_t edit;
        memset(&edit, 0, sizeof(lsmash_edit_t));
        edit.duration = actualDuration;
        edit.start_time = trackInfo.firstCts;
        edit.rate = ISOM_EDIT_MODE_NORMAL;
        result = lsmash_create_explicit_timeline_map(this->m_root, trackId, edit);

        if (result) {
            qCritical() << "failed to set timeline map for video:" << errorToString(result);

            return;
        }
    }

    // Close

    result = lsmash_finish_movie(this->m_root, nullptr);

    if (result) {
        qCritical() << "failed finishing the video:" << errorToString(result);

        return;
    }

    lsmash_destroy_root(this->m_root);
    result = lsmash_close_file(&this->m_fileParams);

    if (result) {
        qCritical() << "Error closing the file:" << errorToString(result);

        return;
    }

    this->m_paused = false;
}

uint32_t VideoMuxerLSmashElementPrivate::addAudioTrack(lsmash_root_t *root,
                                                       lsmash_codec_type_t codecID,
                                                       const AkCompressedAudioCaps &audioCaps,
                                                       QByteArray &privateData,
                                                       int *sampleEntry) const
{
    uint32_t audioTrack =
            lsmash_create_track(root, ISOM_MEDIA_HANDLER_TYPE_AUDIO_TRACK);

    if (!audioTrack) {
        qCritical() << "Failed to create the audio track";

        return 0;
    }

    lsmash_track_parameters_t trackParams;
    lsmash_initialize_track_parameters(&trackParams);
    trackParams.mode =
            lsmash_track_mode(ISOM_TRACK_ENABLED | ISOM_TRACK_IN_MOVIE);
    trackParams.audio_volume = 0x0100;

    // Set track parameters

    auto result = lsmash_set_track_parameters(root, audioTrack, &trackParams);

    if (result) {
        qCritical() << "Failed to set the audio track parameters:" << errorToString(result);

        return 0;
    }

    lsmash_media_parameters_t mediaParams;
    lsmash_initialize_media_parameters(&mediaParams);
    mediaParams.ISO_language = ISOM_LANGUAGE_CODE_UNDEFINED;
    static char handlerName[] = "Audio";
    mediaParams.media_handler_name = handlerName;
    mediaParams.timescale = audioCaps.rawCaps().rate();

    // Set media parameters

    result = lsmash_set_media_parameters(root, audioTrack, &mediaParams);

    if (result) {
        qCritical() << "Failed to set the audio parameters:" << errorToString(result);

        return 0;
    }

    auto summary = reinterpret_cast<lsmash_audio_summary_t *>(lsmash_create_summary(LSMASH_SUMMARY_TYPE_AUDIO));

    if (!summary) {
        qCritical() << "Error creating the summary";

        return 0;
    }

    summary->sample_type = codecID;
    summary->aot = MP4A_AUDIO_OBJECT_TYPE_AAC_LC;
    summary->frequency = audioCaps.rawCaps().rate();
    summary->channels = audioCaps.rawCaps().channels();
    summary->sample_size = audioCaps.rawCaps().bps();
    summary->samples_in_frame = 1024;
    summary->bytes_per_frame = 0;

    if (!privateData.isEmpty()) {
        auto codecSpec = lsmash_create_codec_specific_data(LSMASH_CODEC_SPECIFIC_DATA_TYPE_MP4SYS_DECODER_CONFIG,
                                                           LSMASH_CODEC_SPECIFIC_FORMAT_STRUCTURED);
        auto param = reinterpret_cast<lsmash_mp4sys_decoder_parameters_t *>(codecSpec->data.structured);
        param->objectTypeIndication = MP4SYS_OBJECT_TYPE_Audio_ISO_14496_3;
        param->streamType = MP4SYS_STREAM_TYPE_AudioStream;

        if (lsmash_set_mp4sys_decoder_specific_info(param,
                                                    reinterpret_cast<uint8_t *>(privateData.data()),
                                                    privateData.size()) < 0 ) {
            lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

            return 0;
        }

        result = lsmash_add_codec_specific_data(reinterpret_cast<lsmash_summary_t *>(summary),
                                                codecSpec);

        if (result) {
            qCritical() << "Failed to set codec specific data:" << errorToString(result);
            lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

            return 0;
        }
    }

    auto entry = lsmash_add_sample_entry(root, audioTrack, summary);

    if (entry < 1) {
        qCritical() << "Failed to add sample entry for audio";
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return 0;
    }

    lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

    if (sampleEntry)
        *sampleEntry = entry;

    return audioTrack;
}

uint32_t VideoMuxerLSmashElementPrivate::addVideoTrack(lsmash_root_t *root,
                                                       lsmash_codec_type_t codecID,
                                                       const AkCompressedVideoCaps &videoCaps,
                                                       QByteArray &privateData,
                                                       int *sampleEntry) const
{
    uint32_t videoTrack =
            lsmash_create_track(root, ISOM_MEDIA_HANDLER_TYPE_VIDEO_TRACK);

    if (!videoTrack) {
        qCritical() << "Failed to create the video track";

        return 0;
    }

    lsmash_track_parameters_t trackParams;
    lsmash_initialize_track_parameters(&trackParams);
    trackParams.mode = lsmash_track_mode(ISOM_TRACK_ENABLED
                                         | ISOM_TRACK_IN_MOVIE
                                         | ISOM_TRACK_IN_PREVIEW);

    trackParams.display_width = videoCaps.rawCaps().width();
    trackParams.display_height = videoCaps.rawCaps().height();

    // Set track parameters

    auto result = lsmash_set_track_parameters(root, videoTrack, &trackParams);

    if (result) {
        qCritical() << "Failed to set the video track parameters:" << errorToString(result);

        return 0;
    }

    lsmash_media_parameters_t mediaParams;
    lsmash_initialize_media_parameters(&mediaParams);
    mediaParams.ISO_language = ISOM_LANGUAGE_CODE_UNDEFINED;
    static char handlerName[] = "Video";
    mediaParams.media_handler_name = handlerName;
    mediaParams.timescale = qRound(videoCaps.rawCaps().fps().value());

    // Set media parameters

    result = lsmash_set_media_parameters(root, videoTrack, &mediaParams);

    if (result) {
        qCritical() << "Failed to set the video parameters:" << errorToString(result);

        return 0;
    }

    auto summary = reinterpret_cast<lsmash_video_summary_t *>(lsmash_create_summary(LSMASH_SUMMARY_TYPE_VIDEO));

    if (!summary) {
        qCritical() << "Error creating the summary";

        return 0;
    }

    summary->sample_type = codecID;
    summary->width = trackParams.display_width;
    summary->height = trackParams.display_height;
    summary->par_h = 0;
    summary->par_v = 0;
    summary->color.primaries_index = ISOM_PRIMARIES_INDEX_UNSPECIFIED;
    summary->color.transfer_index = ISOM_TRANSFER_INDEX_UNSPECIFIED;
    summary->color.matrix_index = ISOM_MATRIX_INDEX_UNSPECIFIED;
    summary->color.full_range = 0;

    switch (videoCaps.codec()) {
    case AkCompressedVideoCaps::VideoCodecID_h264:
        if (!this->addH264SpecificData(reinterpret_cast<lsmash_summary_t *>(summary),
                                       privateData)) {
            qCritical() << "Failed to add H264 specific data";
            lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

            return 0;
        }

        break;

    default:
        break;
    }

    auto entry = lsmash_add_sample_entry(root, videoTrack, summary);

    if (entry < 1) {
        qCritical() << "Failed to add sample entry for video";
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return 0;
    }

    lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

    if (sampleEntry)
        *sampleEntry = entry;

    return videoTrack;
}

bool VideoMuxerLSmashElementPrivate::addH264SpecificData(lsmash_summary_t *summary,
                                                         QByteArray &privateData) const
{
    if (privateData.isEmpty()) {
        qCritical() << "The codec private data is empty";

        return false;
    }

    QDataStream ds(&privateData, QIODeviceBase::ReadOnly);
    quint64 nHeaders = 0;
    ds >> nHeaders;

    if (nHeaders < 2) {
        qCritical() << "Can't read the private headers";
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return false;
    }

    QVector<QByteArray> headers;

    for (quint64 i = 0; i < nHeaders; ++i) {
        quint64 payloadSize = 0;
        ds >> payloadSize;
        QByteArray header(payloadSize, Qt::Uninitialized);
        ds.readRawData(header.data(), header.size());
        headers << header;
    }

    auto codecSpec =
            lsmash_create_codec_specific_data(LSMASH_CODEC_SPECIFIC_DATA_TYPE_ISOM_VIDEO_H264,
                                              LSMASH_CODEC_SPECIFIC_FORMAT_STRUCTURED);
    static const int h264NaluLengthSize = 4;

    auto sps = headers[0].data() + h264NaluLengthSize;
    auto pps = headers[1].data() + h264NaluLengthSize;

    qsizetype spsSize = headers[0].size() - h264NaluLengthSize;
    qsizetype ppsSize = headers[1].size() - h264NaluLengthSize;

    auto param = reinterpret_cast<lsmash_h264_specific_parameters_t *>(codecSpec->data.structured);
    param->lengthSizeMinusOne = h264NaluLengthSize - 1;

    // Set SPS

    auto result =
            lsmash_append_h264_parameter_set(param,
                                             H264_PARAMETER_SET_TYPE_SPS,
                                             sps,
                                             spsSize);

    if (result) {
        qCritical() << "Failed to set SPS:" << errorToString(result);
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return false;
    }

    // Set PPS

    result = lsmash_append_h264_parameter_set(param,
                                              H264_PARAMETER_SET_TYPE_PPS,
                                              pps,
                                              ppsSize);

    if (result) {
        qCritical() << "Failed to set PPS:" << errorToString(result);
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return false;
    }

    result = lsmash_add_codec_specific_data(reinterpret_cast<lsmash_summary_t *>(summary),
                                            codecSpec);

    if (result) {
        qCritical() << "Failed to set codec specific data:" << errorToString(result);
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t *>(summary));

        return false;
    }

    auto bitrate =
        lsmash_create_codec_specific_data(LSMASH_CODEC_SPECIFIC_DATA_TYPE_ISOM_VIDEO_H264_BITRATE,
                                          LSMASH_CODEC_SPECIFIC_FORMAT_STRUCTURED);

    if (bitrate) {
        result = lsmash_add_codec_specific_data(reinterpret_cast<lsmash_summary_t *>(summary), bitrate);

        if (result)
            qCritical() << "Error setting the bitrate:" << errorToString(result);

        lsmash_destroy_codec_specific_data(bitrate);
    }

    return true;
}

void VideoMuxerLSmashElementPrivate::packetReady(const AkPacket &packet)
{
    bool isAudio = packet.type() == AkPacket::PacketAudio
                   || packet.type() == AkPacket::PacketAudioCompressed;
    uint32_t track = isAudio? 1: 0;
    bool isSyncSample = true;

    if (packet.type() == AkPacket::PacketVideoCompressed)
        isSyncSample =
                AkCompressedVideoPacket(packet).flags()
                & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame;

    if (track >= this->m_trackInfo.size())
        return;

    auto &trackInfo = this->m_trackInfo[track];
    lsmash_media_parameters_t params;
    auto result =
            lsmash_get_media_parameters(this->m_root, trackInfo.track, &params);

    if (result)
        qCritical() << "Failed to read the track parameters:" << errorToString(result);

    if (trackInfo.firstPacket) {
        trackInfo.startOffset = -packet.dts();
        trackInfo.firstCts = trackInfo.startOffset * params.timescale;
        trackInfo.firstPacket = false;
    }

    auto sample = lsmash_create_sample(packet.size());

    if (!sample) {
        qCritical() << "Failed to create the sample";

        return;
    }

    memcpy(sample->data, packet.constData(), packet.size());
    sample->cts = (packet.pts() + trackInfo.startOffset);
    sample->dts = (packet.dts() + trackInfo.startOffset);
    sample->index = trackInfo.sampleEntry;
    sample->prop.ra_flags = isSyncSample?
                                ISOM_SAMPLE_RANDOM_ACCESS_FLAG_SYNC:
                                ISOM_SAMPLE_RANDOM_ACCESS_FLAG_NONE;

    result = lsmash_append_sample(this->m_root, trackInfo.track, sample);

    if (result) {
        if (isAudio)
            qCritical() << "Failed to write the audio packet:" << errorToString(result);
        else
            qCritical() << "Failed to write the video packet:" << errorToString(result);
    }

    if (trackInfo.firstWrittenPacket) {
        trackInfo.largestPts = trackInfo.secondLargestPts = packet.pts();
        trackInfo.firstWrittenPacket = false;
    } else {
        trackInfo.secondLargestPts = trackInfo.largestPts;
        trackInfo.largestPts = packet.pts();
    }
}

#include "moc_videomuxerlsmashelement.cpp"
