/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QVariant>
#include <QtEndian>

#include "akaudiopacket.h"
#include "akcaps.h"

using AudioConvertFuntion =
    std::function<AkAudioPacket (const AkAudioPacket &src, int align)>;

class AkAudioPacketPrivate
{
    public:
        AkAudioCaps m_caps;

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValue(InputType value)
        {
            InputType xmin;
            InputType xmax;

            if (typeid(InputType) == typeid(float)) {
                value = qBound<InputType>(InputType(-1.0f),
                                          value,
                                          InputType(1.0f));
                xmin = InputType(-1.0f);
                xmax = InputType(1.0f);
            } else if (typeid(InputType) == typeid(qreal)) {
                value = qBound<InputType>(InputType(-1.0),
                                          value,
                                          InputType(1.0));
                xmin = InputType(-1.0);
                xmax = InputType(1.0);
            } else {
                xmin = std::numeric_limits<InputType>::min();
                xmax = std::numeric_limits<InputType>::max();
            }

            OutputType ymin;
            OutputType ymax;

            if (typeid(OutputType) == typeid(float)) {
                ymin = OutputType(-1.0f);
                ymax = OutputType(1.0f);
            } else if (typeid(InputType) == typeid(qreal)) {
                ymin = OutputType(-1.0);
                ymax = OutputType(1.0);
            } else {
                ymin = std::numeric_limits<OutputType>::min();
                ymax = std::numeric_limits<OutputType>::max();
            }

            return OutputType(((value - xmin) * (ymax - ymin)
                               + ymin * (xmax - xmin))
                              / (xmax - xmin));
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValueI(InputType value)
        {
            return scaleValue<InputType, OutputType>(value);
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValueFromBEToLE(InputType value)
        {
            return qFromLittleEndian(scaleValue<InputType, OutputType>(qFromBigEndian(value)));
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValueFromLEToBE(InputType value)
        {
            return qFromBigEndian(scaleValue<InputType, OutputType>(qFromLittleEndian(value)));
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValueFromBEToBE(InputType value)
        {
            return qFromBigEndian(scaleValue<InputType, OutputType>(qFromBigEndian(value)));
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValueFromLEToLE(InputType value)
        {
            return qFromLittleEndian(scaleValue<InputType, OutputType>(qFromLittleEndian(value)));
        }

#define DEFINE_SCALE_VALUES(funcName, func) \
        template<typename InputType, typename OutputType> \
        inline static OutputType funcName(InputType value) \
        { \
            return func<InputType, OutputType>(value); \
        }

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        DEFINE_SCALE_VALUES(scaleValueFromBE, scaleValueFromBEToLE)
        DEFINE_SCALE_VALUES(scaleValueFromLE, scaleValue          )
        DEFINE_SCALE_VALUES(scaleValueToBE  , scaleValueFromLEToBE)
        DEFINE_SCALE_VALUES(scaleValueToLE  , scaleValue          )
#else
        DEFINE_SCALE_VALUES(scaleValueFromBE, scaleValue          )
        DEFINE_SCALE_VALUES(scaleValueFromLE, scaleValueFromLEToBE)
        DEFINE_SCALE_VALUES(scaleValueToBE  , scaleValue          )
        DEFINE_SCALE_VALUES(scaleValueToLE  , scaleValueFromBEToLE)
#endif

        template<typename InputType,
                 typename OutputType,
                 typename ScaleFunc>
        inline static AkAudioPacket convertSampleFormat(const AkAudioPacket &src,
                                                        AkAudioCaps::SampleFormat format,
                                                        ScaleFunc scaleFunc,
                                                        int align)
        {
            auto caps = src.caps();
            caps.setFormat(format);
            caps.setAlign(align);
            AkAudioPacket dst(caps);
            dst.copyMetadata(src);

            for (int plane = 0; plane < caps.planes(); plane++) {
                auto src_line = reinterpret_cast<const InputType *>(src.constPlaneData(plane));
                auto dst_line = reinterpret_cast<OutputType *>(dst.planeData(plane));

                for (int i = 0; i < caps.samples(); i++)
                    dst_line[i] = scaleFunc(src_line[i]);
            }

            return dst;
        }

#define DEFINE_SAMPLE_CONVERT_FUNCTION(sitype, sotype, itype, otype, scaleFunc) \
        {AkAudioCaps::SampleFormat_##sitype, \
         AkAudioCaps::SampleFormat_##sotype, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
            return AkAudioPacketPrivate::convertSampleFormat<itype, otype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sotype, \
                     scaleFunc<itype, otype>, \
                     align); \
         }}, \
        {AkAudioCaps::SampleFormat_##sotype, \
         AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
             return AkAudioPacketPrivate::convertSampleFormat<otype, itype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sitype, \
                     scaleFunc<otype, itype>, \
                     align); \
         }}

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8_8 \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u8, qint8, quint8, scaleValueI)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_S8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, s##obits##le, qint8, qint##obits , scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, s##obits##be, qint8, qint##obits , scaleValueToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u##obits##le, qint8, quint##obits, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u##obits##be, qint8, quint##obits, scaleValueToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_U8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, s##obits##le, quint8, qint##obits , scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, s##obits##be, quint8, qint##obits , scaleValueToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, u##obits##le, quint8, quint##obits, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, u##obits##be, quint8, quint##obits, scaleValueToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_S8_M(obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_U8_M(obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_S8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltle, qint8, float, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltbe, qint8, float, scaleValueToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblle, qint8, qreal, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblbe, qint8, qreal, scaleValueToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_U8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltle, quint8, float, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltbe, quint8, float, scaleValueToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblle, quint8, qreal, scaleValueToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblbe, quint8, qreal, scaleValueToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION_S8_F, \
        DEFINE_SAMPLE_CONVERT_FUNCTION_U8_F

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8 \
        DEFINE_SAMPLE_CONVERT_FUNCTION_8_8, \
        DEFINE_SAMPLE_CONVERT_FUNCTION_8_M(16), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_8_M(32), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_8_M(64), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_8_F

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MS_M(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, s##bits##be, qint##bits, qint##bits, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, u##bits##le, qint##bits, qint##bits, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, u##bits##be, qint##bits, qint##bits, scaleValueFromLEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MU_M(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, u##bits##be, qint##bits, qint##bits, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, s##bits##be, qint##bits, qint##bits, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, s##bits##be, qint##bits, qint##bits, scaleValueFromBEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, s##obits##le, qint##ibits, qint##obits , scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, s##obits##be, qint##ibits, qint##obits , scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, u##obits##le, qint##ibits, quint##obits, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, u##obits##be, qint##ibits, quint##obits, scaleValueFromLEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, s##obits##le, quint##ibits, qint##obits , scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, s##obits##be, quint##ibits, qint##obits , scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, u##obits##le, quint##ibits, quint##obits, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, u##obits##be, quint##ibits, quint##obits, scaleValueFromLEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_ML_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_M(ibits, obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, s##obits##le, qint##ibits, qint##obits , scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, s##obits##be, qint##ibits, qint##obits , scaleValueFromBEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, u##obits##le, qint##ibits, quint##obits, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, u##obits##be, qint##ibits, quint##obits, scaleValueFromBEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, s##obits##le, quint##ibits, qint##obits , scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, s##obits##be, quint##ibits, qint##obits , scaleValueFromBEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, u##obits##le, quint##ibits, quint##obits, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, u##obits##be, quint##ibits, quint##obits, scaleValueFromBEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_M(ibits, obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MO_M(ibits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MS_M(ibits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MU_M(ibits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_ME_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_ML_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MB_M(ibits, obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltle, qint##bits, float, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltbe, qint##bits, float, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblle, qint##bits, qreal, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblbe, qint##bits, qreal, scaleValueFromLEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltle, quint##bits, float, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltbe, quint##bits, float, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblle, quint##bits, qreal, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblbe, quint##bits, qreal, scaleValueFromLEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltle, qint##bits, float, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltbe, qint##bits, float, scaleValueFromBEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblle, qint##bits, qreal, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblbe, qint##bits, qreal, scaleValueFromBEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltle, quint##bits, float, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltbe, quint##bits, float, scaleValueFromBEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblle, quint##bits, qreal, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblbe, quint##bits, qreal, scaleValueFromBEToBE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_M_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_F(bits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_F(bits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_F(bits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_F(bits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_M \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MO_M(16), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MO_M(32), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MO_M(64), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_ME_M(16, 32), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_ME_M(16, 64), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_ME_M(32, 64), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_M_F(16), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_M_F(32), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_M_F(64)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, fltbe, float, float, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblle, float, qreal, scaleValueFromLEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblbe, float, qreal, scaleValueFromLEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblle, float, qreal, scaleValueFromBEToLE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblbe, float, qreal, scaleValueFromBEToBE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(dblle, dblbe, qreal, qreal, scaleValueFromLEToBE)

        struct AudioSampleFormatConvert
        {
            AkAudioCaps::SampleFormat from;
            AkAudioCaps::SampleFormat to;
            AudioConvertFuntion convert;
        };

        using AudioSampleFormatConvertFuncs = QVector<AudioSampleFormatConvert>;

        inline static const AudioSampleFormatConvertFuncs &sampleFormatConvert()
        {
            // Convert sample formats
            static const AudioSampleFormatConvertFuncs convert {
                DEFINE_SAMPLE_CONVERT_FUNCTION_8,
                DEFINE_SAMPLE_CONVERT_FUNCTION_M,
                DEFINE_SAMPLE_CONVERT_FUNCTION_F
            };

            return convert;
        }

        template<typename InputType, typename OutputType>
        inline static OutputType scaleValue(InputType value,
                                            InputType minValue,
                                            InputType maxValue)
        {
            auto ymin = std::numeric_limits<OutputType>::min();
            auto ymax = std::numeric_limits<OutputType>::max();

            return OutputType(((value - minValue) * (ymax - ymin)
                               + ymin * (maxValue - minValue))
                              / (maxValue - minValue));
        }

        template<typename SampleType, typename SumType>
        inline static void scaleSamples(const QByteArray &sumFrame,
                                        int samples,
                                        SumType &smin,
                                        SumType &smax)
        {
            auto sumData =
                    reinterpret_cast<const SumType *>(sumFrame.constData());

            // Find the minimum and maximum values of the sum.
            smin = std::numeric_limits<SumType>::max();
            smax = std::numeric_limits<SumType>::min();

            for (int i = 0; i < samples; i++) {
                if (sumData[i] < smin)
                    smin = sumData[i];

                if (sumData[i] > smax)
                    smax = sumData[i];
            }

            // Limit the maximum and the minimum of the wave so it won't get
            // oyt of the bounds.
            auto minValue = std::numeric_limits<SampleType>::min();
            auto maxValue = std::numeric_limits<SampleType>::max();

            if (smin > SumType(minValue))
                smin = SumType(minValue);

            if (smax < SumType(maxValue))
                smax = SumType(maxValue);
        }

        template<typename SampleType, typename SumType>
        inline static void mixChannels(void *dstFrame,
                                       const QVector<const void *> &srcFrames,
                                       int samples)
        {
            // Create a summatory sample which type is big enough to contain
            // the sum of all values.
            QByteArray sumFrame(samples * int(sizeof(SumType)), 0);
            auto sumData = reinterpret_cast<SumType *>(sumFrame.data());

            // Add the values.
            for (auto &frame: srcFrames) {
                auto data = reinterpret_cast<const SampleType *>(frame);

                for (int i = 0; i < samples; i++)
                    sumData[i] += SumType(data[i]);
            }

            SumType smin;
            SumType smax;
            scaleSamples<SampleType, SumType>(sumFrame,
                                              samples,
                                              smin,
                                              smax);

            // Recreate frame with the wave scaled to fit it.
            auto mixedData = reinterpret_cast<SampleType *>(dstFrame);

            for (int i = 0; i < samples; i++)
                mixedData[i] =
                        scaleValue<SumType, SampleType>(sumData[i],
                                                        smin,
                                                        smax);
        }

        template<typename SampleType, typename SumType>
        inline static void mixChannels(void *dstFrame,
                                       const void *srcFrames,
                                       const QVector<int> &inputChannels,
                                       int outputChannel,
                                       int channels,
                                       int samples)
        {
            // Create a summatory sample which type is big enough to contain
            // the sum of all values.
            QByteArray sumFrame(samples * int(sizeof(SumType)), 0);
            auto sumData = reinterpret_cast<SumType *>(sumFrame.data());

            // Add the values.
            auto data = reinterpret_cast<const SampleType *>(srcFrames);

            for (auto &channel: inputChannels) {
                for (int i = 0; i < samples; i++)
                    sumData[i] += SumType(data[channel + i * channels]);
            }

            SumType smin;
            SumType smax;
            scaleSamples<SampleType, SumType>(sumFrame,
                                              samples,
                                              smin,
                                              smax);

            // Recreate frame with the wave scaled to fit it.
            auto mixedData = reinterpret_cast<SampleType *>(dstFrame);

            for (int i = 0; i < samples; i++)
                mixedData[outputChannel + i * channels] =
                        scaleValue<SumType, SampleType>(sumData[i],
                                                        smin,
                                                        smax);
        }

#define DEFIME_MIXER(sampleType, sumType) \
        inline static void mixChannels_##sampleType(void *dstFrame, \
                                                    const QVector<const void *> &frames, \
                                                    int samples) \
        { \
            mixChannels<sampleType, sumType>(dstFrame, frames, samples); \
        } \
        \
        inline static void mixChannels_##sampleType(void *dstFrame, \
                                                    const void *frames, \
                                                    const QVector<int> &inputChannels, \
                                                    int outputChannel, \
                                                    int channels, \
                                                    int samples) \
        { \
            mixChannels<sampleType, sumType>(dstFrame, \
                                             frames, \
                                             inputChannels, \
                                             outputChannel, \
                                             channels, \
                                             samples); \
        }

        DEFIME_MIXER(qint8, qint16)
        DEFIME_MIXER(quint8, quint16)
        DEFIME_MIXER(qint16, qint32)
        DEFIME_MIXER(quint16, quint32)
        DEFIME_MIXER(qint32, qint64)
        DEFIME_MIXER(quint32, quint64)
        DEFIME_MIXER(qint64, double)
        DEFIME_MIXER(quint64, double)
        DEFIME_MIXER(float, qreal)
        DEFIME_MIXER(qreal, qreal)

        struct AudioChannelLayoutConvert
        {
            AkAudioCaps::ChannelLayout from;
            AkAudioCaps::ChannelLayout to;
            AudioConvertFuntion convert;
        };

        using AudioChannelLayoutConvertFuncs = QVector<AudioChannelLayoutConvert>;

        inline static const AudioChannelLayoutConvertFuncs &channelLayoutConvert()
        {
            static const AudioChannelLayoutConvertFuncs convert {
            };

            return convert;
        }
};

