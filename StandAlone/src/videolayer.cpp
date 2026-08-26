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

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QSettings>
#include <QStandardPaths>
#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#include "videolayer.h"

struct VideoLayerSource
{
    QString device;
    AkElementPtr element;
    qint64 id {0};
    QRectF rect {0.0, 0.0, 1.0, 1.0};
    int zOrder {0};
    qreal opacity {1.0};
    qreal rotation {0.0};
    Qt::AspectRatioMode aspectRatioMode {Qt::KeepAspectRatio};
    bool enabled {true};
    VideoLayer::InputType type {VideoLayer::InputUnknown};
    AkAudioCaps audioCaps;
    AkVideoCaps videoCaps;
    QString label;
    QString lastError;
};

class VideoLayerPrivate
{
    public:
        VideoLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_availableCameras;
        QStringList m_availableScreens;
        QStringList m_availableWindows;
        QStringList m_supportedFileFormats;
        QStringList m_supportedImageFormats;
        QMap<QString, QString> m_formatsDescription;
        AkElement::ElementState m_globalState {AkElement::ElementStateNull};
        bool m_playOnStart {true};
        bool m_outputsAsInputs {false};
        bool m_firstRun {true};

        // Dedicated discovery instances
        AkElementPtr m_cameraDiscovery {akPluginManager->create<AkElement>("VideoSource/CameraCapture")};
        AkElementPtr m_screenDiscovery {akPluginManager->create<AkElement>("VideoSource/DesktopCapture")};

        QMap<qint64, VideoLayerSource> m_activeSources;

        explicit VideoLayerPrivate(VideoLayer *self);
        void connectDiscoverySignals();
        QStringList cameras() const;
        QStringList screens() const;
        QStringList windows() const;
        QString cameraDescription(const QString &camera) const;
        QString screenDescription(const QString &desktop) const;
        bool embedControls(const QString &where,
                           const AkElementPtr &element,
                           const QString &pluginId,
                           const QString &name) const;
        void loadProperties();
        void trySetupFirstRunDevice();
        void saveSources();
        void savePlayOnStart(bool playOnStart);
        void saveOutputsAsInputs(bool outputsAsInputs);
        AkElementPtr createCaptureElement(VideoLayer::InputType type) const;
        QString pluginIdForType(VideoLayer::InputType type) const;
        VideoLayer::InputType classifyByExtension(const QString &device) const;
        void pruneDisconnectedSources();
};

VideoLayer::VideoLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerPrivate(this);
    this->setQmlEngine(engine);
    this->d->connectDiscoverySignals();
    this->d->loadProperties();
}

VideoLayer::~VideoLayer()
{
    this->setState(AkElement::ElementStateNull);

    for (auto &source: this->d->m_activeSources)
        if (source.element)
            source.element->setState(AkElement::ElementStateNull);

    this->d->m_activeSources.clear();
    delete this->d;
}

QStringList VideoLayer::videoSourceFileFilters() const
{
    QStringList filters;

    /* Android's file selection dialog seems to be ignoring the file filters,
     * so allow selecting any type of file.
     */
#ifndef Q_OS_ANDROID
    //  Alternative extension names
    static const QMap<QString, QString> videoLayerFormatsMapping {
        {"jp2" , "jpg" },
        {"jpeg", "jpg" },
        {"svgz", "svg" },
        {"tif" , "tiff"},
        {"m4v" , "mp4" },
        {"mpeg", "mpg" },
    };

    QString extensions =
            "*." + this->d->m_supportedFileFormats.join(" *.");

    filters << tr("All Image and Video Files")
               + QString(" (%1)").arg(extensions);

    QStringList formats;

    for (auto &format: this->d->m_supportedFileFormats) {
        QString fmt;

        if (videoLayerFormatsMapping.contains(format))
            fmt = videoLayerFormatsMapping[format];
        else
            fmt = format;

        if (!formats.contains(fmt))
            formats << fmt;
    }

    QStringList fileFilters;

    for (auto &format: formats) {
        QString filter;
        QStringList extensions = QStringList {format}
                                 + videoLayerFormatsMapping.keys(format);
        QString extensionsFilter = "*." + extensions.join(" *.");

        if (this->d->m_formatsDescription.contains(format))
            filter = format.toUpper() + " - " + this->d->m_formatsDescription[format];
        else
            filter = format.toUpper();

        fileFilters << filter + QString(" (%1)").arg(extensionsFilter);
    }

    fileFilters.sort();
    filters << fileFilters;
#endif

    filters << tr("All Files") + " (*)";

    return filters;
}

