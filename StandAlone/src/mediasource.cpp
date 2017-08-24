/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <QQuickItem>
#include <QQmlProperty>

#include "mediasource.h"

MediaSource::MediaSource(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->m_inputState = AkElement::ElementStateNull;
    this->m_playOnStart = false;
    this->setQmlEngine(engine);
    this->m_pipeline = AkElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/sourcespipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_cameraCapture),
                                  Q_ARG(QString, "cameraCapture"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_desktopCapture),
                                  Q_ARG(QString, "desktopCapture"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_uriCapture),
                                  Q_ARG(QString, "uriCapture"));

        QObject::connect(this->m_pipeline.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
    }

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

    if (this->m_cameraCapture) {
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateStreams()));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCodecLib(const QString &)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveVideoCaptureCaptureLib(const QString &)));
    }

    if (this->m_desktopCapture) {
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateStreams()));
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->m_desktopCapture.data(),
                         SIGNAL(captureLibChanged(const QString &)),
                         this,
                         SLOT(saveDesktopCaptureCaptureLib(const QString &)));
    }

    if (this->m_uriCapture) {
        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveMultiSrcCodecLib(const QString &)));
    }

    this->m_syphonCapture = AkElement::create("SyphonIO");

    if (this->m_syphonCapture) {
        QObject::connect(this->m_syphonCapture.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
        QObject::connect(this->m_syphonCapture.data(),
                         SIGNAL(mediasChanged(const QStringList &)),
                         this,
                         SLOT(updateStreams()));
        QObject::connect(this->m_syphonCapture.data(),
                         SIGNAL(error(const QString &)),
                         this,
                         SIGNAL(error(const QString &)));
    }

    // Setup streams
    QStringList cameras;

    if (this->m_cameraCapture)
        QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, cameras));

    QStringList desktops;

    if (this->m_desktopCapture)
        QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, desktops));

    QStringList syphonServers;

    if (this->m_syphonCapture)
        QMetaObject::invokeMethod(this->m_syphonCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, syphonServers));

    for (const QString &camera: cameras) {
        QString description;

        QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, camera));

        this->m_descriptions[camera] = description;
    }

    for (const QString &desktop: desktops) {
        QString description;

        QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, desktop));

        this->m_descriptions[desktop] = description;
    }

    if (this->m_syphonCapture)
        for (const QString &server: syphonServers) {
            QString description;

            QMetaObject::invokeMethod(this->m_syphonCapture.data(),
                                      "description",
                                      Q_RETURN_ARG(QString, description),
                                      Q_ARG(QString, server));

            this->m_descriptions[server] = description;
        }

    this->m_cameras = cameras;
    this->m_desktops = desktops;
    this->m_syphonServers = syphonServers;
    this->m_streams = cameras + syphonServers + desktops;

    this->loadProperties();
}

MediaSource::~MediaSource()
{
    this->setState(AkElement::ElementStateNull);
    this->saveProperties();
}

QString MediaSource::stream() const
{
    return this->m_stream;
}

QStringList MediaSource::streams() const
{
    return this->m_streams;
}

QStringList MediaSource::cameras() const
{
    return this->m_cameras;
}

QStringList MediaSource::syphonServers() const
{
    return this->m_syphonServers;
}

QStringList MediaSource::desktops() const
{
    return this->m_desktops;
}

QVariantMap MediaSource::uris() const
{
    return this->m_uris;
}

AkCaps MediaSource::audioCaps() const
{
    return this->m_audioCaps;
}

AkCaps MediaSource::videoCaps() const
{
    return this->m_videoCaps;
}

AkElement::ElementState MediaSource::state() const
{
    return this->m_inputState;
}

bool MediaSource::playOnStart() const
{
    return this->m_playOnStart;
}

QString MediaSource::description(const QString &stream) const
{
    return this->m_descriptions.value(stream);
}

bool MediaSource::embedControls(const QString &where,
                                const QString &stream,
                                const QString &name) const
{
    auto source = this->sourceElement(stream);

    if (!source)
        return false;

    auto interface = source->controlInterface(this->m_engine,
                                              source->pluginId());

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto obj: this->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);
        interfaceItem->setParent(item);

        QQmlProperty::write(interfaceItem,
                            "anchors.fill",
                            qVariantFromValue(item));

        return true;
    }

    return false;
}

