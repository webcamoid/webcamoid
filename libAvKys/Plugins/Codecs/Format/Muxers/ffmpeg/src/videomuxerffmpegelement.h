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

#ifndef VIDEOMUXERFFMPEGELEMENT_H
#define VIDEOMUXERFFMPEGELEMENT_H

#include <iak/akvideomuxer.h>

class VideoMuxerFFmpegElementPrivate;

class VideoMuxerFFmpegElement: public AkVideoMuxer
{
    Q_OBJECT
    Q_PROPERTY(QStringList options
               READ options
               NOTIFY optionsChanged)

    public:
        enum OptionType
        {
            OptionType_Unknown,
            OptionType_Number,
            OptionType_Boolean,
            OptionType_Flags,
            OptionType_String,
            OptionType_Frac,
        };
        Q_ENUM(OptionType)

        VideoMuxerFFmpegElement();
        ~VideoMuxerFFmpegElement();

        Q_INVOKABLE QStringList muxers() const override;
        Q_INVOKABLE FormatID formatID(const QString &muxer) const override;
        Q_INVOKABLE QString description(const QString &muxer) const override;
        Q_INVOKABLE QString extension(const QString &muxer) const override;
        Q_INVOKABLE bool gapsAllowed(AkCodecType type) const override;
        Q_INVOKABLE QList<AkCodecID> supportedCodecs(const QString &muxer,
                                                     AkCodecType type) const override;
        Q_INVOKABLE AkCodecID defaultCodec(const QString &muxer,
                                           AkCodecType type) const override;
        Q_INVOKABLE QStringList options() const;
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
        VideoMuxerFFmpegElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void optionsChanged(const QStringList &options);
        void optionValueChanged(const QString &option, const QVariant &value);

    public slots:
        void setOptionValue(const QString &option, const QVariant &value);
        void resetOptionValue(const QString &option);
        void resetFormatOptions();
        void resetOptions() override;
        AkPacket iStream(const AkPacket &packet) override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoMuxerFFmpegElement::OptionType)

#endif // VIDEOMUXERFFMPEGELEMENT_H
