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

#include <QFile>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QSettings>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akplugin.h>
#include <akvideocaps.h>

#include "videolayer.h"
#include "clioptions.h"
#include "mediatools.h"

#define DUMMY_OUTPUT_DEVICE ":dummyout:"

using ObjectPtr = QSharedPointer<QObject>;

class VideoLayerPrivate
{
    public:
        VideoLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QString m_videoInput;
        QStringList m_videoOutput;
        QStringList m_inputs;
        QMap<QString, QString> m_streams;
        AkAudioCaps m_inputAudioCaps;
        AkVideoCaps m_inputVideoCaps;
        AkElementPtr m_cameraCapture {AkElement::create("VideoCapture")};
        ObjectPtr m_cameraCaptureSettings {
            AkElement::create<QObject>("VideoCapture",
                                       AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        AkElementPtr m_desktopCapture {AkElement::create("DesktopCapture")};
        ObjectPtr m_desktopCaptureSettings {
            AkElement::create<QObject>("DesktopCapture",
                                       AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        AkElementPtr m_uriCapture {AkElement::create("MultiSrc")};
        ObjectPtr m_uriCaptureSettings {
            AkElement::create<QObject>("MultiSrc",
                                       AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        AkElementPtr m_cameraOutput {AkElement::create("VirtualCamera")};
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_playOnStart {true};

        explicit VideoLayerPrivate(VideoLayer *self);
        void connectSignals();
        AkElementPtr sourceElement(const QString &stream) const;
        QStringList cameras() const;
        QStringList desktops() const;
        QString cameraDescription(const QString &camera) const;
        QString desktopDescription(const QString &desktop) const;
        void setInputAudioCaps(const AkAudioCaps &audioCaps);
        void setInputVideoCaps(const AkVideoCaps &videoCaps);
        void loadProperties(const CliOptions &cliOptions);
        void saveVideoInput(const QString &videoInput);
        void saveVideoOutput(const QString &videoOutput);
        void saveStreams(const QMap<QString, QString> &streams);
        void savePlayOnStart(bool playOnStart);
};

VideoLayer::VideoLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerPrivate(this);
    this->setQmlEngine(engine);
    this->d->connectSignals();
    this->d->loadProperties({});
}

VideoLayer::VideoLayer(const CliOptions &cliOptions,
                       QQmlApplicationEngine *engine,
                       QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerPrivate(this);
    this->setQmlEngine(engine);
    this->d->connectSignals();
    this->d->loadProperties(cliOptions);
}

VideoLayer::~VideoLayer()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VideoLayer::videoInput() const
{
    return this->d->m_videoInput;
}

QStringList VideoLayer::videoOutput() const
{
    return this->d->m_videoOutput;
}

QStringList VideoLayer::inputs() const
{
    return this->d->m_inputs;
}

QStringList VideoLayer::outputs() const
{
    QStringList outputs;

    if (this->d->m_cameraOutput) {
        auto outs = this->d->m_cameraOutput->property("medias").toStringList();

        if (!outs.isEmpty())
            outputs << DUMMY_OUTPUT_DEVICE << outs;
    }

    return outputs;
}

AkAudioCaps VideoLayer::inputAudioCaps() const
{
    return this->d->m_inputAudioCaps;
}

AkVideoCaps VideoLayer::inputVideoCaps() const
{
    return this->d->m_inputVideoCaps;
}

AkVideoCaps::PixelFormatList VideoLayer::supportedOutputPixelFormats() const
{
    if (!this->d->m_cameraOutput)
        return {};

    AkVideoCaps::PixelFormatList pixelFormats;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "supportedOutputPixelFormats",
                              Q_RETURN_ARG(AkVideoCaps::PixelFormatList, pixelFormats));

    return pixelFormats;
}

AkVideoCaps::PixelFormat VideoLayer::defaultOutputPixelFormat() const
{
    if (!this->d->m_cameraOutput)
        return AkVideoCaps::Format_none;

    AkVideoCaps::PixelFormat pixelFormat;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "defaultOutputPixelFormat",
                              Q_RETURN_ARG(AkVideoCaps::PixelFormat, pixelFormat));

    return pixelFormat;
}

AkVideoCapsList VideoLayer::supportedOutputVideoCaps(const QString &device) const
{
    if (!this->d->m_cameraOutput)
        return {};

    AkVideoCapsList caps;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "description",
                              Q_RETURN_ARG(AkVideoCapsList, caps),
                              Q_ARG(QString, device));

    return caps;
}

AkElement::ElementState VideoLayer::state() const
{
    return this->d->m_state;
}

bool VideoLayer::playOnStart() const
{
    return this->d->m_playOnStart;
}

VideoLayer::InputType VideoLayer::deviceType(const QString &device) const
{
    if (this->d->cameras().contains(device))
        return InputCamera;

    if (this->d->desktops().contains(device))
        return InputDesktop;

    if (this->d->m_streams.contains(device))
        return InputStream;

    return InputUnknown;
}

QStringList VideoLayer::devicesByType(InputType type) const
{
    switch (type) {
    case InputCamera:
        return this->d->cameras();

    case InputDesktop:
        return this->d->desktops();

    case InputStream:
        return this->d->m_streams.keys();

    default:
        break;
    }

    return {};
}

QString VideoLayer::description(const QString &device) const
{
    if (device == DUMMY_OUTPUT_DEVICE)
        //: Disable video output, don't send the video to the output device.
        return tr("No Output");

    if (this->d->m_cameraOutput) {
        auto outputs = this->d->m_cameraOutput->property("medias").toStringList();

        if (outputs.contains(device)) {
            QString description;
            QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                      "description",
                                      Q_RETURN_ARG(QString, description),
                                      Q_ARG(QString, device));

            return description;
        }
    }

    if (this->d->cameras().contains(device))
        return this->d->cameraDescription(device);

    if (this->d->desktops().contains(device))
        return this->d->desktopDescription(device);

    if (this->d->m_streams.contains(device))
        return this->d->m_streams.value(device);

    return {};
}

bool VideoLayer::embedControls(const QString &where,
                                const QString &device,
                                const QString &name) const
{
    auto source = this->d->sourceElement(device);

    if (!source)
        return false;

    auto interface = source->controlInterface(this->d->m_engine,
                                              source->pluginId());

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

void VideoLayer::removeInterface(const QString &where) const
{
    if (!this->d->m_engine)
        return;

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto childItems = item->childItems();

        for (auto child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);
            delete child;
        }
    }
}