void MediaSource::removeInterface(const QString &where) const
{
    if (!this->m_engine)
        return;

    for (const QObject *obj: this->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        QList<decltype(item)> childItems = item->childItems();

        for (auto child: childItems) {
            child->setParentItem(NULL);
            child->setParent(NULL);
            delete child;
        }
    }
}

AkElementPtr MediaSource::sourceElement(const QString &stream) const
{
    if (this->m_cameras.contains(stream))
        return this->m_cameraCapture;
    else if (this->m_desktops.contains(stream))
        return this->m_desktopCapture;
    else if (this->m_uris.contains(stream))
        return this->m_uriCapture;
    else if (this->m_syphonServers.contains(stream))
        return this->m_syphonCapture;

    return AkElementPtr();
}

void MediaSource::setStream(const QString &stream)
{
    if (this->m_stream == stream)
        return;

    this->m_stream = stream;
    emit this->streamChanged(stream);
    this->streamUpdated(stream);
}

void MediaSource::setUris(const QVariantMap &uris)
{
    if (this->m_uris == uris)
        return;

    this->m_uris = uris;
    emit this->urisChanged(uris);
}

void MediaSource::setState(AkElement::ElementState state)
{
    if (this->m_inputState == state)
        return;

    AkElementPtr source;

    if (this->m_cameras.contains(this->m_stream)) {
        if (this->m_desktopCapture)
            this->m_desktopCapture->setState(AkElement::ElementStateNull);

        if (this->m_uriCapture)
            this->m_uriCapture->setState(AkElement::ElementStateNull);

        if (this->m_syphonCapture)
            this->m_syphonCapture->setState(AkElement::ElementStateNull);

        source = this->m_cameraCapture;
    } else if (this->m_desktops.contains(this->m_stream)) {
        if (this->m_cameraCapture)
            this->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->m_uriCapture)
            this->m_uriCapture->setState(AkElement::ElementStateNull);

        if (this->m_syphonCapture)
            this->m_syphonCapture->setState(AkElement::ElementStateNull);

        source = this->m_desktopCapture;
    } else if (this->m_uris.contains(this->m_stream)) {
        if (this->m_cameraCapture)
            this->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->m_desktopCapture)
            this->m_desktopCapture->setState(AkElement::ElementStateNull);

        if (this->m_syphonCapture)
            this->m_syphonCapture->setState(AkElement::ElementStateNull);

        source = this->m_uriCapture;
    } else if (this->m_syphonServers.contains(this->m_stream)) {
        if (this->m_cameraCapture)
            this->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->m_desktopCapture)
            this->m_desktopCapture->setState(AkElement::ElementStateNull);

        if (this->m_uriCapture)
            this->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->m_syphonCapture;
    }

    if (source) {
        if (source->setState(state) || source->state() != this->m_inputState) {
            auto state = source->state();
            this->m_inputState = state;
            emit this->stateChanged(state);
        }
    } else {
        if (this->m_inputState != AkElement::ElementStateNull) {
            this->m_inputState = AkElement::ElementStateNull;
            emit this->stateChanged(AkElement::ElementStateNull);
        }
    }
}

void MediaSource::setPlayOnStart(bool playOnStart)
{
    if (this->m_playOnStart == playOnStart)
        return;

    this->m_playOnStart = playOnStart;
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
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("MediaSource", this);
}

