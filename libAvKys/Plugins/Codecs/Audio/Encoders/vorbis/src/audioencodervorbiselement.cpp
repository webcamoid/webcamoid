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
#include <QVariant>
#include <akfrac.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>
#include <vorbis/vorbisenc.h>

#include "audioencodervorbiselement.h"

class AudioEncoderVorbisElementPrivate
{
    public:
        AudioEncoderVorbisElement *self;
        AkCompressedAudioCaps m_outputCaps;
        AkCompressedAudioPackets m_headers;
        vorbis_info m_info;
        vorbis_comment m_comment;
        vorbis_dsp_state m_dsp;
        vorbis_block m_block;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkCompressedAudioPacket m_prevPacket;
        AkElementPtr m_fillAudioGaps {akPluginManager->create<AkElement>("AudioFilter/FillAudioGaps")};

        explicit AudioEncoderVorbisElementPrivate(AudioEncoderVorbisElement *self);
        ~AudioEncoderVorbisElementPrivate();
        static const char *errorToString(int error);
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkAudioCaps &inputCaps);
        void encodeFrame(const AkAudioPacket &src);
        void sendFrame(const ogg_packet &oggPacket);

        inline static qsizetype xiphLen(qsizetype len)
        {
            return 1 + len / 255 + len;
        }

        inline static qsizetype xiphLacing(quint8 *p, qsizetype value)
        {
            qsizetype n = value / 255;

            if (n > 0)
                memset(p, 255, n);

            p[n] = value - 255 * n;

            return n + 1;
        }
};

AudioEncoderVorbisElement::AudioEncoderVorbisElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderVorbisElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

AudioEncoderVorbisElement::~AudioEncoderVorbisElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList AudioEncoderVorbisElement::codecs() const
{
    return {"vorbis"};
}

AkAudioEncoderCodecID AudioEncoderVorbisElement::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedAudioCaps::AudioCodecID_vorbis:
                AkCompressedAudioCaps::AudioCodecID_unknown;
}

QString AudioEncoderVorbisElement::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("Vorbis"):
                QString();
}

AkCompressedAudioCaps AudioEncoderVorbisElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets AudioEncoderVorbisElement::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 AudioEncoderVorbisElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

QString AudioEncoderVorbisElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AudioEncoderVorbis/share/qml/main.qml");
}

void AudioEncoderVorbisElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AudioEncoderVorbis", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AudioEncoderVorbisElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused
        || !this->d->m_initialized
        || !this->d->m_fillAudioGaps)
        return {};

    this->d->m_fillAudioGaps->iStream(packet);

    return {};
}

bool AudioEncoderVorbisElement::setState(ElementState state)
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

AudioEncoderVorbisElementPrivate::AudioEncoderVorbisElementPrivate(AudioEncoderVorbisElement *self):
    self(self)
{
    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("outputSamples", 64);
        QObject::connect(this->m_fillAudioGaps.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->encodeFrame(packet);
                         });
    }

    QObject::connect(self,
                     &AkAudioEncoder::inputCapsChanged,
                     [this] (const AkAudioCaps &inputCaps) {
                         this->updateOutputCaps(inputCaps);
                     });
}

AudioEncoderVorbisElementPrivate::~AudioEncoderVorbisElementPrivate()
{

}

const char *AudioEncoderVorbisElementPrivate::errorToString(int error)
{
    static const struct ErrorCodesStr
    {
        int code;
        const char *str;
    } vorbisEncErrorCodes[] = {
        {OV_FALSE     , "FALSE"     },
        {OV_EOF       , "EOF"       },
        {OV_HOLE      , "HOLE"      },
        {OV_EREAD     , "EREAD"     },
        {OV_EFAULT    , "EFAULT"    },
        {OV_EIMPL     , "EIMPL"     },
        {OV_EINVAL    , "EINVAL"    },
        {OV_ENOTVORBIS, "ENOTVORBIS"},
        {OV_EBADHEADER, "EBADHEADER"},
        {OV_EVERSION  , "EVERSION"  },
        {OV_ENOTAUDIO , "ENOTAUDIO" },
        {OV_EBADPACKET, "EBADPACKET"},
        {OV_EBADLINK  , "EBADLINK"  },
        {OV_ENOSEEK   , "ENOSEEK"   },
        {0            , ""          },
    };

    auto ec = vorbisEncErrorCodes;

    for (; ec->code; ++ec)
        if (ec->code == error)
            return ec->str;

    static char vorbisEncErrorStr[1024];
    snprintf(vorbisEncErrorStr, 1024, "%d", error);

    return vorbisEncErrorStr;
}

