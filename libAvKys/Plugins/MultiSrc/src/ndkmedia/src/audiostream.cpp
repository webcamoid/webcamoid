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

#include <QtMath>
#include <QSharedPointer>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <akelement.h>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <media/NdkMediaExtractor.h>

#include "audiostream.h"
#include "clock.h"

// No AV correction is done if too big error.
#define AV_NOSYNC_THRESHOLD 10.0

// Maximum audio speed change to get correct sync
#define SAMPLE_CORRECTION_PERCENT_MAX 10

// We use about AUDIO_DIFF_AVG_NB A-V differences to make the average
#define AUDIO_DIFF_AVG_NB 20

#define ENCODING_PCM_16BIT 0x2
#define ENCODING_PCM_8BIT  0x3
#define ENCODING_PCM_FLOAT 0x4

#define CHANNEL_MASK_MONO                  0x2
#define CHANNEL_MASK_FRONT_LEFT            0x4
#define CHANNEL_MASK_FRONT_RIGHT           0x8
#define CHANNEL_MASK_FRONT_CENTER          0x10
#define CHANNEL_MASK_LOW_FREQUENCY         0x20
#define CHANNEL_MASK_BACK_LEFT             0x40
#define CHANNEL_MASK_BACK_RIGHT            0x80
#define CHANNEL_MASK_FRONT_LEFT_OF_CENTER  0x100
#define CHANNEL_MASK_FRONT_RIGHT_OF_CENTER 0x200
#define CHANNEL_MASK_BACK_CENTER           0x400
#define CHANNEL_MASK_SIDE_LEFT             0x800
#define CHANNEL_MASK_SIDE_RIGHT            0x1000

using ChannelMaskToPositionMap = QMap<int32_t, AkAudioCaps::Position>;

inline const ChannelMaskToPositionMap &channelMaskToPosition()
{
    static const ChannelMaskToPositionMap channelMaskToPosition {
        {CHANNEL_MASK_MONO                 , AkAudioCaps::Position_FrontCenter       },
        {CHANNEL_MASK_FRONT_LEFT           , AkAudioCaps::Position_FrontLeft         },
        {CHANNEL_MASK_FRONT_RIGHT          , AkAudioCaps::Position_FrontRight        },
        {CHANNEL_MASK_FRONT_CENTER         , AkAudioCaps::Position_FrontCenter       },
        {CHANNEL_MASK_LOW_FREQUENCY        , AkAudioCaps::Position_LowFrequency1     },
        {CHANNEL_MASK_BACK_LEFT            , AkAudioCaps::Position_BackLeft          },
        {CHANNEL_MASK_BACK_RIGHT           , AkAudioCaps::Position_BackRight         },
        {CHANNEL_MASK_FRONT_LEFT_OF_CENTER , AkAudioCaps::Position_FrontLeftOfCenter },
        {CHANNEL_MASK_FRONT_RIGHT_OF_CENTER, AkAudioCaps::Position_FrontRightOfCenter},
        {CHANNEL_MASK_BACK_CENTER          , AkAudioCaps::Position_BackCenter        },
        {CHANNEL_MASK_SIDE_LEFT            , AkAudioCaps::Position_SideLeft          },
        {CHANNEL_MASK_SIDE_RIGHT           , AkAudioCaps::Position_SideRight         },
    };

    return channelMaskToPosition;
}

class AudioStreamPrivate
{
    public:
        AudioStream *self;
        AkAudioConverter m_audioConvert;
        qreal m_audioDiffCum {0.0}; // used for AV difference average computation
        qreal m_audioDiffAvgCoef {exp(log(0.01) / AUDIO_DIFF_AVG_NB)};
        int m_audioDiffAvgCount {0};
        bool m_eos {false};

        explicit AudioStreamPrivate(AudioStream *self);
        AkPacket readPacket(size_t bufferIndex,
                            const AMediaCodecBufferInfo &info);
};

