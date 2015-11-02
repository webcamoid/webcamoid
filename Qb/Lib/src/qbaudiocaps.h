/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBAUDIOCAPS_H
#define QBAUDIOCAPS_H

#include "qbcaps.h"

class QbAudioCapsPrivate;

class QbAudioCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(SampleFormat)
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
    Q_PROPERTY(bool align
               READ align
               WRITE setAlign
               RESET resetAlign
               NOTIFY alignChanged)

    public:
        enum SampleFormat
        {
            SampleFormat_none = -1,
            SampleFormat_u8,
            SampleFormat_s16,
            SampleFormat_s32,
            SampleFormat_flt,
            SampleFormat_dbl,
            SampleFormat_u8p,
            SampleFormat_s16p,
            SampleFormat_s32p,
            SampleFormat_fltp
        };

        enum ChannelLayout
        {
            Layout_none = -1,
            Layout_mono,
            Layout_stereo,
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
            Layout_6p1_front,
            Layout_7p0,
            Layout_7p0_front,
            Layout_7p1,
            Layout_7p1_wide,
            Layout_7p1_wide_side,
            Layout_octagonal,
            Layout_downmix
        };

        explicit QbAudioCaps(QObject *parent=NULL);
        QbAudioCaps(const QVariantMap &caps);
        QbAudioCaps(const QString &caps);
        QbAudioCaps(const QbCaps &caps);
        QbAudioCaps(const QbAudioCaps &other);
         ~QbAudioCaps();
        QbAudioCaps &operator =(const QbAudioCaps &other);
        QbAudioCaps &operator =(const QbCaps &caps);
        QbAudioCaps &operator =(const QString &caps);
        bool operator ==(const QbAudioCaps &other) const;
        bool operator !=(const QbAudioCaps &other) const;
        operator bool() const;

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
        Q_INVOKABLE bool align() const;
        Q_INVOKABLE bool &align();

        Q_INVOKABLE QbAudioCaps &fromMap(const QVariantMap &caps);
        Q_INVOKABLE QbAudioCaps &fromString(const QString &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbAudioCaps &update(const QbCaps &caps);
        Q_INVOKABLE QbCaps toCaps() const;

        Q_INVOKABLE static int bytesPerSample(SampleFormat sampleFormat);
        Q_INVOKABLE static QString sampleFormatToString(SampleFormat sampleFormat);
        Q_INVOKABLE static SampleFormat sampleFormatFromString(const QString &sampleFormat);
        Q_INVOKABLE static QString channelLayoutToString(ChannelLayout channelLayout);
        Q_INVOKABLE static ChannelLayout channelLayoutFromString(const QString &channelLayout);

    private:
        QbAudioCapsPrivate *d;

    signals:
        void formatChanged(SampleFormat format);
        void bpsChanged(int bps);
        void channelsChanged(int channels);
        void rateChanged(int rate);
        void layoutChanged(ChannelLayout layout);
        void samplesChanged(int samples);
        void alignChanged(bool align);

    public slots:
        void setFormat(SampleFormat format);
        void setBps(int bps);
        void setChannels(int channels);
        void setRate(int rate);
        void setLayout(ChannelLayout layout);
        void setSamples(int samples);
        void setAlign(bool align);
        void resetFormat();
        void resetBps();
        void resetChannels();
        void resetRate();
        void resetLayout();
        void resetSamples();
        void resetAlign();

        friend QDebug operator <<(QDebug debug, const QbAudioCaps &caps);
        friend QDataStream &operator >>(QDataStream &istream, QbAudioCaps &caps);
        friend QDataStream &operator <<(QDataStream &ostream, const QbAudioCaps &caps);
};

QDebug operator <<(QDebug debug, const QbAudioCaps &caps);
QDataStream &operator >>(QDataStream &istream, QbAudioCaps &caps);
QDataStream &operator <<(QDataStream &ostream, const QbAudioCaps &caps);

Q_DECLARE_METATYPE(QbAudioCaps)
Q_DECLARE_METATYPE(QbAudioCaps::SampleFormat)
Q_DECLARE_METATYPE(QbAudioCaps::ChannelLayout)

#endif // QBAUDIOCAPS_H