AkAudioPacket::AkAudioPacket(QObject *parent):
    AkPacket(parent)
{
    this->d = new AkAudioPacketPrivate();
}

AkAudioPacket::AkAudioPacket(const AkAudioCaps &caps)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->buffer() = QByteArray(int(caps.frameSize()), Qt::Uninitialized);
}

AkAudioPacket::AkAudioPacket(const AkPacket &other)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkAudioPacket::AkAudioPacket(const AkAudioPacket &other):
    AkPacket()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkAudioPacket::~AkAudioPacket()
{
    delete this->d;
}

AkAudioPacket &AkAudioPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();

    return *this;
}

AkAudioPacket &AkAudioPacket::operator =(const AkAudioPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->data() = other.data();
        this->buffer() = other.buffer();
        this->pts() = other.pts();
        this->timeBase() = other.timeBase();
        this->index() = other.index();
        this->id() = other.id();
    }

    return *this;
}

AkAudioPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

AkAudioCaps AkAudioPacket::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps &AkAudioPacket::caps()
{
    return this->d->m_caps;
}

const quint8 *AkAudioPacket::constPlaneData(int plane) const
{
    return reinterpret_cast<const quint8 *>(this->buffer().constData())
            + this->d->m_caps.planeOffset(plane);
}

quint8 *AkAudioPacket::planeData(int plane)
{
    return reinterpret_cast<quint8 *>(this->buffer().data())
            + this->d->m_caps.planeOffset(plane);
}

