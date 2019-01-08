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

#include "mediasource.h"

class MediaSourcePrivate
{
    public:
        QQmlApplicationEngine *m_engine {nullptr};
        QString m_stream;
        QStringList m_streams;
        QStringList m_cameras;
        QStringList m_desktops;
        QVariantMap m_uris;
        QMap<QString, QString> m_descriptions;
        AkCaps m_audioCaps;
        AkCaps m_videoCaps;
        AkElementPtr m_cameraCapture {AkElement::create("VideoCapture")};
        AkElementPtr m_desktopCapture {AkElement::create("DesktopCapture")};
        AkElementPtr m_uriCapture {AkElement::create("MultiSrc")};
        AkElement::ElementState m_inputState {AkElement::ElementStateNull};
        bool m_playOnStart {false};

        AkElementPtr sourceElement(const QString &stream) const;
};

MediaSource::MediaSource(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new MediaSourcePrivate;
    this->setQmlEngine(engine);

    QObject::connect(this,
                     &MediaSource::urisChanged,
                     this,
                     &MediaSource::updateStreams);
    QObject::connect(this,
                     &MediaSource::streamChanged,
                     this,
                     &MediaSource::saveStream);
    QObject::connect(this,
                     &MediaSource::urisChanged,
                     this,
                     &MediaSource::saveUris);
    QObject::connect(this,
                     &MediaSource::playOnStartChanged,
                     this,
                     &MediaSource::savePlayOnStart);

    if (this->d->m_cameraCapture) {
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateStreams()));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCodecLib(const QString &)));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCaptureLib(const QString &)));
        QObject::connect(this->d->m_cameraCapture.data(),
                         SIGNAL(streamsChanged(const QList<int> &)),
                         this,
                         SLOT(webcamStreamsChanged(const QList<int> &)));
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
                         SLOT(updateStreams()));
        QObject::connect(this->d->m_desktopCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->d->m_desktopCapture.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveDesktopCaptureCaptureLib(const QString &)));
    }

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
        QObject::connect(this->d->m_uriCapture.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveMultiSrcCodecLib(const QString &)));
    }

    // Setup streams
    QStringList cameras;

    if (this->d->m_cameraCapture)
        QMetaObject::invokeMethod(this->d->m_cameraCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, cameras));

    QStringList desktops;

    if (this->d->m_desktopCapture)
        QMetaObject::invokeMethod(this->d->m_desktopCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, desktops));

    for (const QString &camera: cameras) {
        QString description;

        QMetaObject::invokeMethod(this->d->m_cameraCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, camera));

        this->d->m_descriptions[camera] = description;
    }

    for (const QString &desktop: desktops) {
        QString description;

        QMetaObject::invokeMethod(this->d->m_desktopCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, desktop));

        this->d->m_descriptions[desktop] = description;
    }

    this->d->m_cameras = cameras;
    this->d->m_desktops = desktops;
    this->d->m_streams = cameras + desktops;

    this->loadProperties();
}

MediaSource::~MediaSource()
{
    this->setState(AkElement::ElementStateNull);
    this->saveProperties();
    delete this->d;
}

QString MediaSource::stream() const
{
    return this->d->m_stream;
}

QStringList MediaSource::streams() const
{
    return this->d->m_streams;
}

QStringList MediaSource::cameras() const
{
    return this->d->m_cameras;
}

QStringList MediaSource::desktops() const
{
    return this->d->m_desktops;
}

QVariantMap MediaSource::uris() const
{
    return this->d->m_uris;
}

AkCaps MediaSource::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkCaps MediaSource::videoCaps() const
{
    return this->d->m_videoCaps;
}

AkElement::ElementState MediaSource::state() const
{
    return this->d->m_inputState;
}

bool MediaSource::playOnStart() const
{
    return this->d->m_playOnStart;
}

QString MediaSource::description(const QString &stream) const
{
    return this->d->m_descriptions.value(stream);
}