void VideoLayer::setInputStream(const QString &stream,
                                const QString &description)
{
    if (stream.isEmpty()
        || description.isEmpty()
        || this->d->m_streams.value(stream) == description)
        return;

    this->d->m_streams[stream] = description;
    this->updateInputs();
    this->d->saveStreams(this->d->m_streams);
}

void VideoLayer::removeInputStream(const QString &stream)
{
    if (stream.isEmpty()
        || !this->d->m_streams.contains(stream))
        return;

    this->d->m_streams.remove(stream);
    this->updateInputs();
    this->d->saveStreams(this->d->m_streams);
}

void VideoLayer::setVideoInput(const QString &videoInput)
{
    if (this->d->m_videoInput == videoInput)
        return;

    this->d->m_videoInput = videoInput;
    emit this->videoInputChanged(videoInput);
    this->d->saveVideoInput(videoInput);
    this->updateCaps();
}

void VideoLayer::setVideoOutput(const QStringList &videoOutput)
{
    if (this->d->m_videoOutput == videoOutput)
        return;

    auto output = videoOutput.value(0);

    if (this->d->m_cameraOutput) {
        auto state = this->d->m_cameraOutput->state();
        this->d->m_cameraOutput->setState(AkElement::ElementStateNull);

        if (videoOutput.contains(DUMMY_OUTPUT_DEVICE)) {
            this->d->m_cameraOutput->setProperty("media", QString());
        } else {
            this->d->m_cameraOutput->setProperty("media", output);

            if (!output.isEmpty())
                this->d->m_cameraOutput->setState(state);
        }
    }

    this->d->m_videoOutput = videoOutput;
    emit this->videoOutputChanged(videoOutput);
    this->d->saveVideoOutput(output);
}