QStringList VideoLayer::cameras() const
{
    return this->d->m_availableCameras;
}

QStringList VideoLayer::screens() const
{
    return this->d->m_availableScreens;
}

QStringList VideoLayer::windows() const
{
    return this->d->m_availableWindows;
}

bool VideoLayer::canCaptureWindows() const
{
    if (!this->d->m_screenDiscovery)
        return false;

    return this->d->m_screenDiscovery->property("canCaptureWindows").toBool();
}

QStringList VideoLayer::supportedFileFormats() const
{
    return this->d->m_supportedFileFormats;
}

AkElement::ElementState VideoLayer::state() const
{
    return this->d->m_globalState;
}

bool VideoLayer::playOnStart() const
{
    return this->d->m_playOnStart;
}

bool VideoLayer::outputsAsInputs() const
{
    return this->d->m_outputsAsInputs;
}

VideoLayer::InputType VideoLayer::deviceType(const QString &device) const
{
    if (this->d->m_availableCameras.contains(device))
        return InputCamera;

    if (this->d->m_availableScreens.contains(device)
        || this->d->m_availableWindows.contains(device))
        return InputScreen;

    for (const auto &source: this->d->m_activeSources)
        if (source.device == device)
            return source.type;

    return this->d->classifyByExtension(device);
}

VideoLayer::InputType VideoLayer::deviceType(qint64 id) const
{
    for (const auto &source: this->d->m_activeSources)
        if (source.id == id)
            return source.type;

    return InputUnknown;
}

QStringList VideoLayer::devicesByType(InputType type) const
{
    switch (type) {
    case InputCamera:
        return this->d->m_availableCameras;

    case InputScreen:
        return this->d->m_availableScreens + this->d->m_availableWindows;

    case InputImage:
    case InputStream: {
        QStringList devices;

        for (const auto &source: this->d->m_activeSources)
            if (source.type == type)
                devices << source.device;

        return devices;
    }

    default:
        break;
    }

    return {};
}

QString VideoLayer::description(const QString &device) const
{
    auto type = this->deviceType(device);

    switch (type) {
    case InputCamera:
        return this->d->cameraDescription(device);

    case InputScreen:
        return this->d->screenDescription(device);

    case InputImage:
    case InputStream: {
        for (const auto &source: this->d->m_activeSources)
            if (source.device == device && !source.label.isEmpty())
                return source.label;

        return QFileInfo(device).fileName();
    }

    default:
        break;
    }

    return {};
}

bool VideoLayer::embedControls(const QString &where,
                               qint64 id,
                               const QString &name) const
{
    if (!this->d->m_activeSources.contains(id))
        return false;

    auto &source = this->d->m_activeSources[id];
    auto pluginId = this->d->pluginIdForType(source.type);

    return this->d->embedControls(where, source.element, pluginId, name);
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

        for (auto &child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);
            delete child;
        }
    }
}

QVariantList VideoLayer::sourceIds() const
{
    QVariantList ids;

    for (auto it = this->d->m_activeSources.keyBegin();
         it != this->d->m_activeSources.keyEnd();
         ++it)
        ids << QVariant::fromValue(*it);

    return ids;
}

QString VideoLayer::sourceDevice(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    return this->d->m_activeSources[id].device;
}

QString VideoLayer::sourceLabel(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    auto &source = this->d->m_activeSources[id];

    switch (source.type) {
    case InputCamera:
        return this->d->cameraDescription(source.device);

    case InputScreen:
        return this->d->screenDescription(source.device);

    default:
        break;
    }

    return source.label;
}

bool VideoLayer::sourceEnabled(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return false;

    return this->d->m_activeSources[id].enabled;
}

QRectF VideoLayer::sourceRect(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    return this->d->m_activeSources[id].rect;
}

int VideoLayer::sourceZOrder(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return 0;

    return this->d->m_activeSources[id].zOrder;
}

qreal VideoLayer::sourceOpacity(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return 0.0;

    return this->d->m_activeSources[id].opacity;
}

qreal VideoLayer::sourceRotation(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return 0.0;

    return this->d->m_activeSources[id].rotation;
}

Qt::AspectRatioMode VideoLayer::sourceAspectRatioMode(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return Qt::KeepAspectRatio;

    return this->d->m_activeSources[id].aspectRatioMode;
}

