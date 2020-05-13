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

class AKCOMMONS_EXPORT AkAudioCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(SampleFormat)
    Q_ENUMS(SampleType)
    Q_ENUMS(Position)
    Q_ENUMS(ChannelLayout)
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
    Q_PROPERTY(size_t frameSize
               READ frameSize)
    Q_PROPERTY(bool planar
               READ planar)
    Q_PROPERTY(int planes
               READ planes)
    Q_PROPERTY(QVector<size_t> planeSize
               READ planeSize
               WRITE setPlaneSize
               RESET resetPlaneSize
               NOTIFY planeSizeChanged)

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
            SampleFormat_u16 = SampleFormat_u16le,
            SampleFormat_s32 = SampleFormat_s32le,
            SampleFormat_u32 = SampleFormat_u32le,
            SampleFormat_s64 = SampleFormat_s64le,
            SampleFormat_u64 = SampleFormat_u64le,
            SampleFormat_flt = SampleFormat_fltle,
            SampleFormat_dbl = SampleFormat_dblle,
#else
            SampleFormat_s16 = SampleFormat_s16be,
            SampleFormat_u16 = SampleFormat_u16be,
            SampleFormat_s32 = SampleFormat_s32be,
            SampleFormat_u32 = SampleFormat_u32be,
            SampleFormat_s64 = SampleFormat_s64be,
            SampleFormat_u64 = SampleFormat_u64be,
            SampleFormat_flt = SampleFormat_fltbe,
            SampleFormat_dbl = SampleFormat_dblbe,
