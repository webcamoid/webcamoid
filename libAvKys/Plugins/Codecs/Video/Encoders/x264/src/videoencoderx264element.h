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

#ifndef  VIDEOENCODERX264ELEMENT_H
#define  VIDEOENCODERX264ELEMENT_H

#include <iak/akvideoencoder.h>

class  VideoEncoderX264ElementPrivate;

class  VideoEncoderX264Element: public AkVideoEncoder
{
    Q_OBJECT
    Q_PROPERTY(Preset preset
               READ preset
               WRITE setPreset
               RESET resetPreset
               NOTIFY presetChanged)
    Q_PROPERTY(TuneContent tuneContent
               READ tuneContent
               WRITE setTuneContent
               RESET resetTuneContent
               NOTIFY tuneContentChanged)
    Q_PROPERTY(LogLevel logLevel
               READ logLevel
               WRITE setLogLevel
               RESET resetLogLevel
               NOTIFY logLevelChanged)

    public:
        enum Preset
        {
            Preset_Unknown,
            Preset_UltraFast,
            Preset_SuperFast,
            Preset_VeryFast,
            Preset_Faster,
            Preset_Fast,
            Preset_Medium,
            Preset_Slow,
            Preset_Slower,
            Preset_VerySlow,
            Preset_Placebo,
        };
        Q_ENUM(Preset)

        enum TuneContent
        {
            TuneContent_Unknown,
            TuneContent_Film,
            TuneContent_Animation,
            TuneContent_Grain,
            TuneContent_StillImage,
            TuneContent_PSNR,
            TuneContent_SSIM,
            TuneContent_FastDecode,
            TuneContent_ZeroLatency,
        };
        Q_ENUM(TuneContent)

        enum LogLevel
        {
            LogLevel_None,
            LogLevel_Error,
            LogLevel_Warning,
            LogLevel_Info,
            LogLevel_Debug,
        };
        Q_ENUM(LogLevel)

         VideoEncoderX264Element();
        ~ VideoEncoderX264Element();

        Q_INVOKABLE AkVideoEncoderCodecID codec() const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE Preset preset() const;
        Q_INVOKABLE TuneContent tuneContent() const;
        Q_INVOKABLE LogLevel logLevel() const;

    private:
         VideoEncoderX264ElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void presetChanged(Preset preset);
        void tuneContentChanged(TuneContent tuneContent);
        void logLevelChanged(LogLevel logLevel);

    public slots:
        void setPreset(Preset preset);
        void setTuneContent(TuneContent tuneContent);
        void setLogLevel(LogLevel logLevel);
        void resetPreset();
        void resetTuneContent();
        void resetLogLevel();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderX264Element::Preset)
Q_DECLARE_METATYPE(VideoEncoderX264Element::TuneContent)
Q_DECLARE_METATYPE(VideoEncoderX264Element::LogLevel)

#endif // VIDEOENCODERX264ELEMENT_H
