/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
#include <QGenericMatrix>
#include <QMutex>
#include <QQmlEngine>
#include <QtEndian>
#include <QtMath>

#include "akaudioconverter.h"
#include "akaudiopacket.h"
#include "akfrac.h"

using AudioConvertFuntion =
    std::function<AkAudioPacket (const AkAudioPacket &src)>;

class AkAudioConverterPrivate
{
    public:
        QMutex m_mutex;
        AkAudioCaps m_outputCaps;
        AkAudioCaps m_previousCaps;
        AkAudioConverter::ResampleMethod m_resampleMethod {AkAudioConverter::ResampleMethod_Fast};
        qreal m_sampleCorrection {0};

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
            } else if (typeid(OutputType) == typeid(qreal)) {
                ymin = OutputType(-1.0);
                ymax = OutputType(1.0);
            } else {
                ymin = std::numeric_limits<OutputType>::min();
                ymax = std::numeric_limits<OutputType>::max();
            }

            return OutputType(((OpType(value) - OpType(xmin))
                               * (OpType(ymax) - OpType(ymin))
                               + OpType(ymin)
                               * (OpType(xmax) - OpType(xmin)))
                               / (OpType(xmax) - OpType(xmin)));
        }

        template<typename InputType, typename OutputType, typename OpType>
        inline static OutputType scaleValue(InputType value,
                                            InputType xmin,
                                            InputType xmax,
                                            OutputType ymin,
                                            OutputType ymax)
        {
            return OutputType(((OpType(value) - OpType(xmin))
                               * (OpType(ymax) - OpType(ymin))
                               + OpType(ymin)
                               * (OpType(xmax) - OpType(xmin)))
                               / (OpType(xmax) - OpType(xmin)));
        }

        template<typename T>
        inline static T from_(T value) {
            return value;
        }

        template<typename T>
        inline static T fromLE(T value) {
            return qFromLittleEndian(value);
        }

        template<typename T>
        inline static T fromBE(T value) {
            return qFromBigEndian(value);
        }

        template<typename T>
        inline static T to_(T value) {
            return value;
        }

        template<typename T>
        inline static T toLE(T value) {
            return qToLittleEndian(value);
        }

        template<typename T>
        inline static T toBE(T value) {
            return qToBigEndian(value);
        }

        template<typename InputType,
                 typename OutputType,
                 typename OpType,
                 typename TransformFuncType1,
                 typename TransformFuncType2>
        inline static AkAudioPacket convertSampleFormat(const AkAudioPacket &src,
                                                        AkAudioCaps::SampleFormat format,
                                                        TransformFuncType1 transformFrom,
                                                        TransformFuncType2 transformTo)
        {
            auto caps = src.caps();
            caps.setFormat(format);
            AkAudioPacket dst(caps, src.samples());
            dst.copyMetadata(src);
            auto n = caps.channels() - src.planes() + 1;

            for (int plane = 0; plane < src.planes(); ++plane) {
                auto src_line = reinterpret_cast<const InputType *>(src.constPlane(plane));
                auto dst_line = reinterpret_cast<OutputType *>(dst.plane(plane));

                for (int i = 0; i < n * src.samples(); ++i)
                    dst_line[i] =
                            transformTo(scaleValue<InputType,
                                                   OutputType,
                                                   OpType>(transformFrom(src_line[i])));
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
         [] (const AkAudioPacket &src) -> AkAudioPacket { \
            return convertSampleFormat<itype, otype, optype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sotype, \
                     from##inEndian<itype>, \
                     to##outEndian<otype>); \
         }}, \
        {AkAudioCaps::SampleFormat_##sotype, \
         AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &src) -> AkAudioPacket { \
             return convertSampleFormat<otype, itype, optype> \
                    (src, \
                     AkAudioCaps::SampleFormat_##sitype, \
                     from##outEndian<otype>, \
                     to##inEndian<itype>); \
         }}

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
                DEFINE_SAMPLE_CONVERT_FUNCTION(s8   , dbl,   qint8, qreal, qreal,  _, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u8   , dbl,  quint8, qreal, qreal,  _, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s16le, dbl,  qint16, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s16be, dbl,  qint16, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u16le, dbl, quint16, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u16be, dbl, quint16, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s32le, dbl,  qint32, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s32be, dbl,  qint32, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u32le, dbl, quint32, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u32be, dbl, quint32, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s64le, dbl,  qint64, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(s64be, dbl,  qint64, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u64le, dbl, quint64, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(u64be, dbl, quint64, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(fltle, dbl,   float, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(fltbe, dbl,   float, qreal, qreal, BE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(dblle, dbl,   qreal, qreal, qreal, LE, _),
                DEFINE_SAMPLE_CONVERT_FUNCTION(dblbe, dbl,   qreal, qreal, qreal, BE, _),
            };

            return convert;
        }

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static AkAudioPacket mixChannels(AkAudioCaps::SampleFormat sumFormat,
                                                AkAudioCaps::ChannelLayout outputLayout,
                                                const AkAudioPacket &src,
                                                TransformFuncType transformFrom,
                                                TransformFuncType transformTo)
        {
            // Create a summatory packet which type is big enough to contain
            // the sum of all values.
            auto caps = src.caps();
            caps.setFormat(sumFormat);
            caps.setLayout(outputLayout);
            AkAudioPacket sumPacket(caps, src.samples(), true);

            // Create output packet.
            caps = src.caps();
            caps.setLayout(outputLayout);
            AkAudioPacket dst(caps, src.samples());
            dst.copyMetadata(src);

            // Precalculate positional factors
            QVector<qreal> factors;

            for (int ochannel = 0; ochannel < sumPacket.caps().channels(); ++ochannel) {
                auto oposition = sumPacket.caps().position(ochannel);

                for (int ichannel = 0; ichannel < src.caps().channels(); ++ichannel) {
                    auto iposition = src.caps().position(ichannel);
                    factors << AkAudioCaps::distanceFactor(iposition, oposition);
                }
            }

            // Wave limits
            auto xmin = std::numeric_limits<SumType>::max();
            auto xmax = std::numeric_limits<SumType>::min();
            auto ymin = std::numeric_limits<SampleType>::max();
            auto ymax = std::numeric_limits<SampleType>::min();

            /* We use inverse square law to sum the samples
             * according to the speaker position in the sound dome.
             *
             * http://digitalsoundandmusic.com/4-3-4-the-mathematics-of-the-inverse-square-law-and-pag-equations/
             */
            if (src.caps().planar()) {
                for (int ochannel = 0; ochannel < dst.caps().channels(); ++ochannel) {
                    auto sum_line = reinterpret_cast<SumType *>(sumPacket.plane(ochannel));

                    for (int ichannel = 0; ichannel < src.caps().channels(); ++ichannel) {
                        auto k = factors[ichannel + ochannel * src.caps().channels()];
                        auto src_line = reinterpret_cast<const SampleType *>(src.constPlane(ichannel));

                        for (int sample = 0; sample < dst.samples(); ++sample) {
                            auto sampleValue = transformFrom(src_line[sample]);
                            sum_line[sample] += SumType(k * qreal(sampleValue));

                            // Calculate minimum and maximum values of the wave.

                            if (ichannel == src.caps().channels() - 1) {
                                xmin = qMin(xmin, sum_line[sample]);
                                xmax = qMax(xmax, sum_line[sample]);
                            }

                            if (ochannel == 0) {
                                ymin = qMin(ymin, sampleValue);
                                ymax = qMax(ymax, sampleValue);
                            }
                        }
                    }
                }

                // Recreate frame with the wave scaled to fit it.
                for (int ochannel = 0; ochannel < dst.caps().channels(); ++ochannel) {
                    auto dst_line = reinterpret_cast<SampleType *>(dst.plane(ochannel));
                    auto sum_line = reinterpret_cast<SumType *>(sumPacket.plane(ochannel));

                    for (int sample = 0; sample < dst.samples(); ++sample) {
                        dst_line[sample] = transformTo(scaleValue<SumType,
                                                                  SampleType,
                                                                  SumType>(sum_line[sample],
                                                                           xmin,
                                                                           xmax,
                                                                           ymin,
                                                                           ymax));
                    }
                }
            } else {
                auto src_line = reinterpret_cast<const SampleType *>(src.constPlane(0));
                auto dst_line = reinterpret_cast<SampleType *>(dst.plane(0));
                auto sum_line = reinterpret_cast<SumType *>(sumPacket.plane(0));

                for (int sample = 0; sample < dst.samples(); ++sample) {
                    auto iSampleOffset = sample * src.caps().channels();
                    auto oSampleOffset = sample * dst.caps().channels();

                    for (int ochannel = 0; ochannel < dst.caps().channels(); ++ochannel) {
                        auto sumOffset = oSampleOffset + ochannel;

                        for (int ichannel = 0; ichannel < src.caps().channels(); ++ichannel) {
                            auto k = factors[ichannel + ochannel * src.caps().channels()];
                            auto sampleValue = transformFrom(src_line[iSampleOffset + ichannel]);
                            sum_line[sumOffset] += SumType(k * qreal(sampleValue));

                            // Calculate minimum and maximum values of the wave.

                            if (ichannel == src.caps().channels() - 1) {
                                xmin = qMin(xmin, sum_line[sumOffset]);
                                xmax = qMax(xmax, sum_line[sumOffset]);
                            }

                            if (ochannel == 0) {
                                ymin = qMin(ymin, sampleValue);
                                ymax = qMax(ymax, sampleValue);
                            }
                        }
                    }
                }

                // Recreate frame with the wave scaled to fit it.
                for (int sample = 0; sample < dst.samples(); ++sample) {
                    auto oSampleOffset = sample * dst.caps().channels();

                    for (int ochannel = 0; ochannel < dst.caps().channels(); ++ochannel) {
                        auto sumOffset = oSampleOffset + ochannel;
                        dst_line[sumOffset] = transformTo(scaleValue<SumType,
                                                                     SampleType,
                                                                     SumType>(sum_line[sumOffset],
                                                                              xmin,
                                                                              xmax,
                                                                              ymin,
                                                                              ymax));
                    }
                }
            }

            return dst;
        }

#define HANDLE_CASE_CONVERT_LAYOUT(olayout, \
                                   src, \
                                   format, \
                                   sumFormat, \
                                   sampleType, \
                                   sumType, \
                                   endian) \
        case AkAudioCaps::SampleFormat_##format: \
            return mixChannels<sampleType, sumType> \
                    (AkAudioCaps::SampleFormat_##sumFormat, \
                     olayout, \
                     src, \
                     from##endian<sampleType>, \
                     to##endian<sampleType>);

        inline static AkAudioPacket convertChannels(AkAudioCaps::ChannelLayout outputLayout,
                                                    const AkAudioPacket &src)
        {
            switch (src.caps().format()) {
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s8   , dbl, qint8  , qreal,  _)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u8   , dbl, quint8 , qreal,  _)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s16le, dbl, qint16 , qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s16be, dbl, qint16 , qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u16le, dbl, quint16, qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u16be, dbl, quint16, qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s32le, dbl, qint32 , qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s32be, dbl, qint32 , qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u32le, dbl, quint32, qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u32be, dbl, quint32, qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s64le, dbl, qint64 , qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, s64be, dbl, qint64 , qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u64le, dbl, quint64, qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, u64be, dbl, quint64, qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, fltle, dbl, float  , qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, fltbe, dbl, float  , qreal, BE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, dblle, dbl, qreal  , qreal, LE)
            HANDLE_CASE_CONVERT_LAYOUT(outputLayout, src, dblbe, dbl, qreal  , qreal, BE)
            default:
                return {};
            }
        }

        template<typename SampleType>
        inline static AkAudioPacket convertChannelModel(const AkAudioPacket &packet,
                                                        bool planar)
        {
            auto caps = packet.caps();
            caps.setPlanar(planar);
            AkAudioPacket outPacket(caps, packet.samples());
            outPacket.copyMetadata(packet);

            if (planar) {
                auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(0));

                for (int channel = 0; channel < caps.channels(); ++channel) {
                    auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(channel));

                    for (int sample = 0; sample < packet.samples(); ++sample)
                        dst_line[sample] = src_line[sample * caps.channels() + channel];
                }
            } else {
                auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(0));

                for (int channel = 0; channel < packet.caps().channels(); ++channel) {
                    auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(channel));

                    for (int sample = 0; sample < packet.samples(); ++sample)
                        dst_line[sample * caps.channels() + channel] = src_line[sample];
                }
            }

            return outPacket;
        }

        using ConvertChannelModelFunction =
            std::function<AkAudioPacket (const AkAudioPacket &packet,
                                         bool planar)>;