#endif
        };
        using SampleFormatList = QList<SampleFormat>;

        enum SampleType
        {
            SampleType_unknown = -1,
            SampleType_int,
            SampleType_uint,
            SampleType_float,
        };

        enum Position
        {
            Position_unknown = -1,
            Position_FrontCenter,
            Position_FrontLeft,
            Position_FrontRight,
            Position_BackCenter,
            Position_BackLeft,
            Position_BackRight,
            Position_FrontLeftOfCenter,
            Position_FrontRightOfCenter,
            Position_WideLeft,
            Position_WideRight,
            Position_SideLeft,
            Position_SideRight,
            Position_LowFrequency1,
            Position_LowFrequency2,
            Position_TopCenter,
            Position_TopFrontCenter,
            Position_TopFrontLeft,
            Position_TopFrontRight,
            Position_TopBackCenter,
            Position_TopBackLeft,
            Position_TopBackRight,
            Position_TopSideLeft,
            Position_TopSideRight,
            Position_BottomFrontCenter,
            Position_BottomFrontLeft,
            Position_BottomFrontRight,
            Position_StereoLeft,
            Position_StereoRight,
            Position_SurroundDirectLeft,
            Position_SurroundDirectRight,
        };
        using SpeakerPosition = QPair<qreal, qreal>;

        enum ChannelLayout
        {
            Layout_none = -1,
            Layout_mono,
            Layout_stereo,
            Layout_downmix,
            Layout_2p1,
            Layout_3p0,
            Layout_3p0_back,
            Layout_3p1,
            Layout_4p0,
            Layout_quad,
            Layout_quad_side,
            Layout_4p1,
            Layout_5p0,
            Layout_5p0_side,
            Layout_5p1,
            Layout_5p1_side,
            Layout_6p0,
            Layout_6p0_front,
            Layout_hexagonal,
            Layout_6p1,
            Layout_6p1_back,
            Layout_6p1_front,
            Layout_7p0,
            Layout_7p0_front,
            Layout_7p1,
            Layout_7p1_wide,
            Layout_7p1_wide_back,
            Layout_octagonal,
            Layout_hexadecagonal,
        };
        using ChannelLayoutList = QList<ChannelLayout>;

        AkAudioCaps(QObject *parent=nullptr);
        AkAudioCaps(SampleFormat format,
                    ChannelLayout layout,
                    int rate,
                    int samples=0,
                    bool planar=false,
                    int align=1);
        AkAudioCaps(SampleFormat format,
                    ChannelLayout layout,
                    int rate,
                    int samples,
                    const QVector<size_t> &planeSize);
        AkAudioCaps(const AkCaps &caps);
        AkAudioCaps(const AkAudioCaps &other);
         ~AkAudioCaps();
        AkAudioCaps &operator =(const AkAudioCaps &other);
        AkAudioCaps &operator =(const AkCaps &caps);
        bool operator ==(const AkAudioCaps &other) const;
        bool operator !=(const AkAudioCaps &other) const;
        operator bool() const;
        operator AkCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkAudioCaps &caps);
        Q_INVOKABLE static QObject *create(AkAudioCaps::SampleFormat format,
                                           AkAudioCaps::ChannelLayout layout,
                                           int rate,
                                           int samples=0,
                                           bool planar=false,
                                           int align=1);
        Q_INVOKABLE static QObject *create(const QString &format,
                                           const QString &layout,
                                           int rate,
                                           int samples=0,
                                           bool planar=false,
                                           int align=1);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE SampleFormat format() const;
        Q_INVOKABLE ChannelLayout layout() const;
        Q_INVOKABLE int bps() const;
        Q_INVOKABLE int channels() const;
        Q_INVOKABLE int rate() const;
        Q_INVOKABLE int &rate();
        Q_INVOKABLE int samples() const;
        Q_INVOKABLE size_t frameSize() const;
        Q_INVOKABLE const QVector<Position> positions() const;

        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE AkAudioCaps &update(const AkCaps &caps);
        Q_INVOKABLE size_t planeOffset(int plane) const;
        Q_INVOKABLE bool planar() const;
        Q_INVOKABLE int planes() const;
        Q_INVOKABLE QVector<size_t> planeSize() const;
        Q_INVOKABLE size_t bytesPerPlane() const;
        Q_INVOKABLE void realign(int align);
        Q_INVOKABLE void updatePlaneSize(bool planar, int align=1);

        Q_INVOKABLE static AkAudioCaps fromMap(const QVariantMap &caps);
        Q_INVOKABLE static int bitsPerSample(SampleFormat sampleFormat);
        Q_INVOKABLE static int bitsPerSample(const QString &sampleFormat);
        Q_INVOKABLE static QString sampleFormatToString(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromString(const QString &sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromProperties(SampleType type,
                                                                   int bps,
                                                                   int endianness);
        Q_INVOKABLE static bool sampleFormatProperties(SampleFormat sampleFormat,
                                                       SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr);
        Q_INVOKABLE static bool sampleFormatProperties(const QString &sampleFormat,
                                                       SampleType *type=nullptr,
                                                       int *bps=nullptr,
                                                       int *endianness=nullptr);
        Q_INVOKABLE static SampleType sampleType(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleType sampleType(const QString &sampleFormat);
        Q_INVOKABLE static QString channelLayoutToString(ChannelLayout channelLayout);
        Q_INVOKABLE static ChannelLayout channelLayoutFromString(const QString &channelLayout);
        Q_INVOKABLE static ChannelLayout channelLayoutFromPositions(const QVector<Position> &positions);
        Q_INVOKABLE static int channelCount(ChannelLayout channelLayout);
        Q_INVOKABLE static int channelCount(const QString &channelLayout);
        Q_INVOKABLE static int endianness(SampleFormat sampleFormat);
        Q_INVOKABLE static int endianness(const QString &sampleFormat);
        Q_INVOKABLE static ChannelLayout defaultChannelLayout(int channelCount);
        Q_INVOKABLE static QString defaultChannelLayoutString(int channelCount);
        Q_INVOKABLE static const QVector<Position> &positions(ChannelLayout channelLayout);
        Q_INVOKABLE static SpeakerPosition position(Position position);
        Q_INVOKABLE SpeakerPosition position(int channel) const;

    private:
        AkAudioCapsPrivate *d;

    Q_SIGNALS:
        void formatChanged(SampleFormat format);
        void layoutChanged(ChannelLayout layout);
        void rateChanged(int rate);
        void samplesChanged(int samples);
        void planeSizeChanged(const QVector<size_t> &planeSize);

    public Q_SLOTS:
        void setFormat(SampleFormat format);
        void setLayout(ChannelLayout layout);
        void setRate(int rate);
        void setSamples(int samples);
        void setPlaneSize(const QVector<size_t> &planeSize);
        void resetFormat();
        void resetLayout();
        void resetRate();
        void resetSamples();
        void resetPlaneSize();
        void clear();
        static void registerTypes();
};

AKCOMMONS_EXPORT qreal operator -(const AkAudioCaps::SpeakerPosition &pos1,
                                  const AkAudioCaps::SpeakerPosition &pos2);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkAudioCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioCaps::SampleFormat format);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioCaps::SampleType sampleType);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioCaps::Position position);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioCaps::ChannelLayout layout);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps);

Q_DECLARE_METATYPE(AkAudioCaps)
Q_DECLARE_METATYPE(AkAudioCaps::SampleFormat)
Q_DECLARE_METATYPE(AkAudioCaps::SampleType)
Q_DECLARE_METATYPE(AkAudioCaps::Position)
Q_DECLARE_METATYPE(AkAudioCaps::ChannelLayout)
Q_DECLARE_METATYPE(AkAudioCaps::SampleFormatList)
Q_DECLARE_METATYPE(AkAudioCaps::ChannelLayoutList)

#endif // AKAUDIOCAPS_H