QString AkAudioPacket::toString() const
{
    QString packetInfo;
    QDebug debug(&packetInfo);

    debug.nospace() << "Caps       : "
                    << this->d->m_caps.toString().toStdString().c_str()
                    << "\n"
                    << "Data       : "
                    << this->data()
                    << "\n"
                    << "Buffer Size: "
                    << this->buffer().size()
                    << "\n"
                    << "Id         : "
                    << this->id()
                    << "\n"
                    << "Pts        : "
                    << this->pts()
                    << " ("
                    << this->pts() * this->timeBase().value()
                    << ")\n"
                    << "Time Base  : "
                    << this->timeBase().toString().toStdString().c_str()
                    << "\n"
                    << "Index      : "
                    << this->index();

    return packetInfo;
}

AkPacket AkAudioPacket::toPacket() const
{
    AkPacket packet;
    packet.caps() =  this->d->m_caps.toCaps();
    packet.buffer() = this->buffer();
    packet.pts() = this->pts();
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    return packet;
}

bool AkAudioPacket::canConvert(AkAudioCaps::SampleFormat input,
                               AkAudioCaps::SampleFormat output)
{
    if (input == output)
        return true;

    for (auto &convert: AkAudioPacketPrivate::sampleFormatConvert())
        if (convert.from == input
            && convert.to == output) {
            return true;
        }

    return false;
}