bool AudioEncoderVorbisElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    vorbis_info_init(&this->m_info);
    auto result = vorbis_encode_init(&this->m_info,
                                     this->m_outputCaps.rawCaps().channels(),
                                     this->m_outputCaps.rawCaps().rate(),
                                     -1,
                                     self->bitrate(),
                                     -1);

    if (result) {
        qCritical() << "Failed initializing the encoder (" << errorToString(result) << ")";

        return false;
    }

    vorbis_comment_init(&this->m_comment);
    vorbis_comment_add_tag(&this->m_comment,
                           "ENCODER",
                           QCoreApplication::applicationName().toStdString().c_str());

    vorbis_analysis_init(&this->m_dsp, &this->m_info);
    vorbis_block_init(&this->m_dsp, &this->m_block);

    this->updateHeaders();

    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("fillGaps", self->fillGaps());
        this->m_fillAudioGaps->setState(AkElement::ElementStatePlaying);
    }

    this->m_prevPacket = AkCompressedAudioPacket();
    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderVorbisElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setState(AkElement::ElementStateNull);

    vorbis_analysis_wrote(&this->m_dsp, 0);

    while (vorbis_analysis_blockout(&this->m_dsp, &this->m_block) == 1) {
        if (vorbis_analysis(&this->m_block, nullptr))
            break;

        if (vorbis_bitrate_addblock(&this->m_block))
            break;

        ogg_packet oggPacket;

        while (vorbis_bitrate_flushpacket(&this->m_dsp, &oggPacket) == 1)
            this->sendFrame(oggPacket);
    }

    vorbis_block_clear(&this->m_block);
    vorbis_dsp_clear(&this->m_dsp);
    vorbis_comment_clear(&this->m_comment);
    vorbis_info_clear(&this->m_info);

    this->m_prevPacket = AkCompressedAudioPacket();
    this->m_paused = false;
}

void AudioEncoderVorbisElementPrivate::updateHeaders()
{
    ogg_packet header;
    ogg_packet headerComment;
    ogg_packet headerCode;
    vorbis_analysis_headerout(&this->m_dsp,
                              &this->m_comment,
                              &header,
                              &headerComment,
                              &headerCode);

    qsizetype extraDataSize = 1
                              + xiphLen(header.bytes)
                              + xiphLen(headerComment.bytes)
                              + headerCode.bytes;
    AkCompressedAudioPacket headerPacket(this->m_outputCaps,
                                         extraDataSize);

    memset(headerPacket.data(), 2, 1);
    qsizetype offset = 1;
    offset += xiphLacing(reinterpret_cast<quint8 *>(headerPacket.data())
                         + offset,
                         header.bytes);
    offset += xiphLacing(reinterpret_cast<quint8 *>(headerPacket.data())
                         + offset,
                         headerComment.bytes);
    memcpy(headerPacket.data() + offset,
           header.packet,
           header.bytes);
    offset += header.bytes;
    memcpy(headerPacket.data() + offset,
           headerComment.packet,
           headerComment.bytes);
    offset += headerComment.bytes;
    memcpy(headerPacket.data() + offset,
           headerCode.packet,
           headerCode.bytes);

    headerPacket.setTimeBase({1, this->m_info.rate});
    headerPacket.setFlags(AkCompressedAudioPacket::AudioPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
}

void AudioEncoderVorbisElementPrivate::updateOutputCaps(const AkAudioCaps &inputCaps)
{
    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged({});

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedAudioCaps::AudioCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    int channels = qBound(1, inputCaps.channels(), 2);
    AkAudioCaps rawCaps(AkAudioCaps::SampleFormat_flt,
                        AkAudioCaps::defaultChannelLayout(channels),
                        true,
                        inputCaps.rate());
    AkCompressedAudioCaps outputCaps(codecID, rawCaps);

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setProperty("outputCaps",
                                           QVariant::fromValue(rawCaps));

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

void AudioEncoderVorbisElementPrivate::encodeFrame(const AkAudioPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    auto vorbisBuffer =
            vorbis_analysis_buffer(&this->m_dsp, src.samples());

    for (int channel = 0; channel < src.caps().channels(); channel++)
        memcpy(vorbisBuffer[channel],
               src.constPlane(channel),
               src.planeSize(channel));

    vorbis_analysis_wrote(&this->m_dsp, src.samples());

    while (vorbis_analysis_blockout(&this->m_dsp, &this->m_block) == 1) {
        if (vorbis_analysis(&this->m_block, nullptr))
            break;

        if (vorbis_bitrate_addblock(&this->m_block))
            break;

        ogg_packet oggPacket;

        while (vorbis_bitrate_flushpacket(&this->m_dsp, &oggPacket) == 1)
            this->sendFrame(oggPacket);
    }

    this->m_encodedTimePts += src.samples();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void AudioEncoderVorbisElementPrivate::sendFrame(const ogg_packet &oggPacket)
{
    AkCompressedAudioPacket::ExtraDataPackets extraData {
        {sizeof(ogg_packet), Qt::Uninitialized}
    };
    memcpy(extraData[0].data(), &oggPacket, extraData[0].size());

    AkCompressedAudioPacket packet(this->m_outputCaps, oggPacket.bytes);
    memcpy(packet.data(), oggPacket.packet, packet.size());
    packet.setPts(oggPacket.granulepos);
    packet.setDts(oggPacket.granulepos);
    packet.setDuration(oggPacket.granulepos - this->m_prevPacket.pts());
    packet.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);
    packet.setExtraData(extraData);

    if (this->m_prevPacket) {
        this->m_prevPacket.setDuration(oggPacket.granulepos - this->m_prevPacket.pts());
        emit self->oStream(this->m_prevPacket);
    }

    this->m_prevPacket = packet;
}

#include "moc_audioencodervorbiselement.cpp"