bool MediaSource::embedControls(const QString &where,
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

    for (auto obj: this->d->m_engine->rootObjects()) {
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

void MediaSource::removeInterface(const QString &where) const
{
    if (!this->d->m_engine)
        return;

    for (const QObject *obj: this->d->m_engine->rootObjects()) {
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

AkElementPtr MediaSourcePrivate::sourceElement(const QString &stream) const
{
    if (this->m_cameras.contains(stream))
        return this->m_cameraCapture;

    if (this->m_desktops.contains(stream))
        return this->m_desktopCapture;

    if (this->m_uris.contains(stream))
        return this->m_uriCapture;

    return {};
}

void MediaSource::setStream(const QString &stream)
{
    if (this->d->m_stream == stream)
        return;

    this->d->m_stream = stream;
    emit this->streamChanged(stream);
    this->streamUpdated(stream);
}

void MediaSource::setUris(const QVariantMap &uris)
{
    if (this->d->m_uris == uris)
        return;

    this->d->m_uris = uris;
    emit this->urisChanged(uris);
}

void MediaSource::setState(AkElement::ElementState state)
{
    if (this->d->m_inputState == state)
        return;

    AkElementPtr source;

    if (this->d->m_cameras.contains(this->d->m_stream)) {
        if (this->d->m_desktopCapture)
            this->d->m_desktopCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_cameraCapture;
    } else if (this->d->m_desktops.contains(this->d->m_stream)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_desktopCapture;
    } else if (this->d->m_uris.contains(this->d->m_stream)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_desktopCapture)
            this->d->m_desktopCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_uriCapture;
    }

    if (source) {
        if (source->setState(state) || source->state() != this->d->m_inputState) {
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

void MediaSource::setPlayOnStart(bool playOnStart)
{
    if (this->d->m_playOnStart == playOnStart)
        return;

    this->d->m_playOnStart = playOnStart;
    emit this->playOnStartChanged(playOnStart);
}

void MediaSource::resetStream()
{
    this->setStream("");
}

void MediaSource::resetUris()
{
    this->setUris({});
}

void MediaSource::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void MediaSource::resetPlayOnStart()
{
    this->setPlayOnStart(false);
}

void MediaSource::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("MediaSource", this);
}

void MediaSource::streamUpdated(const QString &stream)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);
    auto source = this->d->sourceElement(stream);

    if (!source) {
        this->setAudioCaps(AkCaps());
        this->setVideoCaps(AkCaps());

        if (state != AkElement::ElementStateNull)
            emit this->stateChanged(AkElement::ElementStateNull);

        return;
    }

    // Set the resource to play.
    source->setProperty("media", stream);

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
        if (audioStream >= 0) {
            AkCaps audioCaps;
            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, audioCaps),
                                      Q_ARG(int, audioStream));
            this->setAudioCaps(audioCaps);
        }

        if (videoStream >= 0) {
            AkCaps videoCaps;
            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, videoCaps),
                                      Q_ARG(int, videoStream));
            this->setVideoCaps(videoCaps);
        }
    } else {
        for (const int &stream: streams) {
            AkCaps caps;

            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, caps),
                                      Q_ARG(int, stream));

            if (caps.mimeType() == "audio/x-raw")
                this->setAudioCaps(caps);
            else if (caps.mimeType() == "video/x-raw")
                this->setVideoCaps(caps);/*
            else if (caps.mimeType() == "text/x-raw")
                this->setSubtitlesCaps(caps);*/
        }
    }

    this->setState(state);
}

void MediaSource::updateStreams()
{
    QStringList cameras;

    if (this->d->m_cameraCapture)
        QMetaObject::invokeMethod(this->d->m_cameraCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, cameras));

    QStringList desktops;

    if (this->d->m_desktopCapture)
        QMetaObject::invokeMethod(this->d->m_desktopCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, desktops));

    this->d->m_descriptions.clear();

    for (auto &camera: cameras) {
        QString description;

        QMetaObject::invokeMethod(this->d->m_cameraCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, camera));

        this->d->m_descriptions[camera] = description;
    }

    for (auto &desktop: desktops) {
        QString description;

        QMetaObject::invokeMethod(this->d->m_desktopCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, desktop));

        this->d->m_descriptions[desktop] = description;
    }

    for (auto it = this->d->m_uris.begin(); it != this->d->m_uris.end(); it++)
        this->d->m_descriptions[it.key()] = it.value().toString();

    bool isSet = this->setCameras(cameras);
    isSet |= this->setDesktops(desktops);
    isSet |= this->setStreams(cameras + desktops + this->d->m_uris.keys());

    if (!isSet)
        emit this->streamsChanged(this->d->m_streams);

    if (!this->d->m_streams.contains(this->d->m_stream)) {
        if (this->d->m_streams.isEmpty())
            this->resetStream();
        else
            this->setStream(this->d->m_streams.first());
    }
}

bool MediaSource::setStreams(const QStringList &streams)
{
    if (this->d->m_streams == streams)
        return false;

    this->d->m_streams = streams;
    emit this->streamsChanged(streams);

    return true;
}