bool AkAudioPacket::canConvert(AkAudioCaps::SampleFormat output) const
{
    return AkAudioPacket::canConvert(this->d->m_caps.format(), output);
}

AkAudioPacket AkAudioPacket::convert(AkAudioCaps::SampleFormat format) const
{
    return this->convert(format, this->d->m_caps.align());
}

AkAudioPacket AkAudioPacket::convert(AkAudioCaps::SampleFormat format,
                                     int align) const
{
    if (this->d->m_caps.format() == format) {
        if (this->d->m_caps.align() != align)
            return this->realign(align);

        return *this;
    }

    for (auto &convert: AkAudioPacketPrivate::sampleFormatConvert())
        if (convert.from == this->d->m_caps.format()
            && convert.to == format) {
            return convert.convert(*this, align);
        }

    return {};
}

bool AkAudioPacket::canConvertLayout(AkAudioCaps::ChannelLayout input,
                                     AkAudioCaps::ChannelLayout output)
{
    if (input == output)
        return true;

    for (auto &convert: AkAudioPacketPrivate::channelLayoutConvert())
        if (convert.from == input
            && convert.to == output) {
            return true;
        }

    return false;
}

bool AkAudioPacket::canConvertLayout(AkAudioCaps::ChannelLayout output) const
{
    return AkAudioPacket::canConvertLayout(this->d->m_caps.layout(), output);
}

