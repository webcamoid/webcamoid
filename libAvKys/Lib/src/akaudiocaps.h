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

#ifndef AKAUDIOCAPS_H
#define AKAUDIOCAPS_H

#include <QObject>

#include "akcommons.h"

class AkAudioCapsPrivate;
class AkCaps;
class QDataStream;

class AKCOMMONS_EXPORT AkAudioCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(SampleFormat)
    Q_ENUMS(SampleType)
    Q_ENUMS(Position)
    Q_ENUMS(ChannelLayout)
    Q_PROPERTY(bool isValid
               READ isValid)
    Q_PROPERTY(SampleFormat format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
    Q_PROPERTY(int bps
               READ bps
               WRITE setBps
               RESET resetBps
               NOTIFY bpsChanged)
    Q_PROPERTY(int channels
               READ channels
               WRITE setChannels
               RESET resetChannels
               NOTIFY channelsChanged)
    Q_PROPERTY(int rate
               READ rate
               WRITE setRate
               RESET resetRate
               NOTIFY rateChanged)
    Q_PROPERTY(ChannelLayout layout
               READ layout
               WRITE setLayout
               RESET resetLayout
               NOTIFY layoutChanged)
    Q_PROPERTY(int samples
               READ samples
               WRITE setSamples
               RESET resetSamples
               NOTIFY samplesChanged)
    Q_PROPERTY(int align
               READ align
               WRITE setAlign
               RESET resetAlign
               NOTIFY alignChanged)

    public:
        enum SampleFormat
        {
            SampleFormat_none = -1,
            SampleFormat_s8,
            SampleFormat_u8,
            SampleFormat_s16,
            SampleFormat_s16le,
            SampleFormat_s16be,
            SampleFormat_u16,
            SampleFormat_u16le,
            SampleFormat_u16be,
            SampleFormat_s24,
            SampleFormat_s24le,
            SampleFormat_s24be,
            SampleFormat_u24,
            SampleFormat_u24le,
            SampleFormat_u24be,
            SampleFormat_s32,
            SampleFormat_s32le,
            SampleFormat_s32be,
            SampleFormat_u32,
            SampleFormat_u32le,
            SampleFormat_u32be,
            SampleFormat_s64,
            SampleFormat_s64le,
            SampleFormat_s64be,
            SampleFormat_u64,
            SampleFormat_u64le,
            SampleFormat_u64be,
            SampleFormat_flt,
            SampleFormat_fltle,
            SampleFormat_fltbe,
            SampleFormat_dbl,
            SampleFormat_dblle,
            SampleFormat_dblbe,
            SampleFormat_u8p,
            SampleFormat_s16p,
            SampleFormat_s32p,
            SampleFormat_s64p,
            SampleFormat_fltp,
            SampleFormat_dblp
        };

        enum SampleType
        {
            SampleType_unknown = -1,
            SampleType_int,
            SampleType_uint,
            SampleType_float
        };

        enum Position
        {
            Position_unknown             = 0x00000000,
            Position_FrontLeft           = 0x00000001,
            Position_FrontRight          = 0x00000002,
            Position_FrontCenter         = 0x00000004,
            Position_LowFrequency1       = 0x00000008,
            Position_BackLeft            = 0x00000010,
            Position_BackRight           = 0x00000020,
            Position_FrontLeftOfCenter   = 0x00000040,
            Position_FrontRightOfCenter  = 0x00000080,
            Position_BackCenter          = 0x00000100,
            Position_LowFrequency2       = 0x00000200,
            Position_SideLeft            = 0x00000400,
            Position_SideRight           = 0x00000800,
            Position_TopCenter           = 0x00001000,
            Position_TopFrontLeft        = 0x00002000,
            Position_TopFrontCenter      = 0x00004000,
            Position_TopFrontRight       = 0x00008000,
            Position_TopBackLeft         = 0x00010000,
            Position_TopBackCenter       = 0x00020000,
            Position_TopBackRight        = 0x00040000,
            Position_TopSideLeft         = 0x00080000,
            Position_TopSideRight        = 0x00100000,
            Position_BottomFrontCenter   = 0x00200000,
            Position_BottomFrontLeft     = 0x00400000,
            Position_BottomFrontRight    = 0x00800000,
            Position_StereoLeft          = 0x01000000,
            Position_StereoRight         = 0x02000000,
            Position_WideLeft            = 0x04000000,
            Position_WideRight           = 0x08000000,
            Position_SurroundDirectLeft  = 0x10000000,
            Position_SurroundDirectRight = 0x20000000
        };
        Q_DECLARE_FLAGS(Positions, Position)
        Q_FLAG(Positions)

        enum ChannelLayout
        {
            Layout_none          = 0,
            Layout_mono          = Position_FrontCenter,
            Layout_stereo        = Position_FrontLeft | Position_FrontRight,
            Layout_2p1           = Layout_stereo | Position_LowFrequency1,
            Layout_3p0           = Layout_stereo | Position_FrontCenter,
            Layout_3p0_back      = Layout_stereo | Position_BackCenter,
            Layout_3p1           = Layout_3p0 | Position_LowFrequency1,
            Layout_4p0           = Layout_3p0 | Position_BackCenter,
            Layout_quad          = Layout_stereo | Position_BackLeft | Position_BackRight,
            Layout_quad_side     = Layout_stereo | Position_SideLeft | Position_SideRight,
            Layout_4p1           = Layout_4p0 | Position_LowFrequency1,
            Layout_5p0           = Layout_3p0 | Position_BackLeft | Position_BackRight,
            Layout_5p0_side      = Layout_3p0 | Position_SideLeft | Position_SideRight,
            Layout_5p1           = Layout_5p0 | Position_LowFrequency1,
            Layout_5p1_side      = Layout_5p0_side | Position_LowFrequency1,
            Layout_6p0           = Layout_5p0_side | Position_BackCenter,
            Layout_6p0_front     = Layout_quad_side | Position_FrontLeftOfCenter | Position_FrontRightOfCenter,
            Layout_hexagonal     = Layout_5p0 | Position_BackCenter,
            Layout_6p1           = Layout_5p1_side | Position_BackCenter,
            Layout_6p1_back      = Layout_5p1 | Position_BackCenter,
            Layout_6p1_front     = Layout_6p0_front | Position_LowFrequency1,
            Layout_7p0           = Layout_5p0_side | Position_BackLeft | Position_BackRight,
            Layout_7p0_front     = Layout_5p0_side | Position_FrontLeftOfCenter | Position_FrontRightOfCenter,
            Layout_7p1           = Layout_5p1_side | Position_BackLeft | Position_BackRight,
            Layout_7p1_wide      = Layout_5p1_side | Position_FrontLeftOfCenter | Position_FrontRightOfCenter,
            Layout_7p1_wide_side = Layout_5p1 | Position_FrontLeftOfCenter | Position_FrontRightOfCenter,
            Layout_octagonal     = Layout_7p0 | Position_BackCenter,
            Layout_hexadecagonal = Layout_octagonal
                                 | Position_WideLeft | Position_WideRight
                                 | Position_BackLeft | Position_BackRight
                                 | Position_TopBackCenter | Position_TopFrontCenter
                                 | Position_TopFrontLeft | Position_TopFrontRight,
            Layout_downmix       = Position_StereoLeft | Position_StereoRight
        };

        explicit AkAudioCaps(QObject *parent=nullptr);
        AkAudioCaps(const QVariantMap &caps);
        AkAudioCaps(const QString &caps);
        AkAudioCaps(const AkCaps &caps);
        AkAudioCaps(const AkAudioCaps &other);
        AkAudioCaps(SampleFormat format, int channels, int rate);
         ~AkAudioCaps();
        AkAudioCaps &operator =(const AkAudioCaps &other);
        AkAudioCaps &operator =(const AkCaps &caps);
        AkAudioCaps &operator =(const QString &caps);
        bool operator ==(const AkAudioCaps &other) const;
        bool operator !=(const AkAudioCaps &other) const;
        operator bool() const;
        operator AkCaps() const;

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE bool &isValid();
        Q_INVOKABLE SampleFormat format() const;
        Q_INVOKABLE SampleFormat &format();
        Q_INVOKABLE int bps() const;
        Q_INVOKABLE int &bps();
        Q_INVOKABLE int channels() const;
        Q_INVOKABLE int &channels();
        Q_INVOKABLE int rate() const;
        Q_INVOKABLE int &rate();
        Q_INVOKABLE ChannelLayout layout() const;
        Q_INVOKABLE ChannelLayout &layout();
        Q_INVOKABLE int samples() const;
        Q_INVOKABLE int &samples();
        Q_INVOKABLE int align() const;
        Q_INVOKABLE int &align();

        Q_INVOKABLE AkAudioCaps &fromMap(const QVariantMap &caps);
        Q_INVOKABLE AkAudioCaps &fromString(const QString &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkAudioCaps &update(const AkCaps &caps);
        Q_INVOKABLE AkCaps toCaps() const;

        Q_INVOKABLE static int bitsPerSample(SampleFormat sampleFormat);
        Q_INVOKABLE static int bitsPerSample(const QString &sampleFormat);
        Q_INVOKABLE static QString sampleFormatToString(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromString(const QString &sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromProperties(AkAudioCaps::SampleType type,
                                                                   int bps,
                                                                   int endianness,
                                                                   bool planar);
        Q_INVOKABLE static bool sampleFormatProperties(SampleFormat sampleFormat,
                                                       AkAudioCaps::SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr,
                                                       bool *planar=nullptr);
        Q_INVOKABLE static bool sampleFormatProperties(const QString &sampleFormat,
                                                       AkAudioCaps::SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr,
                                                       bool *planar=nullptr);
        Q_INVOKABLE static SampleType sampleType(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleType sampleType(const QString &sampleFormat);
        Q_INVOKABLE static QString channelLayoutToString(ChannelLayout channelLayout);
        Q_INVOKABLE static ChannelLayout channelLayoutFromString(const QString &channelLayout);
        Q_INVOKABLE static int channelCount(ChannelLayout channelLayout);
        Q_INVOKABLE static int channelCount(const QString &channelLayout);
        Q_INVOKABLE static int endianness(SampleFormat sampleFormat);
        Q_INVOKABLE static int endianness(const QString &sampleFormat);
        Q_INVOKABLE static bool isPlanar(SampleFormat sampleFormat);
        Q_INVOKABLE static bool isPlanar(const QString &sampleFormat);
        Q_INVOKABLE static ChannelLayout defaultChannelLayout(int channelCount);
        Q_INVOKABLE static QString defaultChannelLayoutString(int channelCount);

    private:
        AkAudioCapsPrivate *d;

    Q_SIGNALS:
        void formatChanged(SampleFormat format);
        void bpsChanged(int bps);
        void channelsChanged(int channels);
        void rateChanged(int rate);
        void layoutChanged(ChannelLayout layout);
        void samplesChanged(int samples);
        void alignChanged(int align);

    public Q_SLOTS:
        void setFormat(SampleFormat format);
        void setBps(int bps);
        void setChannels(int channels);
        void setRate(int rate);
        void setLayout(ChannelLayout layout);
        void setSamples(int samples);
        void setAlign(int align);
        void resetFormat();
        void resetBps();
        void resetChannels();
        void resetRate();
        void resetLayout();
        void resetSamples();
        void resetAlign();

        friend QDebug operator <<(QDebug debug, const AkAudioCaps &caps);
        friend QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps);
        friend QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps);
};

QDebug operator <<(QDebug debug, const AkAudioCaps &caps);
QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps);
QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps);

Q_DECLARE_METATYPE(AkAudioCaps)
Q_DECLARE_METATYPE(AkAudioCaps::SampleFormat)
Q_DECLARE_METATYPE(AkAudioCaps::ChannelLayout)
Q_DECLARE_OPERATORS_FOR_FLAGS(AkAudioCaps::Positions)

#endif // AKAUDIOCAPS_H