AkAudioCaps VideoLayer::sourceAudioCaps(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    return this->d->m_activeSources[id].audioCaps;
}

AkVideoCaps VideoLayer::sourceVideoCaps(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    return this->d->m_activeSources[id].videoCaps;
}

QString VideoLayer::sourceError(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return {};

    // "error" only exists as a signal (error(QString)) on the capture
    // elements, not a queryable getter -- we cache the last value we
    // received from it instead of trying to invoke a nonexistent method.
    return this->d->m_activeSources[id].lastError;
}

VideoLayer::InputType VideoLayer::sourceType(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return InputUnknown;

    return this->d->m_activeSources[id].type;
}

bool VideoLayer::isTorchSupported(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return false;

    auto &source = this->d->m_activeSources[id];

    if (source.type != InputCamera || !source.element)
        return false;

    return source.element->property("isTorchSupported").toBool();
}

VideoLayer::TorchMode VideoLayer::torchMode(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return Torch_Off;

    auto &source = this->d->m_activeSources[id];

    if (source.type != InputCamera || !source.element)
        return Torch_Off;

    return source.element->property("torchMode").value<TorchMode>();
}

VideoLayer::PermissionStatus VideoLayer::cameraPermissionStatus(qint64 id) const
{
    if (!this->d->m_activeSources.contains(id))
        return PermissionStatus_Granted;

    auto &source = this->d->m_activeSources[id];

    if (source.type != InputCamera || !source.element)
        return PermissionStatus_Granted;

    return source.element->property("permissionStatus").value<PermissionStatus>();
}

qint64 VideoLayer::addSource(const QString &device)
{
    for (const auto &source: this->d->m_activeSources)
        if (source.device == device)
            return source.id;

    auto type = this->deviceType(device);

    if (type == InputUnknown) {
        qWarning() << "Unknown input type for" << device;

        return -1;
    }

    auto element = this->d->createCaptureElement(type);

    if (!element) {
        qWarning() << "Failed to create capture element for" << device;

        return -1;
    }

    if (type == InputStream)
        element->setProperty("loop", true);

    element->setProperty("media", device);

    qint64 id = Ak::id();

    VideoLayerSource source;
    source.device = device;
    source.element = element;
    source.id = id;
    source.type = type;
    source.enabled = true;

    // Tag every outgoing packet with this source's id before forwarding.
    QObject::connect(element.data(), &AkElement::oStream,
                     this, [this, id](const AkPacket &packet) {
                         auto pkt = packet;
                         pkt.setId(id);
                         emit this->oStream(pkt);
                     }, Qt::DirectConnection);

    element->setProperty("__sourceId", id);

    if (type == InputCamera)
        QObject::connect(element.data(),
                         SIGNAL(errorChanged(QString)),
                         this,
                         SLOT(handleSourceError(QString)));
    else
        QObject::connect(element.data(),
                         SIGNAL(error(QString)),
                         this,
                         SLOT(handleSourceError(QString)));

    QObject::connect(element.data(),
                     SIGNAL(streamsChanged(QList<int>)),
                     this,
                     SLOT(handleStreamsChanged(QList<int>)));

    if (type == InputCamera) {
        QObject::connect(element.data(),
                         SIGNAL(isTorchSupportedChanged(bool)),
                         this,
                         SLOT(handleIsTorchSupportedChanged(bool)));
        QObject::connect(element.data(),
                         SIGNAL(torchModeChanged(TorchMode)),
                         this,
                         SLOT(handleTorchModeChanged(TorchMode)));
        QObject::connect(element.data(),
                         SIGNAL(permissionStatusChanged(PermissionStatus)),
                         this,
                         SLOT(handlePermissionStatusChanged(PermissionStatus)));
    }

    source.zOrder = (int) this->d->m_activeSources.count();
    this->d->m_activeSources[id] = source;

    this->d->saveSources();
    emit this->sourceAdded(id, device);

    if (this->d->m_globalState == AkElement::ElementStatePlaying)
        element->setState(AkElement::ElementStatePlaying);

    return id;
}

