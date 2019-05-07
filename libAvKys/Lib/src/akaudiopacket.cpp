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

        template<typename InputType, typename OutputType, typename OpType>
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

            return OutputType((OpType(value - xmin) * OpType(ymax - ymin)
                               + OpType(ymin) * OpType(xmax - xmin))
                              / OpType(xmax - xmin));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFrom_To_(InputType value)
        {
            return scaleValue<InputType, OutputType, OpType>(value);
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFrom_ToLE(InputType value)
        {
            return qToLittleEndian(scaleValue<InputType,
                                              OutputType,
                                              OpType>(value));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFrom_ToBE(InputType value)
        {
            return qToLittleEndian(scaleValue<InputType,
                                              OutputType,
                                              OpType>(value));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromLETo_(InputType value)
        {
            return scaleValue<InputType,
                              OutputType,
                              OpType>(qFromLittleEndian(value));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromLEToLE(InputType value)
        {
            return qToLittleEndian(scaleValue
                                   <InputType,
                                    OutputType,
                                    OpType>(qFromLittleEndian(value)));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromLEToBE(InputType value)
        {
            return qToBigEndian(scaleValue
                                <InputType,
                                 OutputType,
                                 OpType>(qFromLittleEndian(value)));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromBETo_(InputType value)
        {
            return scaleValue<InputType,
                              OutputType,
                              OpType>(qFromBigEndian(value));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromBEToLE(InputType value)
        {
            return qToLittleEndian(scaleValue<InputType,
                                              OutputType,
                                              OpType>(qFromBigEndian(value)));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValueFromBEToBE(InputType value)
        {
            return qToBigEndian(scaleValue<InputType,
                                           OutputType,
                                           OpType>(qFromBigEndian(value)));
        }

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

#define DEFINE_SAMPLE_CONVERT_FUNCTION(sitype, \
                                       sotype, \
                                       itype, \
                                       otype, \
                                       optype, \
                                       inEndian, \
                                       outEndian) \
        {AkAudioCaps::SampleFormat_##sitype, \
         AkAudioCaps::SampleFormat_##sotype, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
            return AkAudioPacketPrivate::convertSampleFormat<itype, otype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sotype, \
                     scaleValueFrom##inEndian##To##outEndian<itype, \
                                                             otype, \
                                                             optype>, \
                     align); \
         }}, \
        {AkAudioCaps::SampleFormat_##sotype, \
         AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
             return AkAudioPacketPrivate::convertSampleFormat<otype, itype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sitype, \
                     scaleValueFrom##outEndian##To##inEndian<otype, \
                                                             itype, \
                                                             optype>, \
                     align); \
         }}

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8_8 \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u8, qint8, quint8, qint16, _, _)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_S8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, s##obits##le, qint8, qint##obits , qint64, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, s##obits##be, qint8, qint##obits , qint64, _, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u##obits##le, qint8, quint##obits, qint64, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, u##obits##be, qint8, quint##obits, qint64, _, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_U8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, s##obits##le, quint8, qint##obits , qint64, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, s##obits##be, quint8, qint##obits , qint64, _, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, u##obits##le, quint8, quint##obits, qint64, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, u##obits##be, quint8, quint##obits, qint64, _, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_8_M(obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_S8_M(obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_U8_M(obits)

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_S8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltle, qint8, float, float, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltbe, qint8, float, float, _, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblle, qint8, qreal, qreal, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblbe, qint8, qreal, qreal, _, BE)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_U8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltle, quint8, float, float, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltbe, quint8, float, float, _, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblle, quint8, qreal, qreal, _, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblbe, quint8, qreal, qreal, _, BE)
#else
#define DEFINE_SAMPLE_CONVERT_FUNCTION_S8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltle, qint8, float, float, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, fltbe, qint8, float, float, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblle, qint8, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s8, dblbe, qint8, qreal, qreal, _, _)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_U8_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltle, quint8, float, float, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, fltbe, quint8, float, float, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblle, quint8, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u8, dblbe, quint8, qreal, qreal, _, _)
#endif

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
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, s##bits##be, qint##bits, qint##bits, qint64, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, u##bits##le, qint##bits, qint##bits, qint64, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, u##bits##be, qint##bits, qint##bits, qint64, LE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MU_M(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, u##bits##be, qint##bits, qint##bits, qint64, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, s##bits##be, qint##bits, qint##bits, qint64, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, s##bits##be, qint##bits, qint##bits, qint64, BE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, s##obits##le, qint##ibits, qint##obits , qint64, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, s##obits##be, qint##ibits, qint##obits , qint64, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, u##obits##le, qint##ibits, quint##obits, qint64, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##le, u##obits##be, qint##ibits, quint##obits, qint64, LE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, s##obits##le, quint##ibits, qint##obits , qint64, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, s##obits##be, quint##ibits, qint##obits , qint64, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, u##obits##le, quint##ibits, quint##obits, qint64, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##le, u##obits##be, quint##ibits, quint##obits, qint64, LE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_ML_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_M(ibits, obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, s##obits##le, qint##ibits, qint##obits , qint64, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, s##obits##be, qint##ibits, qint##obits , qint64, BE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, u##obits##le, qint##ibits, quint##obits, qint64, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##ibits##be, u##obits##be, qint##ibits, quint##obits, qint64, BE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, s##obits##le, quint##ibits, qint##obits , qint64, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, s##obits##be, quint##ibits, qint##obits , qint64, BE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, u##obits##le, quint##ibits, quint##obits, qint64, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##ibits##be, u##obits##be, quint##ibits, quint##obits, qint64, BE, BE)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MB_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_M(ibits, obits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_MO_M(ibits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MS_M(ibits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MU_M(ibits)

