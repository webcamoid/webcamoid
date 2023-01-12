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

#include <QMutex>
#include <QSharedPointer>
#include <QVector>
#include <QWaitCondition>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <media/NdkMediaCodec.h>

#include "audiostream.h"
#include "mediawriterndkmedia.h"

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
        AkAudioConverter m_audioConvert;
        AkAudioPacket m_frame;
        QMutex m_frameMutex;
        int64_t m_pts {0};
        QWaitCondition m_frameReady;
        AkAudioCaps m_caps;
};

AudioStream::AudioStream(AMediaMuxer *mediaMuxer,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         const QMap<QString, QVariantMap> &codecOptions,
                         MediaWriterNDKMedia *mediaWriter,
                         QObject *parent):
    AbstractStream(mediaMuxer,
                   index, streamIndex,
                   configs,
                   codecOptions,
                   mediaWriter,
                   parent)
{
    this->d = new AudioStreamPrivate;
    this->d->m_caps = configs["caps"].value<AkCaps>();

#if __ANDROID_API__ >= 28
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_PCM_ENCODING,
                          AudioStream::encodingFromSampleFormat(this->d->m_caps.format()));
#endif
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_CHANNEL_MASK,
                          AudioStream::channelMaskFromLayout(this->d->m_caps.layout()));
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                          this->d->m_caps.channels());
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_SAMPLE_RATE,
                          this->d->m_caps.rate());
    this->d->m_audioConvert.setOutputCaps(this->d->m_caps);
}

AudioStream::~AudioStream()
{
    this->uninit();
    delete this->d;
}

int32_t AudioStream::encodingFromSampleFormat(AkAudioCaps::SampleFormat format)
{
    static const QMap<AkAudioCaps::SampleFormat, int32_t> encodingFromSampleFormat {
        {AkAudioCaps::SampleFormat_u8 , ENCODING_PCM_8BIT },
        {AkAudioCaps::SampleFormat_s16, ENCODING_PCM_16BIT},
        {AkAudioCaps::SampleFormat_flt, ENCODING_PCM_FLOAT},
    };

    return encodingFromSampleFormat.value(format);
}

int32_t AudioStream::channelMaskFromLayout(AkAudioCaps::ChannelLayout layout)
{
    int32_t mask = 0;

    for (auto position: AkAudioCaps::positions(layout))
        mask |= channelMaskToPosition().key(position);

    return mask;
}

void AudioStream::convertPacket(const AkPacket &packet)
{
    if (!packet)
        return;

    auto iPacket = AkAudioPacket(this->d->m_audioConvert.convert(packet));

    if (!iPacket)
        return;

    this->d->m_frameMutex.lock();

    if (this->d->m_frame)
        this->d->m_frame += iPacket;
    else
        this->d->m_frame = iPacket;

    this->d->m_frameReady.wakeAll();
    this->d->m_frameMutex.unlock();
}

void AudioStream::encode(const AkPacket &packet,
                         uint8_t *buffer,
                         size_t bufferSize)
{
    memcpy(buffer,
           packet.constData(),
           qMin(packet.size(), bufferSize));
}

AkPacket AudioStream::avPacketDequeue(size_t bufferSize)
{
    if (bufferSize < 1)
        return {};

    this->d->m_frameMutex.lock();
    int samples = 8 * int(bufferSize)
                  / (this->d->m_caps.bps()
                     * this->d->m_caps.channels());

    if (this->d->m_frame.samples() < samples) {
        if (!this->d->m_frameReady.wait(&this->d->m_frameMutex,
                                        THREAD_WAIT_LIMIT)) {
            this->d->m_frameMutex.unlock();

            return {};
        } else if (this->d->m_frame.samples() < samples) {
            this->d->m_frameMutex.unlock();

            return {};
        }
    }

    auto frame = this->d->m_frame.pop(samples);
    this->d->m_frameMutex.unlock();

    return frame;
}

bool AudioStream::init()
{
    auto result = AbstractStream::init();
    this->d->m_audioConvert.reset();

    return result;
}

#include "moc_audiostream.cpp"