AudioStream::AudioStream(AMediaExtractor *mediaExtractor,
                         uint index,
                         qint64 id,
                         Clock *globalClock,
                         bool sync,
                         QObject *parent):
    AbstractStream(mediaExtractor,
                   index,
                   id,
                   globalClock,
                   sync,
                   parent)
{
    this->d = new AudioStreamPrivate(this);
    this->m_maxData = 9;
}

AudioStream::~AudioStream()
{
    delete this->d;
}

AkCaps AudioStream::caps() const
{
    AkAudioCaps::SampleFormat sampleFormat = AkAudioCaps::SampleFormat_s16;
#if __ANDROID_API__ >= 28
    int32_t pcmEncoding = 0;

    if (AMediaFormat_getInt32(this->mediaFormat(),
                              AMEDIAFORMAT_KEY_PCM_ENCODING,
                              &pcmEncoding))
        sampleFormat = AudioStream::sampleFormatFromEncoding(pcmEncoding);
#endif
    AkAudioCaps::ChannelLayout layout = AkAudioCaps::Layout_none;
    int32_t channelMask = 0;

    if (AMediaFormat_getInt32(this->mediaFormat(),
                              AMEDIAFORMAT_KEY_CHANNEL_MASK,
                              &channelMask)) {
        layout = AudioStream::layoutFromChannelMask(channelMask);
    } else {
        int32_t channels = 0;
        AMediaFormat_getInt32(this->mediaFormat(),
                              AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                              &channels);
        layout = AkAudioCaps::defaultChannelLayout(channels);
    }

    int32_t rate = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_SAMPLE_RATE,
                          &rate);

    return AkAudioCaps(sampleFormat, layout, false, rate);
}

bool AudioStream::eos() const
{
    return this->d->m_eos;
}

AbstractStream::EnqueueResult AudioStream::decodeData()
{
    if (!this->isValid())
        return EnqueueFailed;

    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));
    ssize_t timeOut = 5000;
    auto bufferIndex =
            AMediaCodec_dequeueOutputBuffer(this->codec(), &info, timeOut);
    AkPacket packet;

    if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        return EnqueueAgain;
    } else if (bufferIndex >= 0) {
        packet = this->d->readPacket(size_t(bufferIndex), info);
        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);

        if (this->m_buffersQueued > 0) {
            this->m_bufferQueueSize = this->m_bufferQueueSize *
                                      (this->m_buffersQueued - 1)
                                      / this->m_buffersQueued;
            this->m_buffersQueued--;
        }
    }

    EnqueueResult result = EnqueueFailed;

    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        while (this->running()) {
            result = this->dataEnqueue({});

            if (result != EnqueueAgain)
                break;
        }

        this->d->m_eos = true;
    } else if (packet) {
        while (this->running()) {
            result = this->dataEnqueue(packet);

            if (result != EnqueueAgain)
                break;
        }
    }

    return result;
}

AkAudioCaps::SampleFormat AudioStream::sampleFormatFromEncoding(int32_t encoding)
{
    static const QMap<int32_t, AkAudioCaps::SampleFormat> sampleFormatFromEncoding {
        {ENCODING_PCM_8BIT , AkAudioCaps::SampleFormat_u8 },
        {ENCODING_PCM_16BIT, AkAudioCaps::SampleFormat_s16},
        {ENCODING_PCM_FLOAT, AkAudioCaps::SampleFormat_flt},
    };

    return sampleFormatFromEncoding.value(encoding);
}

AkAudioCaps::ChannelLayout AudioStream::layoutFromChannelMask(int32_t channelMask)
{
    auto &channelMaskToPositionMap = channelMaskToPosition();
    QVector<AkAudioCaps::Position> positions;

    for (auto it = channelMaskToPositionMap.constBegin();
         it != channelMaskToPositionMap.constEnd();
         it++)
        if (channelMask & it.key())
            positions << it.value();

    return AkAudioCaps::channelLayoutFromPositions(positions);
}

