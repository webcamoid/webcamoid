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
#include <QSettings>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akplugin.h>

#include "videolayer.h"

using ObjectPtr = QSharedPointer<QObject>;

class VideoLayerPrivate
{
    public:
        VideoLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QString m_videoInput;
        QStringList m_inputs;
        QMap<QString, QString> m_streams;
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
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
        AkElement::ElementState m_inputState {AkElement::ElementStateNull};
        bool m_playOnStart {false};

        explicit VideoLayerPrivate(VideoLayer *self);
        AkElementPtr sourceElement(const QString &stream) const;
        QStringList cameras() const;
        QStringList desktops() const;
        QString cameraDescription(const QString &camera) const;
        QString desktopDescription(const QString &desktop) const;
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        void loadProperties();
        void saveVideoInput(const QString &videoInput);
        void saveStreams(const QMap<QString, QString> &streams);
        void savePlayOnStart(bool playOnStart);
};

VideoLayer::VideoLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerPrivate(this);
    this->setQmlEngine(engine);

    if (this->d->m_cameraCapture) {
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateInputs()));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(streamsChanged(const QList<int> &)),
                         this,
                         SLOT(updateCaps()));
    }

    if (this->d->m_cameraCaptureSettings) {
        QObject::connect(this->d->m_cameraCaptureSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCodecLib(const QString &)));
        QObject::connect(this->d->m_cameraCaptureSettings.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCaptureLib(const QString &)));
    }

    if (this->d->m_desktopCapture) {
        QObject::connect(this->d->m_desktopCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->d->m_desktopCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateInputs()));
        QObject::connect(this->d->m_desktopCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
    }

    if (this->d->m_desktopCaptureSettings)
        QObject::connect(this->d->m_desktopCaptureSettings.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveDesktopCaptureCaptureLib(const QString &)));

    if (this->d->m_uriCapture) {
        this->d->m_uriCapture->setProperty("objectName", "uriCapture");
        this->d->m_uriCapture->setProperty("loop", true);
        this->d->m_uriCapture->setProperty("audioAlign", true);

        QObject::connect(this->d->m_uriCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->d->m_uriCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
    }

    if (this->d->m_uriCaptureSettings)
        QObject::connect(this->d->m_uriCaptureSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveMultiSrcCodecLib(const QString &)));

    this->d->loadProperties();
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

QStringList VideoLayer::inputs() const
{
    return this->d->m_inputs;
}

AkAudioCaps VideoLayer::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps VideoLayer::videoCaps() const
{
    return this->d->m_videoCaps;
}

AkElement::ElementState VideoLayer::state() const
{
    return this->d->m_inputState;
}

bool VideoLayer::playOnStart() const
{
    return this->d->m_playOnStart;
}

VideoLayer::InputType VideoLayer::deviceType(const QString &stream) const
{
    if (this->d->cameras().contains(stream))
        return InputCamera;

    if (this->d->desktops().contains(stream))
        return InputDesktop;

    if (this->d->m_streams.contains(stream))
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

QString VideoLayer::description(const QString &stream) const
{
    if (this->d->cameras().contains(stream))
        return this->d->cameraDescription(stream);

    if (this->d->desktops().contains(stream))
        return this->d->desktopDescription(stream);

    return this->d->m_streams.value(stream);
}

bool VideoLayer::embedControls(const QString &where,
                                const QString &stream,
                                const QString &name) const
{
    auto source = this->d->sourceElement(stream);

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

void VideoLayer::setState(AkElement::ElementState state)
{
    if (this->d->m_inputState == state)
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
            || source->state() != this->d->m_inputState) {
            auto state = source->state();
            this->d->m_inputState = state;
            emit this->stateChanged(state);
        }
    } else {
        if (this->d->m_inputState != AkElement::ElementStateNull) {
            this->d->m_inputState = AkElement::ElementStateNull;
            emit this->stateChanged(AkElement::ElementStateNull);
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
    this->setVideoInput("");
}

void VideoLayer::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoLayer::resetPlayOnStart()
{
    this->setPlayOnStart(false);
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

    this->setState(state);
    this->d->setAudioCaps(audioCaps);
    this->d->setVideoCaps(videoCaps);
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

VideoLayerPrivate::VideoLayerPrivate(VideoLayer *self):
    self(self)
{

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

void VideoLayerPrivate::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->m_audioCaps == audioCaps)
        return;

    this->m_audioCaps = audioCaps;
    emit self->audioCapsChanged(audioCaps);
}

void VideoLayerPrivate::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->m_videoCaps == videoCaps)
        return;

    this->m_videoCaps = videoCaps;
    emit self->videoCapsChanged(videoCaps);
}

void VideoLayerPrivate::loadProperties()
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
}

void VideoLayerPrivate::saveVideoInput(const QString &videoInput)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("stream", videoInput);
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