#define DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(sitype, itype) \
        {AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &packet, bool planar) -> AkAudioPacket { \
            return convertChannelModel<itype>(packet, planar); \
         }}

        struct AudioSamplesConvertChannelModel
        {
            AkAudioCaps::SampleFormat format;
            ConvertChannelModelFunction convert;
        };

        using AudioSamplesConvertChannelModelFuncs = QVector<AudioSamplesConvertChannelModel>;

        inline static const AudioSamplesConvertChannelModelFuncs &samplesConvertChannelModel()
        {
            static const AudioSamplesConvertChannelModelFuncs convert {
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s8   ,   qint8),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u8   ,  quint8),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s16le,  qint16),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s16be,  qint16),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u16le, quint16),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u16be, quint16),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s32le,  qint32),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s32be,  qint32),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u32le, quint32),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u32be, quint32),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s64le,  qint64),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(s64be,  qint64),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u64le, quint64),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(u64be, quint64),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(fltle,   float),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(fltbe,   float),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(dblle,   qreal),
                DEFINE_CONVERT_CHANNEL_MODEL_FUNCTION(dblbe,   qreal),
            };

            return convert;
        }

        inline static const AudioSamplesConvertChannelModel *byChannelModelFormat(AkAudioCaps::SampleFormat format)
        {
            for (auto &model: samplesConvertChannelModel())
                if (model.format == format)
                    return &model;

            return &samplesConvertChannelModel().front();
        }

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static SampleType interpolate(qreal isample,
                                             SampleType y0,
                                             SampleType y1,
                                             TransformFuncType transformFrom,
                                             TransformFuncType transformTo)
        {
            y0 = transformFrom(y0);
            y1 = transformFrom(y1);
            auto y = isample * (SumType(y1) - SumType(y0)) + qreal(y0);

            return transformTo(SampleType(y));
        }

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static SampleType interpolate(qreal isample,
                                             SampleType y0,
                                             SampleType y1,
                                             SampleType y2,
                                             TransformFuncType transformFrom,
                                             TransformFuncType transformTo)
        {
            y0 = transformFrom(y0);
            y1 = transformFrom(y1);
            y2 = transformFrom(y2);
            auto yy0 =      SumType(y0) - 2 * SumType(y1) + SumType(y2);
            auto yy1 = -3 * SumType(y0) + 4 * SumType(y1) - SumType(y2);
            auto yy2 =  2 * SumType(y0);
            auto y = (yy0 * isample * isample + yy1 * isample + yy2) / 2;

            SampleType ymin;
            SampleType ymax;

            if (typeid(SampleType) == typeid(float)) {
                ymin = SampleType(-1.0f);
                ymax = SampleType(1.0f);
            } else if (typeid(SampleType) == typeid(qreal)) {
                ymin = SampleType(-1.0);
                ymax = SampleType(1.0);
            } else {
                ymin = std::numeric_limits<SampleType>::min();
                ymax = std::numeric_limits<SampleType>::max();
            }

            y = qBound<SumType>(ymin, y, ymax);

            return transformTo(SampleType(y));
        }

        template<typename SampleType>
        inline static AkAudioPacket scaleSamplesFast(const AkAudioPacket &packet,
                                                     int samples)
        {
            auto iSamples = packet.samples();
            AkAudioPacket outPacket(packet.caps(), samples);
            outPacket.copyMetadata(packet);
            QVector<int> sampleValues;

            for (int sample = 0; sample < outPacket.samples(); ++sample)
                sampleValues << sample * (iSamples - 1) / (samples - 1);

            if (packet.caps().planar()) {
                for (int channel = 0; channel < outPacket.caps().channels(); ++channel) {
                    auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(channel));
                    auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(channel));

                    for (int sample = 0; sample < outPacket.samples(); ++sample)
                        dst_line[sample] = src_line[sampleValues[sample]];
                }
            } else {
                auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(0));
                auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(0));
                auto oChannels = outPacket.caps().channels();

                for (int sample = 0; sample < outPacket.samples(); ++sample) {
                    auto iSampleOffset = sampleValues[sample] * oChannels;
                    auto oSampleOffset = sample * oChannels;

                    for (int channel = 0; channel < oChannels; ++channel) {
                        dst_line[oSampleOffset + channel] =
                                src_line[iSampleOffset + channel];
                    }
                }
            }

            return outPacket;
        }

        struct ValuesMinMax
        {
                qreal x {0.0};
                int min {0};
                int mid {0};
                int max {0};

                ValuesMinMax(qreal x, int min, int mid, int max):
                    x(x),
                    min(min),
                    mid(mid),
                    max(max)
                {
                }

                ValuesMinMax(qreal x, int min, int max):
                    x(x),
                    min(min),
                    max(max)
                {
                }
        };

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static AkAudioPacket scaleSamplesLinear(const AkAudioPacket &packet,
                                                       int samples,
                                                       TransformFuncType transformFrom,
                                                       TransformFuncType transformTo)
        {
            auto iSamples = packet.samples();
            AkAudioPacket outPacket(packet.caps(), samples);
            outPacket.copyMetadata(packet);
            QVector<ValuesMinMax> sampleValues;

            for (int sample = 0; sample < outPacket.samples(); ++sample) {
                auto iSample = qreal(sample) * (iSamples - 1) / (samples - 1);
                auto minSample = qFloor(iSample);
                auto maxSample = qCeil(iSample);
                sampleValues << ValuesMinMax {iSample - minSample,
                                              minSample,
                                              maxSample};
            }

            if (packet.caps().planar()) {
                for (int channel = 0; channel < outPacket.caps().channels(); ++channel) {
                    auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(channel));
                    auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(channel));

                    for (int sample = 0; sample < outPacket.samples(); ++sample) {
                        auto minSample = sampleValues[sample].min;
                        auto maxSample = sampleValues[sample].max;
                        dst_line[sample] =
                                interpolate<SampleType,
                                            SumType,
                                            TransformFuncType>(sampleValues[sample].x,
                                                               src_line[minSample],
                                                               src_line[maxSample],
                                                               transformFrom,
                                                               transformTo);
                    }
                }
            } else {
                auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(0));
                auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(0));
                auto oChannels = outPacket.caps().channels();

                for (int sample = 0; sample < outPacket.samples(); ++sample) {
                    auto iSampleMinOffset = sampleValues[sample].min * oChannels;
                    auto iSampleMaxOffset = sampleValues[sample].max * oChannels;
                    auto oSampleOffset = sample * oChannels;

                    for (int channel = 0; channel < oChannels; ++channel) {
                        dst_line[oSampleOffset + channel] =
                                interpolate<SampleType,
                                            SumType,
                                            TransformFuncType>(sampleValues[sample].x,
                                                               src_line[iSampleMinOffset + channel],
                                                               src_line[iSampleMaxOffset + channel],
                                                               transformFrom,
                                                               transformTo);
                    }
                }
            }

            return outPacket;
        }

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static AkAudioPacket scaleSamplesQuadratic(const AkAudioPacket &packet,
                                                          int samples,
                                                          TransformFuncType transformFrom,
                                                          TransformFuncType transformTo)
        {
            auto iSamples = int(packet.samples());
            AkAudioPacket outPacket(packet.caps(), samples);
            outPacket.copyMetadata(packet);
            QVector<ValuesMinMax> sampleValues;

            for (int sample = 0; sample < outPacket.samples(); ++sample) {
                auto iSample = qreal(sample) * (iSamples - 1) / (samples - 1);
                auto midSample = qRound(iSample);
                auto minSample = qMax(midSample - 1, 0);
                auto maxSample = qMin(midSample + 1, iSamples - 1);
                sampleValues << ValuesMinMax {iSample - minSample,
                                              minSample,
                                              midSample,
                                              maxSample};
            }

            if (packet.caps().planar()) {
                for (int channel = 0; channel < outPacket.caps().channels(); ++channel) {
                    auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(channel));
                    auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(channel));

                    for (int sample = 0; sample < outPacket.samples(); ++sample) {
                        auto minSample = sampleValues[sample].min;
                        auto midSample = sampleValues[sample].mid;
                        auto maxSample = sampleValues[sample].max;
                        dst_line[sample] =
                                interpolate<SampleType,
                                            SumType,
                                            TransformFuncType>(sampleValues[sample].x,
                                                               src_line[minSample],
                                                               src_line[midSample],
                                                               src_line[maxSample],
                                                               transformFrom,
                                                               transformTo);
                    }
                }
            } else {
                auto src_line = reinterpret_cast<const SampleType *>(packet.constPlane(0));
                auto dst_line = reinterpret_cast<SampleType *>(outPacket.plane(0));
                auto oChannels = outPacket.caps().channels();

                for (int sample = 0; sample < outPacket.samples(); ++sample) {
                    auto iSampleMinOffset = sampleValues[sample].min * oChannels;
                    auto iSampleMidOffset = sampleValues[sample].mid * oChannels;
                    auto iSampleMaxOffset = sampleValues[sample].max * oChannels;
                    auto oSampleOffset = sample * oChannels;

                    for (int channel = 0; channel < oChannels; ++channel) {
                        dst_line[oSampleOffset + channel] =
                                interpolate<SampleType,
                                            SumType,
                                            TransformFuncType>(sampleValues[sample].x,
                                                               src_line[iSampleMinOffset + channel],
                                                               src_line[iSampleMidOffset + channel],
                                                               src_line[iSampleMaxOffset + channel],
                                                               transformFrom,
                                                               transformTo);

                    }
                }
            }

            return outPacket;
        }

        using ScalingFunction =
            std::function<AkAudioPacket (const AkAudioPacket &packet,
                                         int samples)>;

