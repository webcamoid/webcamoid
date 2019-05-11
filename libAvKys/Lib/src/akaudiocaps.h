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
    Q_PROPERTY(ChannelLayout layout
               READ layout
               WRITE setLayout
               RESET resetLayout
               NOTIFY layoutChanged)
    Q_PROPERTY(int bps
               READ bps)
    Q_PROPERTY(int channels
               READ channels)
    Q_PROPERTY(int rate
               READ rate
               WRITE setRate
               RESET resetRate
               NOTIFY rateChanged)
    Q_PROPERTY(int samples
               READ samples
               WRITE setSamples
               RESET resetSamples
               NOTIFY samplesChanged)
    Q_PROPERTY(bool planar
               READ planar
               WRITE setPlanar
               RESET resetPlanar
               NOTIFY planarChanged)
    Q_PROPERTY(int align
               READ align
               WRITE setAlign
               RESET resetAlign
               NOTIFY alignChanged)
    Q_PROPERTY(size_t frameSize
               READ frameSize)
    Q_PROPERTY(int planes
               READ planes)
    Q_PROPERTY(size_t planeSize
               READ planeSize)

    public:
        enum SampleFormat
        {
            SampleFormat_none = -1,
            SampleFormat_s8,
            SampleFormat_u8,
            SampleFormat_s16le,
            SampleFormat_s16be,
            SampleFormat_u16le,
            SampleFormat_u16be,
            SampleFormat_s32le,
            SampleFormat_s32be,
            SampleFormat_u32le,
            SampleFormat_u32be,
            SampleFormat_s64le,
            SampleFormat_s64be,
            SampleFormat_u64le,
            SampleFormat_u64be,
            SampleFormat_fltle,
            SampleFormat_fltbe,
            SampleFormat_dblle,
            SampleFormat_dblbe,

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            SampleFormat_s16 = SampleFormat_s16le,
            SampleFormat_u16 = SampleFormat_s16le,
            SampleFormat_s32 = SampleFormat_s32le,
            SampleFormat_u32 = SampleFormat_u32le,
            SampleFormat_s64 = SampleFormat_s64le,
            SampleFormat_u64 = SampleFormat_u64le,
            SampleFormat_flt = SampleFormat_fltle,
            SampleFormat_dbl = SampleFormat_dblle,
#else
            SampleFormat_s16 = SampleFormat_s16be,
            SampleFormat_u16 = SampleFormat_s16be,
            SampleFormat_s32 = SampleFormat_s32be,
            SampleFormat_u32 = SampleFormat_u32be,
            SampleFormat_s64 = SampleFormat_s64be,
            SampleFormat_u64 = SampleFormat_u64be,
            SampleFormat_flt = SampleFormat_fltbe,
            SampleFormat_dbl = SampleFormat_dblbe,
#endif
        };

        enum SampleType
        {
            SampleType_unknown = -1,
            SampleType_int,
            SampleType_uint,
            SampleType_float,
        };

        enum Position
        {
            Position_unknown             = 0      ,
            Position_FrontLeft           = 1 << 0 ,
            Position_FrontRight          = 1 << 1 ,
            Position_FrontCenter         = 1 << 2 ,
            Position_LowFrequency1       = 1 << 3 ,
            Position_BackLeft            = 1 << 4 ,
            Position_BackRight           = 1 << 5 ,
            Position_FrontLeftOfCenter   = 1 << 6 ,
            Position_FrontRightOfCenter  = 1 << 7 ,
            Position_BackCenter          = 1 << 8 ,
            Position_LowFrequency2       = 1 << 9 ,
            Position_SideLeft            = 1 << 10,
            Position_SideRight           = 1 << 11,
            Position_TopCenter           = 1 << 12,
            Position_TopFrontLeft        = 1 << 13,
            Position_TopFrontCenter      = 1 << 14,
            Position_TopFrontRight       = 1 << 15,
            Position_TopBackLeft         = 1 << 16,
            Position_TopBackCenter       = 1 << 17,
            Position_TopBackRight        = 1 << 18,
            Position_TopSideLeft         = 1 << 19,
            Position_TopSideRight        = 1 << 20,
            Position_BottomFrontCenter   = 1 << 21,
            Position_BottomFrontLeft     = 1 << 22,
            Position_BottomFrontRight    = 1 << 23,
            Position_StereoLeft          = 1 << 24,
            Position_StereoRight         = 1 << 25,
            Position_WideLeft            = 1 << 26,
            Position_WideRight           = 1 << 27,
            Position_SurroundDirectLeft  = 1 << 28,
            Position_SurroundDirectRight = 1 << 29,
        };
        Q_DECLARE_FLAGS(Positions, Position)
        Q_FLAG(Positions)

        enum ChannelLayout
        {
            Layout_none          = Position_unknown,
            Layout_mono          = Position_FrontCenter,
            Layout_stereo        = Position_FrontLeft | Position_FrontRight,
            Layout_downmix       = Position_StereoLeft | Position_StereoRight,
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
            Layout_7p1_wide_back = Layout_5p1 | Position_FrontLeftOfCenter | Position_FrontRightOfCenter,
            Layout_octagonal     = Layout_7p0 | Position_BackCenter,
            Layout_hexadecagonal = Layout_octagonal
                                 | Position_WideLeft | Position_WideRight
                                 | Position_TopBackLeft | Position_TopBackRight
                                 | Position_TopBackCenter | Position_TopFrontCenter
                                 | Position_TopFrontLeft | Position_TopFrontRight,
        };

        AkAudioCaps(QObject *parent=nullptr);
        AkAudioCaps(SampleFormat format,
                    ChannelLayout layout,
                    int rate,
                    int samples=0,
                    bool planar=false,
                    int align=1);
        AkAudioCaps(const QVariantMap &caps);
        AkAudioCaps(const QString &caps);
        AkAudioCaps(const AkCaps &caps);
        AkAudioCaps(const AkAudioCaps &other);
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
        Q_INVOKABLE ChannelLayout layout() const;
        Q_INVOKABLE int bps() const;
        Q_INVOKABLE int channels() const;
        Q_INVOKABLE int rate() const;
        Q_INVOKABLE int &rate();
        Q_INVOKABLE int samples() const;
        Q_INVOKABLE bool planar() const;
        Q_INVOKABLE int align() const;
        Q_INVOKABLE size_t frameSize() const;

        Q_INVOKABLE AkAudioCaps &fromMap(const QVariantMap &caps);
        Q_INVOKABLE AkAudioCaps &fromString(const QString &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkAudioCaps &update(const AkCaps &caps);
        Q_INVOKABLE AkCaps toCaps() const;
        Q_INVOKABLE size_t planeOffset(int plane) const;
        Q_INVOKABLE int planes() const;
        Q_INVOKABLE size_t planeSize() const;

        Q_INVOKABLE static int bitsPerSample(SampleFormat sampleFormat);
        Q_INVOKABLE static int bitsPerSample(const QString &sampleFormat);
        Q_INVOKABLE static QString sampleFormatToString(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromString(const QString &sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromProperties(AkAudioCaps::SampleType type,
                                                                   int bps,
                                                                   int endianness);
        Q_INVOKABLE static bool sampleFormatProperties(SampleFormat sampleFormat,
                                                       AkAudioCaps::SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr);
        Q_INVOKABLE static bool sampleFormatProperties(const QString &sampleFormat,
                                                       AkAudioCaps::SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr);
        Q_INVOKABLE static SampleType sampleType(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleType sampleType(const QString &sampleFormat);
        Q_INVOKABLE static QString channelLayoutToString(ChannelLayout channelLayout);
        Q_INVOKABLE static ChannelLayout channelLayoutFromString(const QString &channelLayout);
        Q_INVOKABLE static int channelCount(ChannelLayout channelLayout);
        Q_INVOKABLE static int channelCount(const QString &channelLayout);
        Q_INVOKABLE static int endianness(SampleFormat sampleFormat);
        Q_INVOKABLE static int endianness(const QString &sampleFormat);
        Q_INVOKABLE static ChannelLayout defaultChannelLayout(int channelCount);
        Q_INVOKABLE static QString defaultChannelLayoutString(int channelCount);

    private:
        AkAudioCapsPrivate *d;

    Q_SIGNALS:
        void formatChanged(SampleFormat format);
        void layoutChanged(ChannelLayout layout);
        void rateChanged(int rate);
        void samplesChanged(int samples);
        void planarChanged(bool planar);
        void alignChanged(int align);

    public Q_SLOTS:
        void setFormat(SampleFormat format);
        void setLayout(ChannelLayout layout);
        void setRate(int rate);
        void setSamples(int samples);
        void setPlanar(bool planar);
        void setAlign(int align);
        void resetFormat();
        void resetLayout();
        void resetRate();
        void resetSamples();
        void resetPlanar();
        void resetAlign();
        void clear();

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
