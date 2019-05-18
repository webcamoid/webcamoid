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
#include "akpacket.h"
#include "akcaps.h"
#include "akfrac.h"

using AudioConvertFuntion =
    std::function<AkAudioPacket (const AkAudioPacket &src, int align)>;

class AkAudioPacketPrivate
{
    public:
        AkAudioCaps m_caps;
        QByteArray m_buffer;
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};

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
            return convertSampleFormat<itype, otype> \
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
             return convertSampleFormat<otype, itype> \
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
        inline static void mixChannels_(void *dstFrame,
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

        template<typename SampleType, typename SumType>
        inline static void mixChannelsLE(void *dstFrame,
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
                        sumData[i] += SumType(qFromLittleEndian(data[i]));
                }
            } else {
                auto data = reinterpret_cast<const SampleType *>(srcFrames.first());

                for (auto &channel: inputChannels) {
                    for (int i = 0; i < samples; i++)
                        sumData[i] +=
                                SumType(qFromLittleEndian(data[channel + i * nInputChannels]));
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
                        qToLittleEndian(scaleValue<SumType,
                                                   SampleType,
                                                   SumType>(sumData[i],
                                                            smin,
                                                            smax));
        }

        template<typename SampleType, typename SumType>
        inline static void mixChannelsBE(void *dstFrame,
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
                        sumData[i] += SumType(qFromBigEndian(data[i]));
                }
            } else {
                auto data = reinterpret_cast<const SampleType *>(srcFrames.first());

                for (auto &channel: inputChannels) {
                    for (int i = 0; i < samples; i++)
                        sumData[i] +=
                                SumType(qFromBigEndian(data[channel + i * nInputChannels]));
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
                        qToBigEndian(scaleValue<SumType,
                                                SampleType,
                                                SumType>(sumData[i],
                                                         smin,
                                                         smax));
        }

#define HANDLE_CASE(where, sampleFormat, endian, sampleType, sumType) \
        case AkAudioCaps::SampleFormat_##sampleFormat: \
            mixFunc = mixChannels##endian<sampleType, sumType>; \
            break;

        inline static AkAudioPacket mapChannels(AkAudioCaps::ChannelLayout outputLayout,
                                                const AkAudioPacket &src,
                                                const QMap<int, QVector<int>> &channelMap,
                                                int align)
        {
            std::function<void (void *dstFrame,
                                int outputChannel,
                                int nOutputChannels,
                                const QVector<const void *> &srcFrames,
                                const QVector<int> &inputChannels,
                                int nInputChannels,
                                int samples)> mixFunc;

            switch (src.caps().format()) {
            HANDLE_CASE(mixFunc, s8   ,  _, qint8  , qint16 )
            HANDLE_CASE(mixFunc, u8   ,  _, quint8 , quint16)
            HANDLE_CASE(mixFunc, s16le, LE, qint16 , qint32 )
            HANDLE_CASE(mixFunc, s16be, BE, qint16 , qint32 )
            HANDLE_CASE(mixFunc, u16le, LE, quint16, quint32)
            HANDLE_CASE(mixFunc, u16be, BE, quint16, quint32)
            HANDLE_CASE(mixFunc, s32le, LE, qint32 , qint64 )
            HANDLE_CASE(mixFunc, s32be, BE, qint32 , qint64 )
            HANDLE_CASE(mixFunc, u32le, LE, quint32, quint64)
            HANDLE_CASE(mixFunc, u32be, BE, quint32, quint64)
            HANDLE_CASE(mixFunc, s64le, LE, qint64 , qreal  )
            HANDLE_CASE(mixFunc, s64be, BE, qint64 , qreal  )
            HANDLE_CASE(mixFunc, u64le, LE, quint64, qreal  )
            HANDLE_CASE(mixFunc, u64be, BE, quint64, qreal  )
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
            HANDLE_CASE(mixFunc, fltle, LE, float  , qreal  )
            HANDLE_CASE(mixFunc, fltbe, BE, float  , qreal  )
            HANDLE_CASE(mixFunc, dblle, LE, qreal  , qreal  )
            HANDLE_CASE(mixFunc, dblbe, BE, qreal  , qreal  )
#else
            HANDLE_CASE(mixFunc, fltle,  _, float  , qreal  )
            HANDLE_CASE(mixFunc, fltbe,  _, float  , qreal  )
            HANDLE_CASE(mixFunc, dblle,  _, qreal  , qreal  )
            HANDLE_CASE(mixFunc, dblbe,  _, qreal  , qreal  )
#endif
            default:
                return {};
            }

            auto caps = src.caps();
            caps.setLayout(outputLayout);
            caps.setAlign(align);
            AkAudioPacket dst(caps);
            dst.buffer().fill(0);

            for (auto it = channelMap.begin(); it != channelMap.end(); it++) {
                quint8 *outputFrame;
                int outputChannel;
                int nOutputChannels;

                if (dst.caps().planar()) {
                    outputFrame = dst.planeData(it.key());
                    outputChannel = 0;
                    nOutputChannels = 1;
                } else {
                    outputFrame = dst.planeData(0);
                    outputChannel = it.key();
                    nOutputChannels = dst.caps().channels();
                }

                QVector<const void *> srcFrames;

                if (src.caps().planar()) {
                    for (auto &key: it.value())
                        srcFrames << src.constPlaneData(key);
                } else {
                    srcFrames << src.constPlaneData(0);
                }

                mixFunc(outputFrame,
                        outputChannel,
                        nOutputChannels,
                        srcFrames,
                        it.value(),
                        src.caps().channels(),
                        src.caps().samples());
            }

            return dst;
        }

#define DEFINE_LAYOUT_CONVERT_FUNCTION(ilayout, \
                                       olayout, \
                                       imap, \
                                       omap) \
        {AkAudioCaps::Layout_##ilayout, \
         AkAudioCaps::Layout_##olayout, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
            return mapChannels \
                    (AkAudioCaps::Layout_##olayout, \
                     src, \
                     QMap<int, QVector<int>> imap, \
                     align); \
         }}, \
        {AkAudioCaps::Layout_##olayout, \
         AkAudioCaps::Layout_##ilayout, \
         [] (const AkAudioPacket &src, int align) -> AkAudioPacket { \
             return mapChannels \
                    (AkAudioCaps::Layout_##ilayout, \
                     src, \
                     QMap<int, QVector<int>> omap, \
                     align); \
         }}

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
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, stereo       , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, downmix      , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 2p1          , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2}}}))               ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 3p0          , ({{2, {0}}})          , ({{0, {0, 1, 2}}}))               ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 3p0_back     , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2}}}))               ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 3p1          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 4p0          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, quad         , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, quad_side    , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 4p1          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4}}}))         ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 5p0          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4}}}))         ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 5p0_side     , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4}}}))         ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 5p1          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 5p1_side     , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 6p0          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 6p0_front    , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2, 3, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, hexagonal    , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 6p1          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6}}}))   ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 6p1_back     , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6}}}))   ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 6p1_front    , ({{0, {0}}, {1, {0}}}), ({{0, {0, 1, 2, 3, 4, 5, 6}}}))   ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 7p0          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6}}}))   ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 7p0_front    , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6}}}))   ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 7p1          , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 7p1_wide     , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, 7p1_wide_back, ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, octagonal    , ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(mono, hexadecagonal, ({{2, {0}}})          , ({{0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}}})),

                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, downmix      , ({{0, {0}}, {1, {1}}}), ({{0, {0}}, {1, {1}}}))                        ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 2p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2}}, {1, {1, 2}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 3p0          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2}}, {1, {1, 2}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 3p0_back     , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2}}, {1, {1, 2}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 3p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3}}, {1, {1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 4p0          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3}}, {1, {1, 2, 3}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, quad         , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2}}, {1, {1, 3}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, quad_side    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2}}, {1, {1, 3}}}))                  ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 4p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 4}}, {1, {1, 2, 3, 4}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 5p0          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3}}, {1, {1, 2, 4}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 5p0_side     , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3}}, {1, {1, 2, 4}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 5p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 5p1_side     , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 6p0          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 6p0_front    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 4}}, {1, {1, 3, 5}}}))            ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, hexagonal    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 5}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 6p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 6}}, {1, {1, 2, 4, 5, 6}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 6p1_back     , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 6}}, {1, {1, 2, 4, 5, 6}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 6p1_front    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 4, 6}}, {1, {1, 3, 5, 6}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 7p0          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 6}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 7p0_front    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5}}, {1, {1, 2, 4, 6}}}))      ,
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 7p1          , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 6}}, {1, {1, 2, 4, 5, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 7p1_wide     , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 6}}, {1, {1, 2, 4, 5, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, 7p1_wide_back, ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 6}}, {1, {1, 2, 4, 5, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, octagonal    , ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 7}}, {1, {1, 2, 4, 6, 7}}})),
                DEFINE_LAYOUT_CONVERT_FUNCTION(stereo, hexadecagonal, ({{0, {0}}, {1, {1}}}), ({{0, {0, 2, 3, 5, 7, 8, 10, 12, 14}}, {1, {1, 2, 4, 6, 7, 9, 11, 13, 15}}})),
            };

            return convert;
        }
};