void VideoLayer::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    AkElementPtr source;

    if (this->d->cameras().contains(this->d->m_videoInput)) {
        if (this->d->m_desktopCapture)
            this->d->m_desktopCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_cameraCapture;
    } else if (this->d->desktops().contains(this->d->m_videoInput)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_desktopCapture;
    } else if (this->d->m_streams.contains(this->d->m_videoInput)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_desktopCapture)
            this->d->m_desktopCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_uriCapture;
    }

    if (source) {
        if (source->setState(state)
            || source->state() != this->d->m_state) {
            auto state = source->state();
            this->d->m_state = state;
            emit this->stateChanged(state);

            if (this->d->m_cameraOutput) {
                if (this->d->m_videoOutput.isEmpty()
                    || this->d->m_videoOutput.contains(DUMMY_OUTPUT_DEVICE))
                    this->d->m_cameraOutput->setState(AkElement::ElementStateNull);
                else
                    this->d->m_cameraOutput->setState(state);
            }
        }
    } else {
        if (this->d->m_state != AkElement::ElementStateNull) {
            this->d->m_state = AkElement::ElementStateNull;
            emit this->stateChanged(AkElement::ElementStateNull);

            if (this->d->m_cameraOutput)
                this->d->m_cameraOutput->setState(AkElement::ElementStateNull);
        }
    }
}

void VideoLayer::setPlayOnStart(bool playOnStart)
{
    if (this->d->m_playOnStart == playOnStart)
        return;

    this->d->m_playOnStart = playOnStart;
    emit this->playOnStartChanged(playOnStart);
    this->d->savePlayOnStart(playOnStart);
}

void VideoLayer::resetVideoInput()
{
    this->setVideoInput({});
}

void VideoLayer::resetVideoOutput()
{
    this->setVideoOutput({});
}

void VideoLayer::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoLayer::resetPlayOnStart()
{
    this->setPlayOnStart(true);
}

void VideoLayer::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("videoLayer", this);
        qmlRegisterType<VideoLayer>("Webcamoid", 1, 0, "VideoLayer");
    }
}

void VideoLayer::updateCaps()
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);
    auto source = this->d->sourceElement(this->d->m_videoInput);

    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setState(AkElement::ElementStateNull);

    AkCaps audioCaps;
    AkCaps videoCaps;

    if (source) {
        // Set the resource to play.
        source->setProperty("media", this->d->m_videoInput);

        // Update output caps.
        QList<int> streams;
        QMetaObject::invokeMethod(source.data(),
                                  "streams",
                                  Q_RETURN_ARG(QList<int>, streams));

        if (streams.isEmpty()) {
            int audioStream = -1;
            int videoStream = -1;

            // Find the defaults audio and video streams.
            QMetaObject::invokeMethod(source.data(),
                                      "defaultStream",
                                      Q_RETURN_ARG(int, audioStream),
                                      Q_ARG(QString, "audio/x-raw"));
            QMetaObject::invokeMethod(source.data(),
                                      "defaultStream",
                                      Q_RETURN_ARG(int, videoStream),
                                      Q_ARG(QString, "video/x-raw"));

            // Read streams caps.
            if (audioStream >= 0)
                QMetaObject::invokeMethod(source.data(),
                                          "caps",
                                          Q_RETURN_ARG(AkCaps, audioCaps),
                                          Q_ARG(int, audioStream));

            if (videoStream >= 0)
                QMetaObject::invokeMethod(source.data(),
                                          "caps",
                                          Q_RETURN_ARG(AkCaps, videoCaps),
                                          Q_ARG(int, videoStream));
        } else {
            for (const int &stream: streams) {
                AkCaps caps;
                QMetaObject::invokeMethod(source.data(),
                                          "caps",
                                          Q_RETURN_ARG(AkCaps, caps),
                                          Q_ARG(int, stream));

                if (caps.mimeType() == "audio/x-raw")
                    audioCaps = caps;
                else if (caps.mimeType() == "video/x-raw")
                    videoCaps = caps;
            }
        }
    }

    if (this->d->m_cameraOutput) {
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "clearStreams");
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "addStream",
                                  Q_ARG(int, 0),
                                  Q_ARG(AkCaps, videoCaps));

        if (!this->d->m_videoOutput.isEmpty() &&
            !this->d->m_videoOutput.contains(DUMMY_OUTPUT_DEVICE))
            this->d->m_cameraOutput->setState(state);
    }

    this->setState(state);
    this->d->setInputAudioCaps(audioCaps);
    this->d->setInputVideoCaps(videoCaps);
}

