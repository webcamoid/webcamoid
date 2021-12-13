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

using AudioConvertFuntion =
    std::function<AkAudioPacket (const AkAudioPacket &src)>;

class AkAudioConverterPrivate
{
    public:
        QMutex m_mutex;
        AkAudioCaps m_outputCaps;
        AkAudioCaps m_previousCaps;
        AkAudioConverter::ResampleMethod m_resaampleMethod {AkAudioConverter::ResampleMethod_Fast};
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
            AkAudioPacket dst(caps);
            dst.copyMetadata(src);
            auto n = caps.channels() - caps.planes() + 1;

            for (int plane = 0; plane < caps.planes(); plane++) {
                auto src_line = reinterpret_cast<const InputType *>(src.constPlaneData(plane));
                auto dst_line = reinterpret_cast<OutputType *>(dst.planeData(plane));

                for (int i = 0; i < n * caps.samples(); i++)
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
            AkAudioPacket sumPacket(caps);
            sumPacket.buffer().fill(0);

            // Create output packet.
            caps = src.caps();
            caps.setLayout(outputLayout);
            AkAudioPacket dst(caps);
            dst.copyMetadata(src);

            // Precalculate positional factors
            QVector<qreal> factors;

            for (int ochannel = 0; ochannel < sumPacket.caps().channels(); ochannel++) {
                auto oposition = sumPacket.caps().position(ochannel);

                for (int ichannel = 0; ichannel < src.caps().channels(); ichannel++) {
                    auto iposition = src.caps().position(ichannel);
                    factors << AkAudioCaps::distanceFactor(iposition, oposition);
                }
            }

            for (int ochannel = 0; ochannel < sumPacket.caps().channels(); ochannel++) {
                // Wave limits
                auto xmin = std::numeric_limits<SumType>::max();
                auto xmax = std::numeric_limits<SumType>::min();
                auto ymin = std::numeric_limits<SampleType>::max();
                auto ymax = std::numeric_limits<SampleType>::min();

                for (int ichannel = 0; ichannel < src.caps().channels(); ichannel++) {
                    /* We use inverse square law to sum the samples
                     * according to the speaker position in the sound dome.
                     *
                     * http://digitalsoundandmusic.com/4-3-4-the-mathematics-of-the-inverse-square-law-and-pag-equations/
                     */
                    auto k = factors[ichannel + ochannel * src.caps().channels()];

                    for (int sample = 0; sample < sumPacket.caps().samples(); sample++) {
                        auto inSample =
                                reinterpret_cast<const SampleType *>(src.constSample(ichannel,
                                                                                     sample));
                        auto outSample =
                                reinterpret_cast<SumType *>(sumPacket.sample(ochannel,
                                                                             sample));
                        auto isample = SumType(k * qreal(transformFrom(*inSample)));
                        *outSample += isample;

                        // Calculate minimum and maximum values of the wave.
                        if (ichannel == src.caps().channels() - 1) {
                            xmin = qMin(xmin, *outSample);
                            xmax = qMax(xmax, *outSample);
                        }

                        ymin = qMin(ymin, *inSample);
                        ymax = qMax(ymax, *inSample);
                    }
                }

                // Recreate frame with the wave scaled to fit it.
                for (int sample = 0; sample < dst.caps().samples(); sample++) {
                    auto idata =
                            reinterpret_cast<const SumType *>(sumPacket.constSample(ochannel,
                                                                                    sample));
                    auto odata =
                            reinterpret_cast<SampleType *>(dst.sample(ochannel,
                                                                      sample));

                    *odata = transformTo(scaleValue<SumType,
                                                    SampleType,
                                                    SumType>(*idata,
                                                             xmin,
                                                             xmax,
                                                             ymin,
                                                             ymax));
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

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static SampleType interpolate(const AkAudioPacket &packet,
                                             int channel,
                                             qreal isample,
                                             int sample1,
                                             int sample2,
                                             TransformFuncType transformFrom,
                                             TransformFuncType transformTo)
        {
            auto minValue = *reinterpret_cast<const SampleType *>(packet.constSample(channel, sample1));
            auto maxValue = *reinterpret_cast<const SampleType *>(packet.constSample(channel, sample2));
            minValue = transformFrom(minValue);
            maxValue = transformFrom(maxValue);
            auto value = (SumType(isample - sample1) * SumType(maxValue - minValue)
                          + SumType(minValue) * SumType(sample2 - sample1))
                         / (sample2 - sample1);

            return transformTo(SampleType(value));
        }

        template<typename SampleType,
                 typename SumType,
                 typename TransformFuncType>
        inline static SampleType interpolate(const AkAudioPacket &packet,
                                             int channel,
                                             qreal isample,
                                             int sample1,
                                             int sample2,
                                             int sample3,
                                             TransformFuncType transformFrom,
                                             TransformFuncType transformTo)
        {
            auto minValue = *reinterpret_cast<const SampleType *>(packet.constSample(channel, sample1));
            auto midValue = *reinterpret_cast<const SampleType *>(packet.constSample(channel, sample2));
            auto maxValue = *reinterpret_cast<const SampleType *>(packet.constSample(channel, sample3));
            minValue = transformFrom(minValue);
            midValue = transformFrom(midValue);
            maxValue = transformFrom(maxValue);
            auto sample21 = SumType(sample1);
            auto sample22 = SumType(sample2);
            auto sample23 = SumType(sample3);
            /* y = a * x ^ 2 + b * x + c
             *
             * |a|   |x0^2 x0 1|^-1 |y0|;
             * |b| = |x1^2 x1 1|    |y1|;
             * |c|   |x2^2 x2 1|    |y2|;
             */
            auto det = sample21 * SumType(sample2 - sample3) - sample1 * SumType(sample22 - sample23) + SumType(sample22 * sample3 - sample23 * sample2)
                     - sample22 * SumType(sample2 - sample3) + sample2 * SumType(sample21 - sample23) - SumType(sample21 * sample3 - sample23 * sample1)
                     + sample23 * SumType(sample1 - sample2) - sample1 * SumType(sample21 - sample22) + SumType(sample21 * sample2 - sample22 * sample1);
            const SumType matrixValues[] {
                SumType(sample2 - sample3), sample23 - sample22, sample22 * sample3 - sample23 * sample2,
                SumType(sample3 - sample1), sample21 - sample23, sample23 * sample1 - sample21 * sample3,
                SumType(sample1 - sample2), sample22 - sample21, sample21 * sample2 - sample22 * sample1,
            };
            QGenericMatrix<3, 3, SumType> inv(matrixValues);
            const SumType yMatrixValues[] {
                SumType(minValue),
                SumType(midValue),
                SumType(maxValue),
            };
            QGenericMatrix<1, 3, SumType> valuesMatrix(yMatrixValues);
            auto coef = inv * valuesMatrix;
            auto value = (coef(0, 0) * isample * isample + coef(1, 0) * isample + coef(2, 0))
                       / det;

            return transformTo(SampleType(value));
        }

        using InterpolateLinearFunction =
            std::function<void (const AkAudioPacket &packet,
                                int channel,
                                qreal isample,
                                int sample1,
                                int sample2,
                                quint8 *osample)>;
        using InterpolateQuadraticFunction =
            std::function<void (const AkAudioPacket &packet,
                                int channel,
                                qreal isample,
                                int sample1,
                                int sample2,
                                int sample3,
                                quint8 *osample)>;

#define DEFINE_SAMPLE_INTERPOLATION_FUNCTION(sitype, \
                                             itype, \
                                             optype, \
                                             endian) \
        {AkAudioCaps::SampleFormat_##sitype, \
         [] (const AkAudioPacket &packet, \
             int channel, \
             int isample, \
             int sample1, \
             int sample2, \
             quint8 *osample) { \
            auto value = \
                interpolate<itype, optype> \
                    (packet, \
                     channel, \
                     isample, \
                     sample1, \
                     sample2, \
                     from##endian<itype>, \
                     to##endian<itype>); \
            memcpy(osample, &value, sizeof(itype)); \
         }, \
         [] (const AkAudioPacket &packet, \
             int channel, \
             int isample, \
             int sample1, \
             int sample2, \
             int sample3, \
             quint8 *osample) { \
            auto value = \
                interpolate<itype, optype> \
                    (packet, \
                     channel, \
                     isample, \
                     sample1, \
                     sample2, \
                     sample3, \
                     from##endian<itype>, \
                     to##endian<itype>); \
            memcpy(osample, &value, sizeof(itype)); \
         }}

        struct AudioSamplesInterpolation
        {
            AkAudioCaps::SampleFormat format;
            InterpolateLinearFunction linear;
            InterpolateQuadraticFunction quadratic;
        };

        using AudioSamplesInterpolationFuncs = QVector<AudioSamplesInterpolation>;

        inline static const AudioSamplesInterpolationFuncs &samplesInterpolation()
        {
            static const AudioSamplesInterpolationFuncs interpolation {
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s8   ,   qint8, qint64,  _),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u8   ,  quint8, qint64,  _),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s16le,  qint16, qint64, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s16be,  qint16, qint64, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u16le, quint16, qint64, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u16be, quint16, qint64, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s32le,  qint32, qint64, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s32be,  qint32, qint64, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u32le, quint32, qint64, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u32be, quint32, qint64, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s64le,  qint64,  qreal, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(s64be,  qint64,  qreal, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u64le, quint64,  qreal, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(u64be, quint64,  qreal, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(fltle,   float,  qreal, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(fltbe,   float,  qreal, BE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(dblle,   qreal,  qreal, LE),
                DEFINE_SAMPLE_INTERPOLATION_FUNCTION(dblbe,   qreal,  qreal, BE),
            };

            return interpolation;
        }

        inline static const AudioSamplesInterpolation *bySamplesInterpolationFormat(AkAudioCaps::SampleFormat format)
        {
            for (auto &interpolation: samplesInterpolation())
                if (interpolation.format == format)
                    return &interpolation;

            return &samplesInterpolation().front();
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

AkAudioConverter::AkAudioConverter(const AkAudioConverter &other)
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
    return this->d->m_resaampleMethod;
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

    if (packet.buffer().size() < 1)
        return {};

    this->d->m_mutex.lock();

    if (packet.caps() != this->d->m_previousCaps) {
        this->d->m_previousCaps = packet.caps();
        this->d->m_sampleCorrection = 0;
    }

    this->d->m_mutex.unlock();
    auto packet_ = this->d->convertFormat(packet);

    if (!packet_)
        return {};

    packet_ = this->d->convertLayout(packet_);

    if (!packet_)
        return {};

    packet_ = this->d->convertPlanar(packet_);

    if (!packet_)
        return {};

    return this->d->convertSampleRate(packet_);
}

AkAudioPacket AkAudioConverter::scale(const AkAudioPacket &packet,
                                      int samples) const
{
    auto iSamples = packet.caps().samples();

    if (iSamples == samples)
        return packet;

    if (samples < 1)
        return {};

    auto caps = packet.caps();
    caps.setSamples(samples);
    AkAudioPacket packet_(caps);
    auto method = this->d->m_resaampleMethod;

    if (samples <  iSamples)
        method = AkAudioConverter::ResampleMethod_Fast;

    switch (method) {
    case AkAudioConverter::ResampleMethod_Fast:
        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = sample
                               * (iSamples - 1)
                               / (samples - 1);
                auto iValue = packet.constSample(channel, iSample);
                packet_.setSample(channel, sample, iValue);
            }
        }

        break;

    case AkAudioConverter::ResampleMethod_Linear: {
        auto sif =
                AkAudioConverterPrivate::bySamplesInterpolationFormat(caps.format());
        auto interpolation = sif->linear;

        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = qreal(sample)
                               * (iSamples - 1)
                               / (samples - 1);
                auto minSample = qFloor(iSample);
                auto maxSample = qCeil(iSample);

                if (minSample == maxSample) {
                    auto iValue = packet.constSample(channel, minSample);
                    packet_.setSample(channel, sample, iValue);
                } else {
                    quint64 data;
                    interpolation(packet,
                                  channel,
                                  iSample,
                                  minSample,
                                  maxSample,
                                  reinterpret_cast<quint8 *>(&data));
                    packet_.setSample(channel,
                                      sample,
                                      reinterpret_cast<const quint8 *>(&data));
                }
            }
        }

        break;
    }

    case AkAudioConverter::ResampleMethod_Quadratic:
        auto sif =
                AkAudioConverterPrivate::bySamplesInterpolationFormat(caps.format());
        auto interpolationL = sif->linear;
        auto interpolationQ = sif->quadratic;

        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = qreal(sample)
                               * (iSamples - 1)
                               / (samples - 1);
                auto minSample = qFloor(iSample);
                auto maxSample = qCeil(iSample);

                if (minSample == maxSample) {
                    auto iValue = packet.constSample(channel, minSample);
                    packet_.setSample(channel, sample, iValue);
                } else {
                    auto diffMinSample = minSample - iSample;
                    auto diffMaxSample = maxSample - iSample;
                    diffMinSample *= diffMinSample;
                    diffMaxSample *= diffMaxSample;
                    auto midSample = diffMinSample < diffMaxSample?
                                         qMax(minSample - 1, 0):
                                         qMin(maxSample + 1, iSamples - 1);

                    if (midSample < minSample)
                        std::swap(midSample, minSample);

                    if (midSample > maxSample)
                        std::swap(midSample, maxSample);

                    if (midSample == minSample
                        || midSample == maxSample) {
                        quint64 data;
                        interpolationL(packet,
                                       channel,
                                       iSample,
                                       minSample,
                                       maxSample,
                                       reinterpret_cast<quint8 *>(&data));
                        packet_.setSample(channel,
                                          sample,
                                          reinterpret_cast<const quint8 *>(&data));
                    } else {
                        quint64 data;
                        interpolationQ(packet,
                                       channel,
                                       iSample,
                                       minSample,
                                       midSample,
                                       maxSample,
                                       reinterpret_cast<quint8 *>(&data));
                        packet_.setSample(channel,
                                          sample,
                                          reinterpret_cast<const quint8 *>(&data));
                    }
                }
            }
        }

        break;
    }

    return packet_;
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
    if (this->d->m_resaampleMethod == resampleMethod)
        return;

    this->d->m_resaampleMethod = resampleMethod;
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

    auto caps = packet.caps();
    caps.updatePlaneSize(planar);
    AkAudioPacket dst(caps);
    dst.copyMetadata(packet);
    auto byps = caps.bps() / 8;
    auto channels = caps.channels();

    if (planar) {
        auto src_line = packet.constPlaneData(0);

        for (int plane = 0; plane < caps.planes(); plane++) {
            auto dst_line = dst.planeData(plane);

            for (int i = 0; i < caps.samples(); i++)
                memcpy(dst_line + byps * i,
                       src_line + byps * (i * channels + plane),
                       size_t(byps));
        }
    } else {
        auto dst_line = dst.planeData(0);

        for (int plane = 0; plane < packet.caps().planes(); plane++) {
            auto src_line = packet.constPlaneData(plane);

            for (int i = 0; i < caps.samples(); i++)
                memcpy(dst_line + byps * (i * channels + plane),
                       src_line + byps * i,
                       size_t(byps));
        }
    }

    return dst;
}

AkAudioPacket AkAudioConverterPrivate::convertSampleRate(const AkAudioPacket &packet)
{
    auto iSamples = packet.caps().samples();
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
    auto oSamples = qRound(rSamples);

    if (oSamples < 1)
        return {};

    auto caps = packet.caps();
    caps.setSamples(oSamples);
    caps.setRate(oSampleRate);
    AkAudioPacket packet_(caps);
    auto method = this->m_resaampleMethod;

    if (oSamples <  iSamples)
        method = AkAudioConverter::ResampleMethod_Fast;

    switch (method) {
    case AkAudioConverter::ResampleMethod_Fast:
        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = sample
                               * (iSamples - 1)
                               / (oSamples - 1);
                auto iValue = packet.constSample(channel, iSample);
                packet_.setSample(channel, sample, iValue);
            }
        }

