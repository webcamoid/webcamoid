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
            this->dataEnqueue(packet);

        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);
    }

    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        this->dataEnqueue({});
        this->d->m_eos = true;

        return false;
    }

    return true;
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
    emit this->oStream(packet);
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
    int samples = 8
                  * int(bufferSize)
                  / (AkAudioCaps::channelCount(layout)
                     * AkAudioCaps::bitsPerSample(sampleFormat));
    AkAudioPacket packet({sampleFormat,
                          layout,
                          false,
                          rate}, samples);
    bufferSize = qMin<size_t>(qMin<size_t>(packet.size(), bufferSize), info.size);
    memcpy(packet.data(), buffer + info.offset, bufferSize);
    packet.setPts(info.presentationTimeUs);
    packet.setTimeBase(AkFrac(1, 1e6));
    packet.setIndex(self->index());
    packet.setId(self->id());

    return packet;
}

#include "moc_audiostream.cpp"
