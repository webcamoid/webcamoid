/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef  VIDEOENCODERFFMPEGELEMENT_H
#define  VIDEOENCODERFFMPEGELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderFFmpegElementPrivate;

class VideoEncoderFFmpegElement: public AkVideoEncoder
{
    Q_OBJECT
    Q_PROPERTY(QStringList options
               READ options
               NOTIFY optionsChanged)
    Q_PROPERTY(bool globalHeaders
               READ globalHeaders
               WRITE setGlobalHeaders
               RESET resetGlobalHeaders
               NOTIFY globalHeadersChanged)

    public:
        enum OptionType
        {
            OptionType_Unknown,
            OptionType_Number,
            OptionType_Boolean,
            OptionType_Flags,
            OptionType_String,
            OptionType_Frac,
            OptionType_ImageSize,
            OptionType_Color,
        };
        Q_ENUM(OptionType)

        VideoEncoderFFmpegElement();
        ~VideoEncoderFFmpegElement();

        Q_INVOKABLE QStringList codecs() const override;
        Q_INVOKABLE AkVideoEncoderCodecID codecID(const QString &codec) const override;
        Q_INVOKABLE QString codecDescription(const QString &codec) const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE QStringList options() const;
        Q_INVOKABLE bool globalHeaders() const;
        Q_INVOKABLE QString optionHelp(const QString &option) const;
        Q_INVOKABLE OptionType optionType(const QString &option) const;
        Q_INVOKABLE qreal optionMin(const QString &option) const;
        Q_INVOKABLE qreal optionMax(const QString &option) const;
        Q_INVOKABLE qreal optionStep(const QString &option) const;
        Q_INVOKABLE QVariant optionDefaultValue(const QString &option) const;
        Q_INVOKABLE QVariant optionValue(const QString &option) const;
        Q_INVOKABLE QStringList optionMenu(const QString &option) const;
        Q_INVOKABLE QString menuOptionHelp(const QString &option,
                                           const QString &menuOption) const;
        Q_INVOKABLE QVariant menuOptionValue(const QString &option,
                                             const QString &menuOption) const;


    private:
        VideoEncoderFFmpegElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void optionsChanged(const QStringList &options);
        void globalHeadersChanged(bool globalHeaders);
        void optionValueChanged(const QString &option, const QVariant &value);

    public slots:
        void setGlobalHeaders(bool globalHeaders);
        void setOptionValue(const QString &option, const QVariant &value);
        void resetGlobalHeaders();
        void resetOptionValue(const QString &option);
        void resetCodecOptions();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderFFmpegElement::OptionType)

#endif // VIDEOENCODERFFMPEGELEMENT_H