AkAudioPacket AkAudioPacket::convertLayout(AkAudioCaps::ChannelLayout layout) const
{
    return this->convertLayout(layout, this->d->m_caps.align());
}

AkAudioPacket AkAudioPacket::convertLayout(AkAudioCaps::ChannelLayout layout,
                                           int align) const
{
    if (this->d->m_caps.layout() == layout) {
        if (this->d->m_caps.align() != align)
            return this->realign(align);

        return *this;
    }

    for (auto &convert: AkAudioPacketPrivate::channelLayoutConvert())
        if (convert.from == this->d->m_caps.layout()
            && convert.to == layout) {
            return convert.convert(*this, align);
        }

    return {};
}

AkAudioPacket AkAudioPacket::convertPlanar(bool planar) const
{
    return this->convertPlanar(planar, this->d->m_caps.align());
}

AkAudioPacket AkAudioPacket::convertPlanar(bool planar, int align) const
{
    if (this->d->m_caps.planar() == planar) {
        if (this->d->m_caps.align() != align)
            return this->realign(align);

        return *this;
    }

    auto caps = this->d->m_caps;
    caps.setPlanar(planar);
    caps.setAlign(align);
    AkAudioPacket dst(caps);
    dst.copyMetadata(*this);
    auto byps = caps.bps() / 8;
    auto channels = caps.channels();

    if (planar) {
        auto src_line = reinterpret_cast<const qint8 *>(this->constPlaneData(0));

        for (int plane = 0; plane < caps.planes(); plane++) {
            auto dst_line = reinterpret_cast<qint8 *>(dst.planeData(plane));

            for (int i = 0; i < caps.samples(); i++)
                memcpy(dst_line + byps * i,
                       src_line + byps * (i * channels + plane),
                       size_t(byps));
        }
    } else {
        auto dst_line = reinterpret_cast<qint8 *>(dst.planeData(0));

        for (int plane = 0; plane < caps.planes(); plane++) {
            auto src_line = reinterpret_cast<const qint8 *>(this->constPlaneData(plane));

            for (int i = 0; i < caps.samples(); i++)
                memcpy(dst_line + byps * (i * channels + plane),
                       src_line + byps * i,
                       size_t(byps));
        }
    }

    return dst;
}

AkAudioPacket AkAudioPacket::realign(int align) const
{
    if (this->d->m_caps.align() == align)
        return *this;

    auto caps = this->d->m_caps;
    caps.setAlign(align);
    AkAudioPacket dst(caps);
    dst.copyMetadata(*this);

    for (int plane = 0; plane < caps.planes(); plane++) {
        auto planeSize = qMin(caps.planeSize(), this->d->m_caps.planeSize());
        auto src_line = this->constPlaneData(plane);
        auto dst_line = dst.planeData(plane);
        memcpy(dst_line, src_line, planeSize);
    }

    return dst;
}

void AkAudioPacket::copyMetadata(const AkPacket &other)
{
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

void AkAudioPacket::copyMetadata(const AkAudioPacket &other)
{
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

void AkAudioPacket::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkAudioPacket::resetCaps()
{
    this->setCaps(AkAudioCaps());
}

QDebug operator <<(QDebug debug, const AkAudioPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}

#include "moc_akaudiopacket.cpp"