bool MediaSource::setCameras(const QStringList &cameras)
{
    if (this->d->m_cameras == cameras)
        return false;

    this->d->m_cameras = cameras;
    emit this->camerasChanged(cameras);

    return true;
}

bool MediaSource::setDesktops(const QStringList &desktops)
{
    if (this->d->m_desktops == desktops)
        return false;

    this->d->m_desktops = desktops;
    emit this->desktopsChanged(desktops);

    return true;
}

void MediaSource::setAudioCaps(const AkCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
}

void MediaSource::setVideoCaps(const AkCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
}

void MediaSource::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");

    if (this->d->m_cameraCapture) {
        this->d->m_cameraCapture->setProperty("codecLib",
                                              config.value("VideoCapture.codecLib",
                                                           this->d->m_cameraCapture->property("codecLib")));
        this->d->m_cameraCapture->setProperty("captureLib",
                                              config.value("VideoCapture.captureLib",
                                                           this->d->m_cameraCapture->property("captureLib")));
    }

    if (this->d->m_desktopCapture)
        this->d->m_desktopCapture->setProperty("captureLib",
                                               config.value("DesktopCapture.captureLib",
                                                            this->d->m_desktopCapture->property("captureLib")));

    if (this->d->m_uriCapture)
        this->d->m_uriCapture->setProperty("codecLib",
                                           config.value("MultiSrc.codecLib",
                                                        this->d->m_uriCapture->property("codecLib")));

    config.endGroup();

    config.beginGroup("StreamConfigs");
    auto stream = config.value("stream").toString();
    this->setPlayOnStart(config.value("playOnStart", true).toBool());

    QVariantMap uris;
    int size = config.beginReadArray("uris");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        auto uri = config.value("uri").toString();
        auto description = config.value("description");
        uris[uri] = description;
    }

    this->setUris(uris);
    config.endArray();
    config.endGroup();

    if (this->d->m_streams.contains(stream))
        this->setStream(stream);
    else if (!this->d->m_streams.isEmpty())
        this->setStream(this->d->m_streams.first());
}

void MediaSource::saveStream(const QString &stream)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("stream", stream);
    config.endGroup();
}

void MediaSource::saveUris(const QVariantMap &uris)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.beginWriteArray("uris");

    int i = 0;

    for (auto it = uris.begin(); it != uris.end(); it++) {
        config.setArrayIndex(i);
        config.setValue("uri", it.key());
        config.setValue("description", it.value());
        i++;
    }

    config.endArray();
    config.endGroup();
}

void MediaSource::savePlayOnStart(bool playOnStart)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("playOnStart", playOnStart);
    config.endGroup();
}

void MediaSource::saveVideoCaptureCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VideoCapture.codecLib", codecLib);
    config.endGroup();
}

void MediaSource::saveVideoCaptureCaptureLib(const QString &captureLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VideoCapture.captureLib", captureLib);
    config.endGroup();
}

void MediaSource::saveDesktopCaptureCaptureLib(const QString &captureLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("DesktopCapture.captureLib", captureLib);
    config.endGroup();
}

void MediaSource::saveMultiSrcCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("MultiSrc.codecLib", codecLib);
    config.endGroup();
}

void MediaSource::saveProperties()
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("stream", this->d->m_stream);
    config.setValue("playOnStart", this->d->m_playOnStart);
    config.beginWriteArray("uris");

    int i = 0;

    for (auto it = this->d->m_uris.begin();
         it != this->d->m_uris.end();
         it++) {
        config.setArrayIndex(i);
        config.setValue("uri", it.key());
        config.setValue("description", it.value());
        i++;
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("Libraries");

    if (this->d->m_cameraCapture) {
        config.setValue("VideoCapture.codecLib", this->d->m_cameraCapture->property("codecLib"));
        config.setValue("VideoCapture.captureLib", this->d->m_cameraCapture->property("captureLib"));
    }

    if (this->d->m_desktopCapture)
        config.setValue("DesktopCapture.captureLib", this->d->m_desktopCapture->property("captureLib"));

    if (this->d->m_uriCapture)
        config.setValue("MultiSrc.codecLib", this->d->m_uriCapture->property("codecLib"));

    config.endGroup();
}

void MediaSource::webcamStreamsChanged(const QList<int> &streams)
{
    Q_UNUSED(streams);

    this->streamUpdated(this->d->m_stream);
}

#include "moc_mediasource.cpp"