void VideoLayer::removeSource(qint64 id)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto source = this->d->m_activeSources.take(id);

    if (source.element) {
        source.element->setState(AkElement::ElementStateNull);
        QObject::disconnect(source.element.data(), nullptr, this, nullptr);
    }

    QVector<QPair<qint64, int>> ordered;

    for (auto it = this->d->m_activeSources.begin();
         it != this->d->m_activeSources.end();
         ++it)
        ordered.append({it.key(), it.value().zOrder});

    std::sort(ordered.begin(), ordered.end(),
              [](const auto &a, const auto &b) {
                  return a.second < b.second;
              });

    for (int i = 0; i < ordered.size(); i++) {
        auto &src = this->d->m_activeSources[ordered[i].first];

        if (src.zOrder != i) {
            src.zOrder = i;
            emit this->sourceZOrderChanged(ordered[i].first, i);
        }
    }

    this->d->saveSources();
    emit this->sourceRemoved(id);
}

void VideoLayer::setSourceEnabled(qint64 id, bool enabled)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.enabled == enabled)
        return;

    source.enabled = enabled;
    this->d->saveSources();
    emit this->sourceEnabledChanged(id, enabled);

    if (source.element) {
        if (enabled && this->d->m_globalState == AkElement::ElementStatePlaying)
            auto ok = source.element->setState(AkElement::ElementStatePlaying);
        else if (!enabled)
            source.element->setState(AkElement::ElementStateNull);
    }
}

void VideoLayer::setSourceLabel(qint64 id, const QString &label)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.type != InputImage && source.type != InputStream)
        return;

    if (source.label == label)
        return;

    source.label = label;
    this->d->saveSources();
    emit this->sourceLabelChanged(id, label);
}

void VideoLayer::setSourceRect(qint64 id, const QRectF &rect)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.rect == rect)
        return;

    source.rect = rect;
    this->d->saveSources();
    emit this->sourceRectChanged(id, rect);
}

void VideoLayer::setSourceZOrder(qint64 id, int zOrder)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.zOrder == zOrder)
        return;

    int oldZOrder = source.zOrder;
    int n = this->d->m_activeSources.count();
    zOrder = qBound(0, zOrder, n - 1);

    for (auto it = this->d->m_activeSources.begin();
         it != this->d->m_activeSources.end();
         ++it) {
        if (it.key() == id)
            continue;

        int z = it.value().zOrder;

        if (oldZOrder < zOrder) {
            if (z > oldZOrder && z <= zOrder) {
                it.value().zOrder = z - 1;
                emit this->sourceZOrderChanged(it.key(), z - 1);
            }
        } else {
            if (z >= zOrder && z < oldZOrder) {
                it.value().zOrder = z + 1;
                emit this->sourceZOrderChanged(it.key(), z + 1);
            }
        }
    }

    source.zOrder = zOrder;
    this->d->saveSources();
    emit this->sourceZOrderChanged(id, zOrder);
}

void VideoLayer::setSourceOpacity(qint64 id, qreal opacity)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (qFuzzyCompare(source.opacity, opacity))
        return;

    source.opacity = opacity;
    this->d->saveSources();
    emit this->sourceOpacityChanged(id, opacity);
}

void VideoLayer::setSourceRotation(qint64 id, qreal rotation)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (qFuzzyCompare(source.rotation, rotation))
        return;

    source.rotation = rotation;
    this->d->saveSources();
    emit this->sourceRotationChanged(id, rotation);
}

void VideoLayer::setSourceAspectRatioMode(qint64 id, Qt::AspectRatioMode mode)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.aspectRatioMode == mode)
        return;

    source.aspectRatioMode = mode;
    this->d->saveSources();
    emit this->sourceAspectRatioModeChanged(id, mode);
}

void VideoLayer::setState(AkElement::ElementState state)
{
    if (this->d->m_globalState == state)
        return;

    this->d->m_globalState = state;
    emit this->stateChanged(state);

    for (auto &source: this->d->m_activeSources) {
        if (!source.element)
            continue;

        if (state == AkElement::ElementStatePlaying) {
            if (source.enabled)
                auto ok = source.element->setState(state);
        } else {
            source.element->setState(state);
        }
    }
}

void VideoLayer::setTorchMode(qint64 id, TorchMode mode)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (source.type != InputCamera || !source.element)
        return;

    source.element->setProperty("torchMode", mode);
}

void VideoLayer::setPlayOnStart(bool playOnStart)
{
    if (this->d->m_playOnStart == playOnStart)
        return;

    this->d->m_playOnStart = playOnStart;
    emit this->playOnStartChanged(playOnStart);
    this->d->savePlayOnStart(playOnStart);
}

