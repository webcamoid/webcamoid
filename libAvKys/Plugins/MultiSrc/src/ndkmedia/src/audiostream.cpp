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
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
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
        AkElementPtr m_audioConvert;
        qreal audioDiffCum {0.0}; // used for AV difference average computation
        qreal audioDiffAvgCoef {exp(log(0.01) / AUDIO_DIFF_AVG_NB)};
        int audioDiffAvgCount {0};

        explicit AudioStreamPrivate(AudioStream *self);
        AkPacket readPacket(size_t bufferIndex,
                            const AMediaCodecBufferInfo &info);
        AkAudioPacket convert(const AkAudioPacket &packet);
};

AudioStream::AudioStream(AMediaExtractor *mediaExtractor,
                         uint index, qint64 id, Clock *globalClock,
                         QObject *parent):
    AbstractStream(mediaExtractor, index, id, globalClock, parent)
{
    this->d = new AudioStreamPrivate(this);
    this->m_maxData = 9;
    this->d->m_audioConvert = AkElement::create("ACapsConvert");
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

    return AkAudioCaps(sampleFormat, layout, rate);
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

bool AudioStream::decodeData()
{
    if (!this->isValid())
        return false;

    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));
    ssize_t timeOut = 5000;
    auto bufferIndex =
            AMediaCodec_dequeueOutputBuffer(this->codec(), &info, timeOut);

    if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        return true;
    else if (bufferIndex >= 0) {
        auto packet = this->d->readPacket(size_t(bufferIndex), info);

        if (packet)
            this->avPacketEnqueue(packet);

        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);
    }

    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        this->avPacketEnqueue({});

        return false;
    }

    return true;
}

void AudioStream::processPacket(const AkPacket &packet)
{
    auto oPacket = this->d->convert(packet);
    emit this->oStream(oPacket);
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
    bufferSize = qMin(bufferSize, size_t(info.size));
    QByteArray oBuffer(int(bufferSize), Qt::Uninitialized);
    memcpy(oBuffer.data(), buffer + info.offset, bufferSize);

    AkAudioPacket packet;
    packet.caps() = {sampleFormat,
                     layout,
                     rate,
                     8
                     * int(bufferSize)
                     / (AkAudioCaps::channelCount(layout)
                        * AkAudioCaps::bitsPerSample(sampleFormat))};
    packet.buffer() = oBuffer;
    packet.pts() = info.presentationTimeUs;
    packet.timeBase() = AkFrac(1, 1e6);
    packet.index() = int(self->index());
    packet.id() = self->id();

    return packet;
}

AkAudioPacket AudioStreamPrivate::convert(const AkAudioPacket &packet)
{
    if (this->m_audioConvert->state() != AkElement::ElementStatePlaying) {
        this->m_audioConvert->setProperty("caps",
                                          QVariant::fromValue(packet.caps()));
        this->m_audioConvert->setState(AkElement::ElementStatePlaying);
    }

    AkAudioPacket audioPacket = packet;

    // Synchronize audio
    qreal pts = packet.pts() * packet.timeBase().value();
    qreal diff = pts - self->globalClock()->clock();
    int wantedSamples = audioPacket.caps().samples();

    if (!qIsNaN(diff) && qAbs(diff) < AV_NOSYNC_THRESHOLD) {
        this->audioDiffCum = diff + this->audioDiffAvgCoef * this->audioDiffCum;

        if (this->audioDiffAvgCount < AUDIO_DIFF_AVG_NB) {
            // not enough measures to have a correct estimate
            this->audioDiffAvgCount++;
        } else {
            // estimate the A-V difference
            qreal avgDiff = this->audioDiffCum * (1.0 - this->audioDiffAvgCoef);

            // since we do not have a precise anough audio fifo fullness,
            // we correct audio sync only if larger than this threshold
            qreal diffThreshold = 2.0 * audioPacket.caps().samples() / audioPacket.caps().rate();

            if (qAbs(avgDiff) >= diffThreshold) {
                wantedSamples = audioPacket.caps().samples() + int(diff * audioPacket.caps().rate());
                int minSamples = audioPacket.caps().samples() * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                int maxSamples = audioPacket.caps().samples() * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                wantedSamples = qBound(minSamples, wantedSamples, maxSamples);
                audioPacket = audioPacket.scale(wantedSamples);
            }
        }
    } else {
        // Too big difference: may be initial PTS errors, so
        // reset A-V filter
        this->audioDiffAvgCount = 0;
        this->audioDiffCum = 0.0;
    }

    if (qAbs(diff) >= AV_NOSYNC_THRESHOLD)
        self->globalClock()->setClock(pts);

    self->clockDiff() = diff;

    return this->m_audioConvert->iStream(packet);
}

#include "moc_audiostream.cpp"