AkAudioPacket::AkAudioPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioPacketPrivate();
}

AkAudioPacket::AkAudioPacket(const AkAudioCaps &caps)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->d->m_buffer = QByteArray(int(caps.frameSize()), Qt::Uninitialized);
}

AkAudioPacket::AkAudioPacket(const AkPacket &other)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();
}

AkAudioPacket::AkAudioPacket(const AkAudioPacket &other):
    QObject()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

AkAudioPacket::~AkAudioPacket()
{
    delete this->d;
}

AkAudioPacket &AkAudioPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();

    return *this;
}

AkAudioPacket &AkAudioPacket::operator =(const AkAudioPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
    }

    return *this;
}

AkAudioPacket AkAudioPacket::operator +(const AkAudioPacket &other)
{
    if (this->caps().format() != other.caps().format()
        || this->caps().layout() != other.caps().layout()
        || this->caps().rate() != other.caps().rate()
        || this->caps().planar() != other.caps().planar())
        return {};

    auto caps = this->caps();
    caps.setSamples(this->caps().samples() + other.caps().samples());
    AkAudioPacket packet(caps);
/*
    for (int plane = 0; plane < caps.planes(); plane++) {
        memcpy(this->planeData(plane),
               other.constPlaneData(plane),
               );
        memcpy(packet.planeData(plane),
               other.constPlaneData(plane),
               );
    }
*/
    return packet;
}