void VideoLayer::setOutputsAsInputs(bool outputsAsInputs)
{
    if (this->d->m_outputsAsInputs == outputsAsInputs)
        return;

    this->d->m_outputsAsInputs = outputsAsInputs;
    emit this->outputsAsInputsChanged(this->d->m_outputsAsInputs);
    this->d->saveOutputsAsInputs(outputsAsInputs);
}

void VideoLayer::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoLayer::resetTorchMode(qint64 id)
{
    this->setTorchMode(id, Torch_Off);
}

void VideoLayer::resetPlayOnStart()
{
    this->setPlayOnStart(true);
}

void VideoLayer::resetOutputsAsInputs()
{
    this->setOutputsAsInputs(false);
}

void VideoLayer::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        qRegisterMetaType<InputType>("VideoInputType");
        qRegisterMetaType<TorchMode>("TorchMode");
        qRegisterMetaType<PermissionStatus>("PermissionStatus");
        qmlRegisterType<VideoLayer>("Webcamoid", 1, 0, "VideoLayer");
        engine->rootContext()->setContextProperty("videoLayer", this);
    }
}

void VideoLayer::updateInputs()
{
    if (this->d->m_cameraDiscovery)
        QMetaObject::invokeMethod(this->d->m_cameraDiscovery.data(),
                                  "updateDevices");

    if (this->d->m_screenDiscovery)
        QMetaObject::invokeMethod(this->d->m_screenDiscovery.data(),
                                  "updateDevices");

    this->handleDevicesChanged();
}

void VideoLayer::handleDevicesChanged()
{
    auto availableCameras = this->d->cameras();
    bool emitCamerasChanged = false;

    if (availableCameras != this->d->m_availableCameras) {
        this->d->m_availableCameras = availableCameras;
        emitCamerasChanged = true;
    }

    auto availableScreens = this->d->screens();
    bool emitScreensChanged = false;

    if (availableScreens != this->d->m_availableScreens) {
        this->d->m_availableScreens = availableScreens;
        emitScreensChanged = true;
    }

    auto availableWindows = this->d->windows();
    bool emitWindowsChanged = false;

    if (availableWindows != this->d->m_availableWindows) {
        this->d->m_availableWindows = availableWindows;
        emitWindowsChanged = true;
    }

    this->d->pruneDisconnectedSources();

    if (emitCamerasChanged)
        emit this->camerasChanged(this->d->m_availableCameras);

    if (emitScreensChanged)
        emit this->screensChanged(this->d->m_availableScreens);

    if (emitWindowsChanged)
        emit this->windowsChanged(this->d->m_availableWindows);

    if (this->d->m_firstRun)
        this->d->trySetupFirstRunDevice();
}

void VideoLayer::updateSourceCaps(qint64 id)
{
    if (!this->d->m_activeSources.contains(id))
        return;

    auto &source = this->d->m_activeSources[id];

    if (!source.element)
        return;

    AkCaps audioCaps;
    AkCaps videoCaps;

    QList<int> streams;
    QMetaObject::invokeMethod(source.element.data(),
                              "streams",
                              Q_RETURN_ARG(QList<int>, streams));

    if (streams.isEmpty()) {
        int audioStream = -1;
        int videoStream = -1;

        QMetaObject::invokeMethod(source.element.data(),
                                  "defaultStream",
                                  Q_RETURN_ARG(int, audioStream),
                                  Q_ARG(AkCaps::CapsType, AkCaps::CapsAudio));
        QMetaObject::invokeMethod(source.element.data(),
                                  "defaultStream",
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(AkCaps::CapsType, AkCaps::CapsVideo));

        if (audioStream >= 0)
            QMetaObject::invokeMethod(source.element.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, audioCaps),
                                      Q_ARG(int, audioStream));

        if (videoStream >= 0)
            QMetaObject::invokeMethod(source.element.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, videoCaps),
                                      Q_ARG(int, videoStream));
    } else {
        for (auto &stream: streams) {
            AkCaps caps;
            QMetaObject::invokeMethod(source.element.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, caps),
                                      Q_ARG(int, stream));

            if (caps.type() == AkCaps::CapsAudio)
                audioCaps = caps;
            else if (caps.type() == AkCaps::CapsVideo)
                videoCaps = caps;
        }
    }

    AkAudioCaps newAudioCaps(audioCaps);
    AkVideoCaps newVideoCaps(videoCaps);

    if (source.audioCaps != newAudioCaps) {
        source.audioCaps = newAudioCaps;
        emit this->sourceAudioCapsChanged(id, newAudioCaps);
    }

    if (source.videoCaps != newVideoCaps) {
        source.videoCaps = newVideoCaps;
        emit this->sourceVideoCapsChanged(id, newVideoCaps);
    }
}