void MediaSource::streamUpdated(const QString &stream)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);
    auto source = this->sourceElement(stream);

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

    if (this->m_cameraCapture)
        QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, cameras));

    QStringList desktops;

    if (this->m_desktopCapture)
        QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, desktops));

    QStringList syphonServers;

    if (this->m_syphonCapture)
        QMetaObject::invokeMethod(this->m_syphonCapture.data(),
                                  "medias",
                                  Q_RETURN_ARG(QStringList, syphonServers));

    this->m_descriptions.clear();

    for (const QString &camera: cameras) {
        QString description;

        QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, camera));

        this->m_descriptions[camera] = description;
    }

    for (const QString &desktop: desktops) {
        QString description;

        QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, desktop));

        this->m_descriptions[desktop] = description;
    }

    for (const QString &uri: this->m_uris.keys())
        this->m_descriptions[uri] = this->m_uris[uri].toString();

    for (const QString &server: syphonServers) {
        QString description;

        QMetaObject::invokeMethod(this->m_syphonCapture.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, server));

        this->m_descriptions[server] = description;
    }

    bool isSet = this->setCameras(cameras);
    isSet |= this->setSyphonServers(syphonServers);
    isSet |= this->setDesktops(desktops);
    isSet |= this->setStreams(cameras + syphonServers + desktops + this->m_uris.keys());

    if (!isSet)
        emit this->streamsChanged(this->m_streams);

    if (!this->m_streams.contains(this->m_stream)) {
        if (this->m_streams.isEmpty())
            this->resetStream();
        else
            this->setStream(this->m_streams.first());
    }
}

bool MediaSource::setStreams(const QStringList &streams)
{
    if (this->m_streams == streams)
        return false;

    this->m_streams = streams;
    emit this->streamsChanged(streams);

    return true;
}

bool MediaSource::setCameras(const QStringList &cameras)
{
    if (this->m_cameras == cameras)
        return false;

    this->m_cameras = cameras;
    emit this->camerasChanged(cameras);

    return true;
}

bool MediaSource::setSyphonServers(const QStringList &servers)
{
    if (this->m_syphonServers == servers)
        return false;

    this->m_syphonServers = servers;
    emit this->syphonServersChanged(servers);

    return true;
}

bool MediaSource::setDesktops(const QStringList &desktops)
{
    if (this->m_desktops == desktops)
        return false;

    this->m_desktops = desktops;
    emit this->desktopsChanged(desktops);

    return true;
}

void MediaSource::setAudioCaps(const AkCaps &audioCaps)
{
    if (this->m_audioCaps == audioCaps)
        return;

    this->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
}

void MediaSource::setVideoCaps(const AkCaps &videoCaps)
{
    if (this->m_videoCaps == videoCaps)
        return;

    this->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
}

void MediaSource::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");

    if (this->m_cameraCapture) {
        this->m_cameraCapture->setProperty("codecLib",
                                           config.value("VideoCapture.codecLib",
                                                        this->m_cameraCapture->property("codecLib")));
        this->m_cameraCapture->setProperty("captureLib",
                                           config.value("VideoCapture.captureLib",
                                                        this->m_cameraCapture->property("captureLib")));
    }

    if (this->m_desktopCapture)
        this->m_desktopCapture->setProperty("captureLib",
                                            config.value("DesktopCapture.captureLib",
                                                         this->m_cameraCapture->property("captureLib")));


    if (this->m_uriCapture)
        this->m_uriCapture->setProperty("codecLib",
                                        config.value("MultiSrc.codecLib",
                                                     this->m_uriCapture->property("codecLib")));

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

    if (this->m_streams.contains(stream))
        this->setStream(stream);
    else if (!this->m_streams.isEmpty())
        this->setStream(this->m_streams.first());
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

    for (const QString &uri: uris.keys()) {
        config.setArrayIndex(i);
        config.setValue("uri", uri);
        config.setValue("description", uris[uri]);
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
    config.setValue("stream", this->m_stream);
    config.setValue("playOnStart", this->m_playOnStart);
    config.beginWriteArray("uris");

    int i = 0;

    for (const QString &uri: this->m_uris.keys()) {
        config.setArrayIndex(i);
        config.setValue("uri", uri);
        config.setValue("description", this->m_uris[uri]);
        i++;
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("Libraries");

    if (this->m_cameraCapture) {
        config.setValue("VideoCapture.codecLib", this->m_cameraCapture->property("codecLib"));
        config.setValue("VideoCapture.captureLib", this->m_cameraCapture->property("captureLib"));
    }

    if (this->m_desktopCapture)
        config.setValue("DesktopCapture.captureLib", this->m_desktopCapture->property("captureLib"));

    if (this->m_uriCapture)
        config.setValue("MultiSrc.codecLib", this->m_uriCapture->property("codecLib"));

    config.endGroup();
}