AkAudioPacket &AkAudioPacket::operator +=(const AkAudioPacket &other)
{
    return *this;
}

AkAudioPacket::operator AkPacket() const
{
    AkPacket packet(this->d->m_caps);
    packet.buffer() = this->d->m_buffer;
    packet.pts() = this->d->m_pts;
    packet.timeBase() = this->d->m_timeBase;
    packet.index() = this->d->m_index;
    packet.id() = this->d->m_id;

    return packet;
}

AkAudioPacket::operator bool() const
{
    return this->d->m_caps;
}

AkAudioCaps AkAudioPacket::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps &AkAudioPacket::caps()
{
    return this->d->m_caps;
}

QByteArray AkAudioPacket::buffer() const
{
    return this->d->m_buffer;
}

QByteArray &AkAudioPacket::buffer()
{
    return this->d->m_buffer;
}

qint64 AkAudioPacket::id() const
{
    return this->d->m_id;
}

qint64 &AkAudioPacket::id()
{
    return this->d->m_id;
}

qint64 AkAudioPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &AkAudioPacket::pts()
{
    return this->d->m_pts;
}

AkFrac AkAudioPacket::timeBase() const
{
    return this->d->m_timeBase;
}

AkFrac &AkAudioPacket::timeBase()
{
    return this->d->m_timeBase;
}

int AkAudioPacket::index() const
{
    return this->d->m_index;
}

int &AkAudioPacket::index()
{
    return this->d->m_index;
}

void AkAudioPacket::copyMetadata(const AkAudioPacket &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

const quint8 *AkAudioPacket::constPlaneData(int plane) const
{
    return reinterpret_cast<const quint8 *>(this->d->m_buffer.constData())
            + this->d->m_caps.planeOffset(plane);
}

quint8 *AkAudioPacket::planeData(int plane)
{
    return reinterpret_cast<quint8 *>(this->d->m_buffer.data())
            + this->d->m_caps.planeOffset(plane);
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

void AkAudioPacket::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkAudioPacket::setBuffer(const QByteArray &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    emit this->bufferChanged(buffer);
}

void AkAudioPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkAudioPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkAudioPacket::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkAudioPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkAudioPacket::resetCaps()
{
    this->setCaps(AkAudioCaps());
}

void AkAudioPacket::resetBuffer()
{
    this->setBuffer({});
}

void AkAudioPacket::resetId()
{
    this->setId(-1);
}

void AkAudioPacket::resetPts()
{
    this->setPts(0);
}

void AkAudioPacket::resetTimeBase()
{
    this->setTimeBase({});
}

void AkAudioPacket::resetIndex()
{
    this->setIndex(-1);
}

QDebug operator <<(QDebug debug, const AkAudioPacket &packet)
{
    debug.nospace() << "AkAudioPacket("
                    << "caps="
                    << packet.caps()
                    << ",bufferSize="
                    << packet.buffer().size()
                    << ",id="
                    << packet.id()
                    << ",pts="
                    << packet.pts()
                    << "("
                    << packet.pts() * packet.timeBase().value()
                    << ")"
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ")";

    return debug.space();
}

#include "moc_akaudiopacket.cpp"