void AudioStream::processData(const AkPacket &packet)
{
    if (!this->sync()) {
        emit this->oStream(packet);

        return;
    }

    AkAudioPacket audioPacket = packet;

    // Synchronize audio
    qreal pts = packet.pts() * packet.timeBase().value();
    qreal diff = pts - this->globalClock()->clock();

    if (!qIsNaN(diff) && qAbs(diff) < AV_NOSYNC_THRESHOLD) {
        this->d->m_audioDiffCum = diff + this->d->m_audioDiffAvgCoef * this->d->m_audioDiffCum;

        if (this->d->m_audioDiffAvgCount < AUDIO_DIFF_AVG_NB) {
            // not enough measures to have a correct estimate
            this->d->m_audioDiffAvgCount++;
        } else {
            // estimate the A-V difference
            qreal avgDiff = this->d->m_audioDiffCum * (1.0 - this->d->m_audioDiffAvgCoef);

            // since we do not have a precise anough audio fifo fullness,
            // we correct audio sync only if larger than this threshold
            qreal diffThreshold = 2.0 * audioPacket.samples() / audioPacket.caps().rate();

            if (qAbs(avgDiff) >= diffThreshold) {
                int wantedSamples = audioPacket.samples() + int(diff * audioPacket.caps().rate());
                int minSamples = audioPacket.samples() * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                int maxSamples = audioPacket.samples() * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                wantedSamples = qBound(minSamples, wantedSamples, maxSamples);
                this->d->m_audioConvert.setOutputCaps(audioPacket.caps());
                audioPacket = this->d->m_audioConvert.scale(audioPacket, wantedSamples);
            }
        }
    } else {
        // Too big difference: may be initial PTS errors, so
        // reset A-V filter
        this->d->m_audioDiffAvgCount = 0;
        this->d->m_audioDiffCum = 0.0;
    }

    if (qAbs(diff) >= AV_NOSYNC_THRESHOLD)
        this->globalClock()->setClock(pts);

    this->clockDiff() = diff;

    emit this->oStream(audioPacket);
}

AudioStreamPrivate::AudioStreamPrivate(AudioStream *self):
    self(self)
{
}

AkPacket AudioStreamPrivate::readPacket(size_t bufferIndex,
                                        const AMediaCodecBufferInfo &info)
{
    auto format = AMediaCodec_getOutputFormat(self->codec());
    AkAudioCaps::SampleFormat sampleFormat = AkAudioCaps::SampleFormat_s16;
#if __ANDROID_API__ >= 28
    int32_t pcmEncoding = 0;

    if (AMediaFormat_getInt32(format,
                              AMEDIAFORMAT_KEY_PCM_ENCODING,
                              &pcmEncoding))
        sampleFormat = AudioStream::sampleFormatFromEncoding(pcmEncoding);
#endif
    AkAudioCaps::ChannelLayout layout = AkAudioCaps::Layout_none;
    int32_t channelMask = 0;

    if (AMediaFormat_getInt32(format,
                              AMEDIAFORMAT_KEY_CHANNEL_MASK,
                              &channelMask)) {
        layout = AudioStream::layoutFromChannelMask(channelMask);
    } else {
        int32_t channels = 0;
        AMediaFormat_getInt32(format,
                              AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                              &channels);
        layout = AkAudioCaps::defaultChannelLayout(channels);
    }

    int32_t rate = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_SAMPLE_RATE,
                          &rate);

    AMediaFormat_delete(format);

    size_t bufferSize = 0;
    auto buffer = AMediaCodec_getOutputBuffer(self->codec(),
                                             size_t(bufferIndex),
                                             &bufferSize);
    bufferSize = qMin<size_t>(bufferSize, info.size);
    int samples = 8
                  * int(bufferSize)
                  / (AkAudioCaps::channelCount(layout)
                     * AkAudioCaps::bitsPerSample(sampleFormat));
    AkAudioPacket packet({sampleFormat,
                          layout,
                          false,
                          rate}, samples);
    memcpy(packet.data(), buffer + info.offset, packet.size());
    packet.setPts(info.presentationTimeUs);
    packet.setTimeBase(AkFrac(1, 1e6));
    packet.setIndex(self->index());
    packet.setId(self->id());

    return packet;
}

#include "moc_audiostream.cpp"