void VideoLayer::handleSourceError(const QString &error)
{
    auto id = this->sender()->property("__sourceId").toLongLong();

    if (!this->d->m_activeSources.contains(id))
        return;

    this->d->m_activeSources[id].lastError = error;
    emit this->sourceErrorChanged(id, error);
}

void VideoLayer::handleStreamsChanged(const QList<int> &streams)
{
    Q_UNUSED(streams)
    auto id = this->sender()->property("__sourceId").toLongLong();
    this->updateSourceCaps(id);
}

void VideoLayer::handleIsTorchSupportedChanged(bool torchSupported)
{
    auto id = this->sender()->property("__sourceId").toLongLong();
    emit this->isTorchSupportedChanged(id, torchSupported);
}

void VideoLayer::handleTorchModeChanged(TorchMode mode)
{
    auto id = this->sender()->property("__sourceId").toLongLong();
    emit this->torchModeChanged(id, mode);
}

void VideoLayer::handlePermissionStatusChanged(PermissionStatus status)
{
    auto id = this->sender()->property("__sourceId").toLongLong();
    emit this->cameraPermissionStatusChanged(id, status);
}

VideoLayerPrivate::VideoLayerPrivate(VideoLayer *self):
    self(self)
{
    this->m_formatsDescription = {
        {"3gp" , QObject::tr("3GP Video")                       },
        {"avi" , QObject::tr("AVI Video")                       },
        {"bmp" , QObject::tr("Windows Bitmap")                  },
        {"cur" , QObject::tr("Microsoft Windows Cursor")        },
        //: Adobe FLV Flash video
        {"flv" , QObject::tr("Flash Video")                     },
        {"gif" , QObject::tr("Animated GIF")                    },
        {"gif" , QObject::tr("Graphic Interchange Format")      },
        {"icns", QObject::tr("Apple Icon Image")                },
        {"ico" , QObject::tr("Microsoft Windows Icon")          },
        {"jpg" , QObject::tr("Joint Photographic Experts Group")},
        {"mkv" , QObject::tr("MKV Video")                       },
        {"mng" , QObject::tr("Animated PNG")                    },
        {"mng" , QObject::tr("Multiple-image Network Graphics") },
        {"mov" , QObject::tr("QuickTime Video")                 },
        {"mp4" , QObject::tr("MP4 Video")                       },
        {"mpg" , QObject::tr("MPEG Video")                      },
        {"ogg" , QObject::tr("Ogg Video")                       },
        {"pbm" , QObject::tr("Portable Bitmap")                 },
        {"pgm" , QObject::tr("Portable Graymap")                },
        {"png" , QObject::tr("Portable Network Graphics")       },
        {"ppm" , QObject::tr("Portable Pixmap")                 },
        //: Don't translate "RealMedia", leave it as is.
        {"rm"  , QObject::tr("RealMedia Video")                 },
        {"svg" , QObject::tr("Scalable Vector Graphics")        },
        {"tga" , QObject::tr("Truevision TGA")                  },
        {"tiff", QObject::tr("Tagged Image File Format")        },
        {"vob" , QObject::tr("DVD Video")                       },
        {"wbmp", QObject::tr("Wireless Bitmap")                 },
        {"webm", QObject::tr("WebM Video")                      },
        {"webp", QObject::tr("WebP")                            },
        //: Also known as WMV, is a video file format.
        {"wmv" , QObject::tr("Windows Media Video")             },
        {"xbm" , QObject::tr("X11 Bitmap")                      },
        {"xpm" , QObject::tr("X11 Pixmap")                      },
    };

    static const QStringList supportedVideoFormats {
        "3gp",
        "avi",
        "flv",
        "gif",
        "mkv",
        "mng",
        "mov",
        "mp4",
        "m4v",
        "mpg",
        "mpeg",
        "ogg",
        "rm",
        "vob",
        "webm",
        "wmv"
    };

    auto supportedImageFormats = QImageReader::supportedImageFormats();
    supportedImageFormats.removeAll("pdf");
    this->m_supportedImageFormats =
            QStringList(supportedImageFormats.begin(),
                       supportedImageFormats.end());
    this->m_supportedFileFormats =
        supportedVideoFormats + this->m_supportedImageFormats;

    this->m_availableCameras = this->cameras();
    this->m_availableScreens = this->screens();
    this->m_availableWindows = this->windows();
}

