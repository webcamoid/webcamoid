/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef ENCODING_H
#define ENCODING_H

#include <QObject>
#include <QSharedPointer>
#include <QVariant>
#include <akcaps.h>
#include <akpropertyoption.h>
#include <akcompressedcaps.h>
#include <iak/akaudioencoder.h>
#include <iak/akvideoencoder.h>

class EncodingPrivate;
class Encoding;
class QQmlApplicationEngine;

using EncodingPtr = QSharedPointer<Encoding>;

/* Shared encoding settings for all the outputs (recording, streaming and local
 * streaming).
 *
 * Codecs are referred by a generic key ("h264", "h265", "vp8", "vp9", "av1",
 * "mjpeg", "aac", "mp3", "opus", "vorbis"), independent of the plugin that
 * implements them. Each generic codec is associated to one encoder
 * implementation (a "pluginID:codecName" string, for example a software or a
 * hardware encoder), and the options of each encoder are stored here too.
 */
class Encoding: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int videoBitrate
               READ videoBitrate
               WRITE setVideoBitrate
               RESET resetVideoBitrate
               NOTIFY videoBitrateChanged)
    Q_PROPERTY(int videoGOP
               READ videoGOP
               WRITE setVideoGOP
               RESET resetVideoGOP
               NOTIFY videoGOPChanged)
    Q_PROPERTY(int audioBitrate
               READ audioBitrate
               WRITE setAudioBitrate
               RESET resetAudioBitrate
               NOTIFY audioBitrateChanged)
    Q_PROPERTY(int audioSampleRate
               READ audioSampleRate
               WRITE setAudioSampleRate
               RESET resetAudioSampleRate
               NOTIFY audioSampleRateChanged)

    public:
        Encoding(QQmlApplicationEngine *engine=nullptr,
                 QObject *parent=nullptr);
        ~Encoding();

        // Common values. Bitrates are in bits/s, the keyframes interval in ms
        // and the sample rate in Hz.
        Q_INVOKABLE int videoBitrate() const;
        Q_INVOKABLE int videoGOP() const;
        Q_INVOKABLE int audioBitrate() const;
        Q_INVOKABLE int audioSampleRate() const;
        Q_INVOKABLE int defaultVideoBitrate() const;
        Q_INVOKABLE int defaultVideoGOP() const;
        Q_INVOKABLE int defaultAudioBitrate() const;
        Q_INVOKABLE int defaultAudioSampleRate() const;

        // Generic codecs with, at least, one encoder available.
        Q_INVOKABLE QStringList codecs(AkCaps::CapsType type) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE AkCaps::CapsType codecType(const QString &codec) const;

        // The generic codec implemented by an encoder.
        Q_INVOKABLE QString encoderCodec(const QString &encoder) const;

        // Encoder implementations of a generic codec, the first one is the
        // default.
        Q_INVOKABLE QStringList encoders(AkCaps::CapsType type,
                                         const QString &codec) const;
        Q_INVOKABLE QString encoder(AkCaps::CapsType type,
                                    const QString &codec) const;
        Q_INVOKABLE QString defaultEncoder(AkCaps::CapsType type,
                                           const QString &codec) const;
        Q_INVOKABLE QString encoderDescription(const QString &encoder) const;
        Q_INVOKABLE bool encoderIsHardware(const QString &encoder) const;

        // The ID the muxers use to refer to the codec implemented by the
        // encoder associated to the generic codec.
        Q_INVOKABLE quint32 encoderCodecID(AkCaps::CapsType type,
                                           const QString &codec) const;

        // Encoder options.
        Q_INVOKABLE AkPropertyOptions encoderOptions(const QString &encoder) const;
        Q_INVOKABLE QVariant encoderOptionValue(const QString &encoder,
                                                const QString &option) const;

        // The encoder associated to each codec, with the options stored for
        // it, for the outputs that create their own encoders (like the local
        // streaming server). Each element is a map with the keys:
        //
        //   type    "video" or "audio"
        //   codecID Generic ID of the codec
        //   plugin  ID of the plugin that provides the encoder
        //   codec   Name of the codec inside the plugin
        //   options Map with the values stored for the options of the encoder
        QVariantList encoderPreferences() const;

        // Encoders ready to be used by the outputs, with the stored options
        // already applied. The bitrate, GOP, and input caps are set by the
        // caller.
        //
        // Some outputs can't work with certain encoders, they can be skipped
        // with the list of excluded encoder names (the codec name without the
        // plugin ID). In that case the next encoder of the codec is used.
        AkVideoEncoderPtr createVideoEncoder(const QString &codec,
                                             const QStringList &excludedEncoders={}) const;
        AkAudioEncoderPtr createAudioEncoder(const QString &codec,
                                             const QStringList &excludedEncoders={}) const;

    private:
        EncodingPrivate *d;

    signals:
        void videoBitrateChanged(int videoBitrate);
        void videoGOPChanged(int videoGOP);
        void audioBitrateChanged(int audioBitrate);
        void audioSampleRateChanged(int audioSampleRate);
        void encoderChanged(AkCaps::CapsType type,
                            const QString &codec,
                            const QString &encoder);
        void encoderOptionsChanged(const QString &encoder,
                                   const AkPropertyOptions &options);
        void encoderOptionValueChanged(const QString &encoder,
                                       const QString &option,
                                       const QVariant &value);

        // Emitted once, when the user finishes editing (see finishEditing()),
        // if anything changed since the last time. The outputs that need to
        // restart to use the new settings, like the local streaming server,
        // should wait for it instead of reacting to every change.
        void changesCommitted();

    public slots:
        void setVideoBitrate(int videoBitrate);
        void setVideoGOP(int videoGOP);
        void setAudioBitrate(int audioBitrate);
        void setAudioSampleRate(int audioSampleRate);
        void setEncoder(AkCaps::CapsType type,
                        const QString &codec,
                        const QString &encoder);
        void setEncoderOptionValue(const QString &encoder,
                                   const QString &option,
                                   const QVariant &value);
        void resetVideoBitrate();
        void resetVideoGOP();
        void resetAudioBitrate();
        void resetAudioSampleRate();
        void resetEncoder(AkCaps::CapsType type, const QString &codec);
        void resetEncoderOptionValue(const QString &encoder,
                                     const QString &option);
        void resetEncoderOptions(const QString &encoder);
        void finishEditing();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
};

#endif // ENCODING_H
