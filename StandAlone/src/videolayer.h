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

class VideoLayerPrivate;
class VideoLayer;
class AkAudioCaps;
class AkVideoCaps;
class QQmlApplicationEngine;

using VideoLayerPtr = QSharedPointer<VideoLayer>;

class VideoLayer: public QObject
{
    Q_OBJECT
    Q_ENUMS(InputType)
    Q_PROPERTY(QString videoInput
               READ videoInput
               WRITE setVideoInput
               RESET resetVideoInput
               NOTIFY videoInputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(AkAudioCaps audioCaps
               READ audioCaps
               NOTIFY audioCapsChanged)
    Q_PROPERTY(AkVideoCaps videoCaps
               READ videoCaps
               NOTIFY videoCapsChanged)
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

        VideoLayer(QQmlApplicationEngine *engine=nullptr,
                    QObject *parent=nullptr);
        ~VideoLayer();

        Q_INVOKABLE QString videoInput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE InputType deviceType(const QString &videoInput) const;
        Q_INVOKABLE QStringList devicesByType(InputType type) const;
        Q_INVOKABLE QString description(const QString &videoInput) const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       const QString &videoInput,
                                       const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoLayerPrivate *d;

    signals:
        void videoInputChanged(const QString &videoInput);
        void inputsChanged(const QStringList &inputs);
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void stateChanged(AkElement::ElementState state);
        void playOnStartChanged(bool playOnStart);
        void oStream(const AkPacket &packet);
        void error(const QString &message);

    public slots:
        void setInputStream(const QString &stream, const QString &description);
        void removeInputStream(const QString &stream);
        void setVideoInput(const QString &videoInput);
        void setState(AkElement::ElementState state);
        void setPlayOnStart(bool playOnStart);
        void resetVideoInput();
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

    friend VideoLayerPrivate;
};

Q_DECLARE_METATYPE(VideoLayer::InputType)

#endif // VIDEOLAYER_H