void VideoLayerPrivate::connectDiscoverySignals()
{
    if (this->m_cameraDiscovery) {
        QObject::connect(this->m_cameraDiscovery.data(),
                         SIGNAL(mediasChanged(QStringList)),
                         self,
                         SLOT(handleDevicesChanged()));
    }

    if (this->m_screenDiscovery) {
        QObject::connect(this->m_screenDiscovery.data(),
                         SIGNAL(mediasChanged(QStringList)),
                         self,
                         SLOT(handleDevicesChanged()));
        QObject::connect(this->m_screenDiscovery.data(),
                         SIGNAL(canCaptureWindowsChanged(bool)),
                         self,
                         SIGNAL(canCaptureWindowsChanged(bool)));
    }
}

QStringList VideoLayerPrivate::cameras() const
{
    if (!this->m_cameraDiscovery)
        return {};

    QStringList cameras;
    QMetaObject::invokeMethod(this->m_cameraDiscovery.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, cameras));

    return cameras;
}

QStringList VideoLayerPrivate::screens() const
{
    if (!this->m_screenDiscovery)
        return {};

    QStringList screens;
    QStringList medias;
    QMetaObject::invokeMethod(this->m_screenDiscovery.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, medias));

    for (const auto &media: medias) {
        bool isWindow = false;
        QMetaObject::invokeMethod(this->m_screenDiscovery.data(),
                                  "isWindow",
                                  Q_RETURN_ARG(bool, isWindow),
                                  Q_ARG(QString, media));

        if (!isWindow)
            screens << media;
    }

    return screens;
}

QStringList VideoLayerPrivate::windows() const
{
    if (!this->m_screenDiscovery)
        return {};

    QStringList windows;
    QStringList medias;
    QMetaObject::invokeMethod(this->m_screenDiscovery.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, medias));

    for (const auto &media: medias) {
        bool isWindow = false;
        QMetaObject::invokeMethod(this->m_screenDiscovery.data(),
                                  "isWindow",
                                  Q_RETURN_ARG(bool, isWindow),
                                  Q_ARG(QString, media));

        if (isWindow)
            windows << media;
    }

    return windows;
}

QString VideoLayerPrivate::cameraDescription(const QString &camera) const
{
    if (!this->m_cameraDiscovery)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_cameraDiscovery.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, camera));

    return description;
}

QString VideoLayerPrivate::screenDescription(const QString &desktop) const
{
    if (!this->m_screenDiscovery)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_screenDiscovery.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, desktop));

    return description;
}

bool VideoLayerPrivate::embedControls(const QString &where,
                                      const AkElementPtr &element,
                                      const QString &pluginId,
                                      const QString &name) const
{
    if (!element || !this->m_engine)
        return false;

    auto controlInterface = element->controlInterface(this->m_engine, pluginId);

    if (!controlInterface)
        return false;

    if (!name.isEmpty())
        controlInterface->setObjectName(name);

    for (auto &obj: this->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto interfaceItem = qobject_cast<QQuickItem *>(controlInterface);

        if (!interfaceItem)
            continue;

        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

void VideoLayerPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("StreamConfigs");
    this->m_playOnStart = config.value("playOnStart", true).toBool();
    this->m_outputsAsInputs = config.value("outputsAsInputs", false).toBool();
    config.endGroup();

    config.beginGroup("GeneralConfigs");
    this->m_firstRun = config.value("firstRun", true).toBool();
    config.endGroup();

    self->updateInputs();

    config.beginGroup("StreamConfigs");
    int size = config.beginReadArray("sources");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        auto device = config.value("source").toString();
        auto label = config.value("description").toString();
        auto enabled = config.value("enabled", true).toBool();
        auto rect = config.value("rect", QRectF(0.0, 0.0, 1.0, 1.0)).toRectF();
        auto zOrder = config.value("zorder", i).toInt();
        auto opacity = config.value("opacity", 1.0).toReal();
        auto rotation = config.value("rotation", 0.0).toReal();
        auto aspectRatioMode = config.value("aspectRatioMode", Qt::KeepAspectRatio).toInt();
        auto deviceType = self->deviceType(device);

        if (device.isEmpty())
            continue;

        auto id = self->addSource(device);

        if (id < 0)
            continue;

        self->setSourceEnabled(id, enabled);
        self->setSourceRect(id, rect);
        self->setSourceZOrder(id, zOrder);
        self->setSourceOpacity(id, opacity);
        self->setSourceRotation(id, rotation);
        self->setSourceAspectRatioMode(id, static_cast<Qt::AspectRatioMode>(aspectRatioMode));

        if (label.isEmpty()
            && (deviceType == VideoLayer::InputImage
                || deviceType == VideoLayer::InputStream)) {
            label = QFileInfo(device).baseName();
        }

        self->setSourceLabel(id, label);
    }

    config.endArray();
    config.endGroup();

    if (this->m_firstRun)
        this->trySetupFirstRunDevice();
}

