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

#ifndef VIDEOLAYER_H
#define VIDEOLAYER_H

#include <akelement.h>
#include <akvideocaps.h>

class VideoLayerPrivate;
class VideoLayer;
class CliOptions;
class AkAudioCaps;
class QQmlApplicationEngine;

using VideoLayerPtr = QSharedPointer<VideoLayer>;

class VideoLayer: public QObject
{
    Q_OBJECT
    Q_ENUMS(InputType)
    Q_ENUMS(OutputType)
    Q_PROPERTY(QString videoInput
               READ videoInput
               WRITE setVideoInput
               RESET resetVideoInput
               NOTIFY videoInputChanged)
    Q_PROPERTY(QStringList videoOutput
               READ videoOutput
               WRITE setVideoOutput
               RESET resetVideoOutput
               NOTIFY videoOutputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(QStringList outputs
               READ outputs
               NOTIFY outputsChanged)
    Q_PROPERTY(AkAudioCaps inputAudioCaps
               READ inputAudioCaps
               NOTIFY inputAudioCapsChanged)
    Q_PROPERTY(AkVideoCaps inputVideoCaps
               READ inputVideoCaps
               NOTIFY inputVideoCapsChanged)
    Q_PROPERTY(AkVideoCaps::PixelFormatList supportedOutputPixelFormats
               READ supportedOutputPixelFormats
               CONSTANT)
    Q_PROPERTY(AkVideoCaps::PixelFormat defaultOutputPixelFormat
               READ defaultOutputPixelFormat
               CONSTANT)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(bool playOnStart
               READ playOnStart
               WRITE setPlayOnStart
               RESET resetPlayOnStart
               NOTIFY playOnStartChanged)

    public:
        enum InputType {
            InputUnknown,
            InputCamera,
            InputDesktop,
            InputStream
        };
        enum OutputType {
            OutputUnknown,
            OutputVirtualCamera,
        };

        VideoLayer(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        VideoLayer(const CliOptions &cliOptions,
                   QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~VideoLayer();

        Q_INVOKABLE QString videoInput() const;
        Q_INVOKABLE QStringList videoOutput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE QStringList outputs() const;
        Q_INVOKABLE AkAudioCaps inputAudioCaps() const;
        Q_INVOKABLE AkVideoCaps inputVideoCaps() const;
        Q_INVOKABLE AkVideoCaps::PixelFormatList supportedOutputPixelFormats() const;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE AkVideoCapsList supportedOutputVideoCaps(const QString &device) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE InputType deviceType(const QString &device) const;
        Q_INVOKABLE QStringList devicesByType(InputType type) const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       const QString &device,
                                       const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoLayerPrivate *d;

    signals:
        void videoInputChanged(const QString &videoInput);
        void videoOutputChanged(const QStringList &videoOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void inputAudioCapsChanged(const AkAudioCaps &inputAudioCaps);
        void inputVideoCapsChanged(const AkVideoCaps &inputVideoCaps);
        void stateChanged(AkElement::ElementState state);
        void playOnStartChanged(bool playOnStart);
        void oStream(const AkPacket &packet);
        void error(const QString &message);

    public slots:
        void setInputStream(const QString &stream, const QString &description);
        void removeInputStream(const QString &stream);
        void setVideoInput(const QString &videoInput);
        void setVideoOutput(const QStringList &videoOutput);
        void setState(AkElement::ElementState state);
        void setPlayOnStart(bool playOnStart);
        void resetVideoInput();
        void resetVideoOutput();
        void resetState();
        void resetPlayOnStart();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void updateCaps();
        void updateInputs();
        void saveVideoCaptureCodecLib(const QString &codecLib);
        void saveVideoCaptureCaptureLib(const QString &captureLib);
        void saveDesktopCaptureCaptureLib(const QString &captureLib);
        void saveMultiSrcCodecLib(const QString &codecLib);
        AkPacket iStream(const AkPacket &packet);

    friend VideoLayerPrivate;
};

Q_DECLARE_METATYPE(VideoLayer::InputType)
Q_DECLARE_METATYPE(VideoLayer::OutputType)

#endif // VIDEOLAYER_H