void VideoLayer::updateInputs()
{
    QStringList inputs;
    QMap<QString, QString> descriptions;

    // Read cameras
    auto cameras = this->d->cameras();
    inputs << cameras;

    for (auto &camera: cameras) {
        QString description;
        QMetaObject::invokeMethod(this->d->m_cameraCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, camera));
        descriptions[camera] = description;
    }

    // Read desktops
    auto desktops = this->d->desktops();
    inputs << desktops;

    for (auto &desktop: desktops) {
        QString description;
        QMetaObject::invokeMethod(this->d->m_desktopCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, desktop));
        descriptions[desktop] = description;
    }

    // Read streams
    inputs << this->d->m_streams.keys();

    for (auto it = this->d->m_streams.begin();
         it != this->d->m_streams.end();
         it++)
        descriptions[it.key()] = it.value();

    // Remove outputs to prevent self blocking.
    if (this->d->m_cameraOutput) {
        auto outputs =
                this->d->m_cameraOutput->property("medias").toStringList();

        for (auto &output: outputs)
            inputs.removeAll(output);
    }

    // Update inputs
    if (inputs != this->d->m_inputs) {
        this->d->m_inputs = inputs;
        emit this->inputsChanged(this->d->m_inputs);

        if (!this->d->m_inputs.contains(this->d->m_videoInput))
            this->setVideoInput(this->d->m_inputs.value(0));
    }
}

void VideoLayer::saveVideoCaptureCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VideoCapture.codecLib", codecLib);
    config.endGroup();
}

void VideoLayer::saveVideoCaptureCaptureLib(const QString &captureLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VideoCapture.captureLib", captureLib);
    config.endGroup();
}

void VideoLayer::saveDesktopCaptureCaptureLib(const QString &captureLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("DesktopCapture.captureLib", captureLib);
    config.endGroup();
}

void VideoLayer::saveMultiSrcCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("MultiSrc.codecLib", codecLib);
    config.endGroup();
}

AkPacket VideoLayer::iStream(const AkPacket &packet)
{
    if (this->d->m_cameraOutput
        && !this->d->m_videoOutput.isEmpty()
        && !this->d->m_videoOutput.contains(DUMMY_OUTPUT_DEVICE))
        this->d->m_cameraOutput->iStream(packet);

    return {};
}

VideoLayerPrivate::VideoLayerPrivate(VideoLayer *self):
    self(self)
{

}

void VideoLayerPrivate::connectSignals()
{
    if (this->m_cameraCapture) {
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         self,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         self,
                         SLOT(updateInputs()));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(error(const QString &)),
                         self,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(streamsChanged(const QList<int> &)),
                         self,
                         SLOT(updateCaps()));
    }

    if (this->m_cameraCaptureSettings) {
        QObject::connect(this->m_cameraCaptureSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         self,
                         SLOT(saveVideoCaptureCodecLib(const QString &)));
        QObject::connect(this->m_cameraCaptureSettings.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         self,
                         SLOT(saveVideoCaptureCaptureLib(const QString &)));
    }

    if (this->m_desktopCapture) {
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         self,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         self,
                         SLOT(updateInputs()));
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(error(const QString &)),
                         self,
                         SIGNAL(error(const QString &)));
    }

    if (this->m_desktopCaptureSettings)
        QObject::connect(this->m_desktopCaptureSettings.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         self,
                         SLOT(saveDesktopCaptureCaptureLib(const QString &)));

    if (this->m_uriCapture) {
        this->m_uriCapture->setProperty("objectName", "uriCapture");
        this->m_uriCapture->setProperty("loop", true);
        this->m_uriCapture->setProperty("audioAlign", true);

        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         self,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(error(const QString &)),
                         self,
                         SIGNAL(error(const QString &)));
    }

    if (this->m_uriCaptureSettings)
        QObject::connect(this->m_uriCaptureSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         self,
                         SLOT(saveMultiSrcCodecLib(const QString &)));

    if (this->m_cameraOutput) {
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         self,
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         Qt::DirectConnection);
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         self,
                         SIGNAL(outputsChanged(const QStringList &)));
    }
}

AkElementPtr VideoLayerPrivate::sourceElement(const QString &stream) const
{
    if (this->cameras().contains(stream))
        return this->m_cameraCapture;

    if (this->desktops().contains(stream))
        return this->m_desktopCapture;

    if (this->m_streams.contains(stream))
        return this->m_uriCapture;

    return {};
}

QStringList VideoLayerPrivate::cameras() const
{
    if (!this->m_cameraCapture)
        return {};

    QStringList cameras;
    QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, cameras));
    return cameras;
}

QStringList VideoLayerPrivate::desktops() const
{
    if (!this->m_desktopCapture)
        return {};

    QStringList desktops;
    QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, desktops));
    return desktops;
}