#define DEFINE_SAMPLE_SCALING_FUNCTION(sitype, \
                                       itype, \
                                       optype, \
                                       endian) \
        {AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &packet, int samples) -> AkAudioPacket { \
            return scaleSamplesFast<itype>(packet, samples); \
         }, \
         [] (const AkAudioPacket &packet, int samples) -> AkAudioPacket { \
            return scaleSamplesLinear<itype, optype> \
                    (packet, \
                     samples, \
                     from##endian<itype>, \
                     to##endian<itype>); \
         }, \
         [] (const AkAudioPacket &packet, int samples) -> AkAudioPacket { \
            return scaleSamplesQuadratic<itype, optype> \
                    (packet, \
                     samples, \
                     from##endian<itype>, \
                     to##endian<itype>); \
         }}

        struct AudioSamplesScale
        {
            AkAudioCaps::SampleFormat format;
            ScalingFunction fast;
            ScalingFunction linear;
            ScalingFunction quadratic;
        };

        using AudioSamplesScaleFuncs = QVector<AudioSamplesScale>;

        inline static const AudioSamplesScaleFuncs &samplesScaling()
        {
            static const AudioSamplesScaleFuncs scaling {
                DEFINE_SAMPLE_SCALING_FUNCTION(s8   ,   qint8,  qreal,  _),
                DEFINE_SAMPLE_SCALING_FUNCTION(u8   ,  quint8,  qreal,  _),
                DEFINE_SAMPLE_SCALING_FUNCTION(s16le,  qint16,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(s16be,  qint16,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u16le, quint16,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u16be, quint16,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(s32le,  qint32,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(s32be,  qint32,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u32le, quint32,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u32be, quint32,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(s64le,  qint64,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(s64be,  qint64,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u64le, quint64,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(u64be, quint64,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(fltle,   float,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(fltbe,   float,  qreal, BE),
                DEFINE_SAMPLE_SCALING_FUNCTION(dblle,   qreal,  qreal, LE),
                DEFINE_SAMPLE_SCALING_FUNCTION(dblbe,   qreal,  qreal, BE),
            };

            return scaling;
        }

        inline static const AudioSamplesScale *bySamplesScalingFormat(AkAudioCaps::SampleFormat format)
        {
            for (auto &scaling: samplesScaling())
                if (scaling.format == format)
                    return &scaling;

            return &samplesScaling().front();
        }

        AkAudioPacket convertFormat(const AkAudioPacket &packet);
        AkAudioPacket convertLayout(const AkAudioPacket &packet);
        AkAudioPacket convertPlanar(const AkAudioPacket &packet);
        AkAudioPacket convertSampleRate(const AkAudioPacket &packet);
};

AkAudioConverter::AkAudioConverter(const AkAudioCaps &outputCaps, QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioConverterPrivate();
    this->d->m_outputCaps = outputCaps;
}

AkAudioConverter::AkAudioConverter(const AkAudioConverter &other):
    QObject()
{
    this->d = new AkAudioConverterPrivate();
    this->d->m_outputCaps = other.d->m_outputCaps;
    this->d->m_previousCaps = other.d->m_previousCaps;
    this->d->m_sampleCorrection = other.d->m_sampleCorrection;
}

AkAudioConverter::~AkAudioConverter()
{
    delete this->d;
}

AkAudioConverter &AkAudioConverter::operator =(const AkAudioConverter &other)
{
    if (this != &other) {
        this->d->m_outputCaps = other.d->m_outputCaps;
        this->d->m_previousCaps = other.d->m_previousCaps;
        this->d->m_sampleCorrection = other.d->m_sampleCorrection;
    }

    return *this;
}

QObject *AkAudioConverter::create()
{
    return new AkAudioConverter();
}

AkAudioCaps AkAudioConverter::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkAudioConverter::ResampleMethod AkAudioConverter::resampleMethod() const
{
    return this->d->m_resampleMethod;
}

bool AkAudioConverter::canConvertFormat(AkAudioCaps::SampleFormat input,
                                        AkAudioCaps::SampleFormat output)
{
    if (input == output)
        return true;

    bool fromFormat = false;
    bool toFormat = false;

    for (auto &convert: AkAudioConverterPrivate::sampleFormatConvert()) {
        if (convert.from == input)
            fromFormat = true;

        if (convert.to == output)
            toFormat = true;

        if (fromFormat && toFormat)
            return true;
    }

    return false;
}

AkAudioPacket AkAudioConverter::convert(const AkAudioPacket &packet)
{
    this->d->m_mutex.lock();
    auto outputCaps = this->d->m_outputCaps;
    this->d->m_mutex.unlock();

    if (!outputCaps)
        return packet;

    if (packet.size() < 1)
        return {};

    this->d->m_mutex.lock();

    if (packet.caps() != this->d->m_previousCaps) {
        this->d->m_previousCaps = packet.caps();
        this->d->m_sampleCorrection = 0;
    }

    this->d->m_mutex.unlock();
    auto outPacket = this->d->convertFormat(packet);

    if (!outPacket)
        return {};

    outPacket = this->d->convertLayout(outPacket);

    if (!outPacket)
        return {};

    outPacket = this->d->convertPlanar(outPacket);

    if (!outPacket)
        return {};

    return this->d->convertSampleRate(outPacket);
}

AkAudioPacket AkAudioConverter::scale(const AkAudioPacket &packet,
                                      int samples) const
{
    auto iSamples = packet.samples();

    if (iSamples == samples)
        return packet;

    if (samples < 1)
        return {};

    auto ssf =
            AkAudioConverterPrivate::bySamplesScalingFormat(packet.caps().format());
    auto method = this->d->m_resampleMethod;

    if (samples < iSamples)
        method = AkAudioConverter::ResampleMethod_Fast;

    switch (method) {
    case AkAudioConverter::ResampleMethod_Fast:
        return ssf->fast(packet, samples);

    case AkAudioConverter::ResampleMethod_Linear:
        return ssf->linear(packet, samples);

    case AkAudioConverter::ResampleMethod_Quadratic:
        return ssf->quadratic(packet, samples);
    }

    return {};
}

void AkAudioConverter::setOutputCaps(const AkAudioCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_mutex.lock();
    this->d->m_outputCaps = outputCaps;
    this->d->m_mutex.unlock();
    emit this->outputCapsChanged(outputCaps);
}

void AkAudioConverter::setResampleMethod(ResampleMethod resampleMethod)
{
    if (this->d->m_resampleMethod == resampleMethod)
        return;

    this->d->m_resampleMethod = resampleMethod;
    emit this->resampleMethodChanged(resampleMethod);
}

void AkAudioConverter::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkAudioConverter::resetResampleMethod()
{
    this->setResampleMethod(AkAudioConverter::ResampleMethod_Fast);
}

void AkAudioConverter::reset()
{
    this->d->m_mutex.lock();
    this->d->m_previousCaps = AkAudioCaps();
    this->d->m_sampleCorrection = 0;
    this->d->m_mutex.unlock();
}

void AkAudioConverter::registerTypes()
{
    qRegisterMetaType<AkAudioConverter>("AkAudioConverter");
    qmlRegisterSingletonType<AkAudioConverter>("Ak", 1, 0, "AkAudioConverter",
                                               [] (QQmlEngine *qmlEngine,
                                                   QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkAudioConverter();
    });
}

QDebug operator <<(QDebug debug, AkAudioConverter::ResampleMethod method)
{
    AkAudioConverter converter;
    int resampleMethodIndex = converter.metaObject()->indexOfEnumerator("ResampleMethod");
    QMetaEnum resampleMethodEnum = converter.metaObject()->enumerator(resampleMethodIndex);
    QString resampleMethodStr(resampleMethodEnum.valueToKey(method));
    resampleMethodStr.remove("ResampleMethod_");
    QDebugStateSaver saver(debug);
    debug.nospace() << resampleMethodStr.toStdString().c_str();

    return debug;
}

AkAudioPacket AkAudioConverterPrivate::convertFormat(const AkAudioPacket &packet)
{
    auto iFormat = packet.caps().format();
    this->m_mutex.lock();
    auto oFormat = this->m_outputCaps.format();
    this->m_mutex.unlock();

    if (iFormat == oFormat)
        return packet;

    AudioConvertFuntion convertFrom;
    AudioConvertFuntion convertTo;

    for (auto &convert: AkAudioConverterPrivate::sampleFormatConvert()) {
        if (convert.from == iFormat)
            convertFrom = convert.convert;

        if (convert.to == oFormat)
            convertTo = convert.convert;

        if (convert.from == iFormat && convert.to == oFormat)
            return convert.convert(packet);
    }

    if (convertFrom && convertTo)
        return convertTo(convertFrom(packet));

    return {};
}

AkAudioPacket AkAudioConverterPrivate::convertLayout(const AkAudioPacket &packet)
{
    this->m_mutex.lock();
    auto oLayout = this->m_outputCaps.layout();
    this->m_mutex.unlock();

    if (packet.caps().layout() == oLayout)
        return packet;

    return AkAudioConverterPrivate::convertChannels(oLayout, packet);
}

AkAudioPacket AkAudioConverterPrivate::convertPlanar(const AkAudioPacket &packet)
{
    this->m_mutex.lock();
    auto planar = this->m_outputCaps.planar();
    this->m_mutex.unlock();

    if (packet.caps().planar() == planar)
        return packet;

    auto cmf =
            AkAudioConverterPrivate::byChannelModelFormat(packet.caps().format());

    if (!cmf)
        return {};

    return cmf->convert(packet, planar);
}

AkAudioPacket AkAudioConverterPrivate::convertSampleRate(const AkAudioPacket &packet)
{
    auto iSamples = packet.samples();
    this->m_mutex.lock();
    auto oSampleRate = this->m_outputCaps.rate();
    this->m_mutex.unlock();

    if (packet.caps().rate() == oSampleRate)
        return packet;

    this->m_mutex.lock();
    auto sampleCorrection = this->m_sampleCorrection;
    this->m_mutex.unlock();

    auto rSamples = qreal(iSamples)
                    * oSampleRate
                    / packet.caps().rate()
                    + sampleCorrection;
    auto samples = qRound(rSamples);

    if (samples < 1)
        return {};

    if (iSamples == samples)
        return packet;

    auto ssf =
            AkAudioConverterPrivate::bySamplesScalingFormat(packet.caps().format());
    auto method = this->m_resampleMethod;

    if (samples < iSamples)
        method = AkAudioConverter::ResampleMethod_Fast;

    AkAudioPacket tmpPacket;

    switch (method) {
    case AkAudioConverter::ResampleMethod_Fast:
        tmpPacket = ssf->fast(packet, samples);
        break;

    case AkAudioConverter::ResampleMethod_Linear:
        tmpPacket = ssf->linear(packet, samples);
        break;

    case AkAudioConverter::ResampleMethod_Quadratic:
        tmpPacket = ssf->quadratic(packet, samples);
        break;
    }

    auto caps = packet.caps();
    caps.setRate(oSampleRate);
    AkAudioPacket outPacket(caps, samples);
    outPacket.copyMetadata(tmpPacket);
    outPacket.setPts(packet.pts() * oSampleRate / packet.caps().rate());
    outPacket.setTimeBase(packet.timeBase() * AkFrac(packet.caps().rate(),
                                                     oSampleRate));

    for (int plane = 0; plane < outPacket.planes(); plane++)
        memcpy(outPacket.plane(plane),
               tmpPacket.constPlane(plane),
               qMin(outPacket.planeSize(plane), tmpPacket.planeSize(plane)));

    this->m_mutex.lock();
    this->m_sampleCorrection = rSamples - samples;
    this->m_mutex.unlock();

    return outPacket;
}

#include "moc_akaudioconverter.cpp"