#define DEFINE_SAMPLE_CONVERT_FUNCTION_ME_M(ibits, obits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION_ML_M(ibits, obits), \
        DEFINE_SAMPLE_CONVERT_FUNCTION_MB_M(ibits, obits)

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltle, qint##bits, float, float, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltbe, qint##bits, float, float, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblle, qint##bits, qreal, qreal, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblbe, qint##bits, qreal, qreal, LE, BE)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltle, quint##bits, float, float, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltbe, quint##bits, float, float, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblle, quint##bits, qreal, qreal, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblbe, quint##bits, qreal, qreal, LE, BE)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltle, qint##bits, float, float, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltbe, qint##bits, float, float, BE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblle, qint##bits, qreal, qreal, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblbe, qint##bits, qreal, qreal, BE, BE)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltle, quint##bits, float, float, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltbe, quint##bits, float, float, BE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblle, quint##bits, qreal, qreal, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblbe, quint##bits, qreal, qreal, BE, BE)
#else
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltle, qint##bits, float, float, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, fltbe, qint##bits, float, float, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblle, qint##bits, qreal, qreal, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##le, dblbe, qint##bits, qreal, qreal, LE, _)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUL_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltle, quint##bits, float, float, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, fltbe, quint##bits, float, float, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblle, quint##bits, qreal, qreal, LE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##le, dblbe, quint##bits, qreal, qreal, LE, _)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MSB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltle, qint##bits, float, float, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, fltbe, qint##bits, float, float, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblle, qint##bits, qreal, qreal, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(s##bits##be, dblbe, qint##bits, qreal, qreal, BE, _)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_MUB_F(bits) \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltle, quint##bits, float, float, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, fltbe, quint##bits, float, float, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblle, quint##bits, qreal, qreal, BE, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(u##bits##be, dblbe, quint##bits, qreal, qreal, BE, _)
#endif

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
#define DEFINE_SAMPLE_CONVERT_FUNCTION_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, fltbe, float, float, float, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblle, float, qreal, qreal, LE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblbe, float, qreal, qreal, LE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblle, float, qreal, qreal, BE, LE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblbe, float, qreal, qreal, BE, BE), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(dblle, dblbe, qreal, qreal, qreal, LE, BE)
#else
#define DEFINE_SAMPLE_CONVERT_FUNCTION_F \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, fltbe, float, float, float, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblle, float, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dblbe, float, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblle, float, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dblbe, float, qreal, qreal, _, _), \
        DEFINE_SAMPLE_CONVERT_FUNCTION(dblle, dblbe, qreal, qreal, qreal, _, _)
#endif

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

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValue(InputType value,
                                            InputType minValue,
                                            InputType maxValue)
        {
            auto ymin = std::numeric_limits<OutputType>::min();
            auto ymax = std::numeric_limits<OutputType>::max();

            return OutputType((OpType(value - minValue) * OpType(ymax - ymin)
                               + OpType(ymin) * OpType(maxValue - minValue))
                              / OpType(maxValue - minValue));
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
                                       int outputChannel,
                                       int nOutputChannels,
                                       const QVector<const void *> &srcFrames,
                                       const QVector<int> &inputChannels,
                                       int nInputChannels,
                                       int samples)
        {
            // Create a summatory sample which type is big enough to contain
            // the sum of all values.
            QByteArray sumFrame(samples * int(sizeof(SumType)), 0);
            auto sumData = reinterpret_cast<SumType *>(sumFrame.data());

            // Add the values.
            if (srcFrames.size() > 1) {
                for (auto &frame: srcFrames) {
                    auto data = reinterpret_cast<const SampleType *>(frame);

                    for (int i = 0; i < samples; i++)
                        sumData[i] += SumType(data[i]);
                }
            } else {
                auto data = reinterpret_cast<const SampleType *>(srcFrames.first());

                for (auto &channel: inputChannels) {
                    for (int i = 0; i < samples; i++)
                        sumData[i] +=
                                SumType(data[channel + i * nInputChannels]);
                }
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
                mixedData[outputChannel + i * nOutputChannels] =
                        scaleValue<SumType, SampleType, SumType>(sumData[i],
                                                                 smin,
                                                                 smax);
        }

#define DEFIME_MIXER(sampleType, sumType) \
        inline static void mixChannels_##sampleType(void *dstFrame, \
                                                    int outputChannel, \
                                                    int nOutputChannels, \
                                                    const QVector<const void *> &srcFrames, \
                                                    const QVector<int> &inputChannels, \
                                                    int nInputChannels, \
                                                    int samples) \
        { \
            mixChannels<sampleType, sumType>(dstFrame, \
                                             outputChannel, \
                                             nOutputChannels, \
                                             srcFrames, \
                                             inputChannels, \
                                             nInputChannels, \
                                             samples); \
        }

        DEFIME_MIXER(qint8  , qint16 )
        DEFIME_MIXER(quint8 , quint16)
        DEFIME_MIXER(qint16 , qint32 )
        DEFIME_MIXER(quint16, quint32)
        DEFIME_MIXER(qint32 , qint64 )
        DEFIME_MIXER(quint32, quint64)
        DEFIME_MIXER(qint64 , qreal  )
        DEFIME_MIXER(quint64, qreal  )
        DEFIME_MIXER(float  , qreal  )
        DEFIME_MIXER(qreal  , qreal  )

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