QString VideoLayerPrivate::cameraDescription(const QString &camera) const
{
    if (!this->m_cameraCapture)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, camera));

    return description;
}

QString VideoLayerPrivate::desktopDescription(const QString &desktop) const
{
    if (!this->m_desktopCapture)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, desktop));

    return description;
}

void VideoLayerPrivate::setInputAudioCaps(const AkAudioCaps &inputAudioCaps)
{
    if (this->m_inputAudioCaps == inputAudioCaps)
        return;

    this->m_inputAudioCaps = inputAudioCaps;
    emit self->inputAudioCapsChanged(inputAudioCaps);
}

void VideoLayerPrivate::setInputVideoCaps(const AkVideoCaps &inputVideoCaps)
{
    if (this->m_inputVideoCaps == inputVideoCaps)
        return;

    this->m_inputVideoCaps = inputVideoCaps;
    emit self->inputVideoCapsChanged(inputVideoCaps);
}

void VideoLayerPrivate::loadProperties(const CliOptions &cliOptions)
{
    QSettings config;
    config.beginGroup("Libraries");

    if (this->m_cameraCaptureSettings) {
        auto codecLib =
                config.value("VideoCapture.codecLib",
                             this->m_cameraCaptureSettings->property("codecLib"));
        this->m_cameraCaptureSettings->setProperty("codecLib", codecLib);
        auto captureLib =
                config.value("VideoCapture.captureLib",
                             this->m_cameraCaptureSettings->property("captureLib"));
        this->m_cameraCaptureSettings->setProperty("captureLib", captureLib);
    }

    if (this->m_desktopCaptureSettings) {
        auto captureLib =
                config.value("DesktopCapture.captureLib",
                             this->m_desktopCaptureSettings->property("captureLib"));
        this->m_desktopCaptureSettings->setProperty("captureLib", captureLib);
    }

    if (this->m_uriCaptureSettings) {
        auto codecLib =
                config.value("MultiSrc.codecLib",
                             this->m_uriCaptureSettings->property("codecLib"));
        this->m_uriCapture->setProperty("codecLib", codecLib);
    }

    config.endGroup();

    config.beginGroup("StreamConfigs");
    this->m_videoInput = config.value("stream").toString();
    this->m_playOnStart = config.value("playOnStart", true).toBool();

    int size = config.beginReadArray("uris");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        auto uri = config.value("uri").toString();
        auto description = config.value("description").toString();
        this->m_streams[uri] = description;
    }

    config.endArray();
    config.endGroup();

    self->updateInputs();
    self->updateCaps();

    config.beginGroup("VirtualCamera");

    if (this->m_cameraOutput) {
        auto optPaths = cliOptions.value(cliOptions.vcamPathOpt()).split(';');
        QStringList driverPaths;

        for (auto path: optPaths) {
            path = MediaTools::convertToAbsolute(path);

            if (QFileInfo::exists(path))
                driverPaths << path;
        }

        QMetaObject::invokeMethod(this->m_cameraOutput.data(),
                                  "addDriverPaths",
                                  Q_ARG(QStringList, driverPaths));
        auto streams = this->m_cameraOutput->property("medias").toStringList();
        auto stream = config.value("stream", streams.value(0)).toString();
        this->m_videoOutput = QStringList {stream};

        if (stream != DUMMY_OUTPUT_DEVICE)
            this->m_cameraOutput->setProperty("media", stream);
    }

    config.endGroup();
}

void VideoLayerPrivate::saveVideoInput(const QString &videoInput)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("stream", videoInput);
    config.endGroup();
}

void VideoLayerPrivate::saveVideoOutput(const QString &videoOutput)
{
    QSettings config;
    config.beginGroup("VirtualCamera");
    config.setValue("stream", videoOutput);
    config.endGroup();
}

void VideoLayerPrivate::saveStreams(const QMap<QString, QString> &streams)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.beginWriteArray("uris");

    int i = 0;

    for (auto it = streams.begin(); it != streams.end(); it++) {
        config.setArrayIndex(i);
        config.setValue("uri", it.key());
        config.setValue("description", it.value());
        i++;
    }

    config.endArray();
    config.endGroup();
}

void VideoLayerPrivate::savePlayOnStart(bool playOnStart)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("playOnStart", playOnStart);
    config.endGroup();
}

#include "moc_videolayer.cpp"