void VideoLayerPrivate::trySetupFirstRunDevice()
{
    if (!this->m_firstRun)
        return;

    auto availableCameras = this->cameras();
    QString device;

    if (!availableCameras.isEmpty()) {
        device = availableCameras.first();
    } else {
        auto availableScreens = this->screens();

        if (!availableScreens.isEmpty())
            device = availableScreens.first();
    }

    if (device.isEmpty())
        return;

    // Only add if nothing was already restored/added for this device.
    bool alreadyActive = false;

    for (const auto &source: this->m_activeSources)
        if (source.device == device) {
            alreadyActive = true;

            break;
        }

    if (!alreadyActive) {
        auto id = self->addSource(device);

        if (id >= 0)
            self->setSourceEnabled(id, true);
    }

    this->m_firstRun = false;

    QSettings config;
    config.beginGroup("GeneralConfigs");
    config.setValue("firstRun", false);
    config.endGroup();
}

void VideoLayerPrivate::saveSources()
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.beginWriteArray("sources");

    int i = 0;

    for (const auto &source: this->m_activeSources) {
        config.setArrayIndex(i);
        config.setValue("source", source.device);
        config.setValue("description", source.label);
        config.setValue("enabled", source.enabled);
        config.setValue("rect", source.rect);
        config.setValue("zorder", source.zOrder);
        config.setValue("opacity", source.opacity);
        config.setValue("rotation", source.rotation);
        config.setValue("aspectRatioMode", source.aspectRatioMode);
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

void VideoLayerPrivate::saveOutputsAsInputs(bool outputsAsInputs)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("outputsAsInputs", outputsAsInputs);
    config.endGroup();
}

AkElementPtr VideoLayerPrivate::createCaptureElement(VideoLayer::InputType type) const
{
    QString pluginId = this->pluginIdForType(type);

    if (pluginId.isEmpty())
        return {};

    return akPluginManager->create<AkElement>(pluginId);
}

QString VideoLayerPrivate::pluginIdForType(VideoLayer::InputType type) const
{
    switch (type) {
    case VideoLayer::InputCamera:
        return "VideoSource/CameraCapture";
    case VideoLayer::InputScreen:
        return "VideoSource/DesktopCapture";
    case VideoLayer::InputImage:
        return "VideoSource/ImageSrc";
    case VideoLayer::InputStream:
        return "MultimediaSource/MultiSrc";
    default:
        return {};
    }
}

VideoLayer::InputType VideoLayerPrivate::classifyByExtension(const QString &device) const
{
    auto suffix = QFileInfo(device).suffix().toLower();

    if (suffix.isEmpty())
        return VideoLayer::InputUnknown;

    if (this->m_supportedImageFormats.contains(suffix))
        return VideoLayer::InputImage;

    if (this->m_supportedFileFormats.contains(suffix))
        return VideoLayer::InputStream;

    return VideoLayer::InputUnknown;
}

void VideoLayerPrivate::pruneDisconnectedSources()
{
    // Create a list of all disconnected sources
    QList<qint64> sourcesToRemove;

    for (auto it = this->m_activeSources.begin(); it != this->m_activeSources.end(); ++it) {
        const auto &source = it.value();
        bool shouldRemove = false;

        if (source.type == VideoLayer::InputCamera) {
            // Remove unnavailable cameras
            if (!this->m_availableCameras.contains(source.device))
                shouldRemove = true;
        } else if (source.type == VideoLayer::InputScreen) {
            // Remove unnavailable screens
            if (!this->m_availableScreens.contains(source.device)
                && !this->m_availableWindows.contains(source.device))
                shouldRemove = true;
        }

        if (shouldRemove)
            sourcesToRemove.append(it.key());
    }

    // Remove all disconnected sources
    for (auto id: sourcesToRemove)
        self->removeSource(id);
}

#include "moc_videolayer.cpp"