        break;

    case AkAudioConverter::ResampleMethod_Linear: {
        auto sif =
                AkAudioConverterPrivate::bySamplesInterpolationFormat(caps.format());
        auto interpolation = sif->linear;

        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = qreal(sample)
                               * (iSamples - 1)
                               / (oSamples - 1);
                auto minSample = qFloor(iSample);
                auto maxSample = qCeil(iSample);

                if (minSample == maxSample) {
                    auto iValue = packet.constSample(channel, minSample);
                    packet_.setSample(channel, sample, iValue);
                } else {
                    quint64 data;
                    interpolation(packet,
                                  channel,
                                  iSample,
                                  minSample,
                                  maxSample,
                                  reinterpret_cast<quint8 *>(&data));
                    packet_.setSample(channel,
                                      sample,
                                      reinterpret_cast<const quint8 *>(&data));
                }
            }
        }

        break;
    }

    case AkAudioConverter::ResampleMethod_Quadratic:
        auto sif =
                AkAudioConverterPrivate::bySamplesInterpolationFormat(caps.format());
        auto interpolationL = sif->linear;
        auto interpolationQ = sif->quadratic;

        for (int channel = 0; channel < packet_.caps().channels(); channel++) {
            for (int sample = 0; sample < packet_.caps().samples(); sample++) {
                auto iSample = qreal(sample)
                               * (iSamples - 1)
                               / (oSamples - 1);
                auto minSample = qFloor(iSample);
                auto maxSample = qCeil(iSample);

                if (minSample == maxSample) {
                    auto iValue = packet.constSample(channel, minSample);
                    packet_.setSample(channel, sample, iValue);
                } else {
                    auto diffMinSample = minSample - iSample;
                    auto diffMaxSample = maxSample - iSample;
                    diffMinSample *= diffMinSample;
                    diffMaxSample *= diffMaxSample;
                    auto midSample = diffMinSample < diffMaxSample?
                                         qMax(minSample - 1, 0):
                                         qMin(maxSample + 1, iSamples - 1);

                    if (midSample < minSample)
                        std::swap(midSample, minSample);

                    if (midSample > maxSample)
                        std::swap(midSample, maxSample);

                    if (midSample == minSample
                        || midSample == maxSample) {
                        quint64 data;
                        interpolationL(packet,
                                       channel,
                                       iSample,
                                       minSample,
                                       maxSample,
                                       reinterpret_cast<quint8 *>(&data));
                        packet_.setSample(channel,
                                          sample,
                                          reinterpret_cast<const quint8 *>(&data));
                    } else {
                        quint64 data;
                        interpolationQ(packet,
                                       channel,
                                       iSample,
                                       minSample,
                                       midSample,
                                       maxSample,
                                       reinterpret_cast<quint8 *>(&data));
                        packet_.setSample(channel,
                                          sample,
                                          reinterpret_cast<const quint8 *>(&data));
                    }
                }
            }
        }

        break;
    }

    this->m_mutex.lock();
    this->m_sampleCorrection = rSamples - oSamples;
    this->m_mutex.unlock();

    return packet_;
}

#include "moc_akaudioconverter.cpp"
