/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>

#include "mediatools.h"

typedef QMap<MediaTools::RecordFrom, QString> RecordFromMap;

inline RecordFromMap initRecordFromMap()
{
    RecordFromMap recordFromMap;
    recordFromMap[MediaTools::RecordFromNone] = "none";
    recordFromMap[MediaTools::RecordFromSource] = "source";
    recordFromMap[MediaTools::RecordFromMic] = "mic";

    return recordFromMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(RecordFromMap, recordFromMap, (initRecordFromMap()))

MediaTools::MediaTools(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->m_appEngine = engine;

    this->m_playAudioFromSource = true;
    this->m_recordAudioFrom = RecordFromMic;
    this->m_recording = false;
    this->m_windowWidth = 0;
    this->m_windowHeight = 0;
    this->m_advancedMode = false;
    this->m_enableVirtualCamera = false;

    Ak::setQmlEngine(engine);
    this->m_pipeline = AkElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/mainpipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_source),
                                  Q_ARG(QString, "source"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioSwitch),
                                  Q_ARG(QString, "audioSwitch"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioOutput),
                                  Q_ARG(QString, "audioOutput"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_mic),
                                  Q_ARG(QString, "mic"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_record),
                                  Q_ARG(QString, "record"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_videoCapture),
                                  Q_ARG(QString, "videoCapture"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_desktopCapture),
                                  Q_ARG(QString, "desktopCapture"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_videoMux),
                                  Q_ARG(QString, "videoMux"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_videoOutput),
                                  Q_ARG(QString, "videoOutput"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_videoGen),
                                  Q_ARG(QString, "videoGen"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_virtualCamera),
                                  Q_ARG(QString, "virtualCamera"));

        if (this->m_videoOutput)
            this->m_videoOutput->link(this);

        if (this->m_source) {
            QObject::connect(this->m_source.data(),
                             SIGNAL(error(const QString &)),
                             this,
                             SIGNAL(error(const QString &)));

            QObject::connect(this->m_source.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this,
                             SIGNAL(stateChanged(AkElement::ElementState)));

            QObject::connect(this->m_source.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this->m_audioOutput.data(),
                             SLOT(setState(AkElement::ElementState)));
        }

        if (this->m_videoCapture) {
            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(error(const QString &)),
                             this,
                             SIGNAL(error(const QString &)));

            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this,
                             SIGNAL(stateChanged(AkElement::ElementState)));
        }

        if (this->m_desktopCapture) {
            QObject::connect(this->m_desktopCapture.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this,
                             SIGNAL(stateChanged(AkElement::ElementState)));
        }

        QObject::connect(this,
                         &MediaTools::stateChanged,
                         this,
                         &MediaTools::isPlayingChanged);

        if (this->m_audioSwitch)
            this->m_audioSwitch->setProperty("inputIndex", 1);

        if (this->m_videoCapture)
            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(mediasChanged(const QStringList &)),
                             this,
                             SLOT(webcamsChanged(const QStringList &)));

        if (this->m_desktopCapture)
            QObject::connect(this->m_desktopCapture.data(),
                             SIGNAL(mediasChanged(const QStringList &)),
                             this,
                             SLOT(webcamsChanged(const QStringList &)));
    }

    if (this->m_record)
        QObject::connect(this->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SIGNAL(curRecordingFormatChanged(const QString &)));

    this->loadConfigs();
}

MediaTools::~MediaTools()
{
    this->stopRecording();
    this->stop();
    this->saveConfigs();
}

void MediaTools::iStream(const AkPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw")
        return;

    if (!this->sender())
        return;

    QString sender = this->sender()->objectName();

    if (sender == "videoOutput") {
        this->m_curPacket = packet;
        emit this->frameReady(packet);
    }
}

QString MediaTools::curStream() const
{
    return this->m_curStream;
}

bool MediaTools::playAudioFromSource() const
{
    return this->m_playAudioFromSource;
}

QString MediaTools::recordAudioFrom() const
{
    return recordFromMap->value(this->m_recordAudioFrom);
}

QString MediaTools::curRecordingFormat() const
{
    return this->m_record?
                this->m_record->property("outputFormat").toString():
                QString();
}

bool MediaTools::recording() const
{
    return this->m_recording;
}

QStringList MediaTools::recordingFormats() const
{
    QStringList formats;

    QStringList supportedFormats;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedFormats",
                              Q_RETURN_ARG(QStringList, supportedFormats));

    foreach (QString format, supportedFormats) {
        QStringList audioCodecs;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "supportedCodecs",
                                  Q_RETURN_ARG(QStringList, audioCodecs),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "audio/x-raw"));

        QStringList videoCodecs;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "supportedCodecs",
                                  Q_RETURN_ARG(QStringList, videoCodecs),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "video/x-raw"));

        QStringList extentions;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "fileExtensions",
                                  Q_RETURN_ARG(QStringList, extentions),
                                  Q_ARG(QString, format));

        if (!audioCodecs.isEmpty()
            && !videoCodecs.isEmpty()
            && !extentions.isEmpty())
            formats << format;
    }

    return formats;
}

QString MediaTools::recordingFormatDescription(const QString &formatId) const
{
    QString description;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "formatDescription",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, formatId));

    return description;
}

QStringList MediaTools::recordingFormatSuffix(const QString &formatId) const
{
    QStringList suffix;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "fileExtensions",
                              Q_RETURN_ARG(QStringList, suffix),
                              Q_ARG(QString, formatId));

    return suffix;
}

QStringList MediaTools::streams() const
{
    QStringList streams;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, streams));

    QStringList screens;

    QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, screens));

    streams << screens;

    return streams + this->m_streams.keys();
}

int MediaTools::windowWidth() const
{
    return this->m_windowWidth;
}

int MediaTools::windowHeight() const
{
    return this->m_windowHeight;
}

bool MediaTools::advancedMode() const
{
    return this->m_advancedMode;
}

bool MediaTools::enableVirtualCamera() const
{
    return this->m_enableVirtualCamera;
}

QString MediaTools::applicationName() const
{
    return QCoreApplication::applicationName();
}

QString MediaTools::applicationVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString MediaTools::qtVersion() const
{
    return QT_VERSION_STR;
}

QString MediaTools::copyrightNotice() const
{
    return COMMONS_COPYRIGHT_NOTICE;
}

QString MediaTools::projectUrl() const
{
    return COMMONS_PROJECT_URL;
}

QString MediaTools::projectLicenseUrl() const
{
    return COMMONS_PROJECT_LICENSE_URL;
}

QString MediaTools::streamDescription(const QString &stream) const
{
    QString description;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, stream));

    if (!description.isEmpty())
        return description;

    QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, stream));

    if (!description.isEmpty())
        return description;

    if (this->m_streams.contains(stream))
        return this->m_streams[stream];

    return QString();
}

bool MediaTools::canModify(const QString &stream) const
{
    if (this->m_streams.contains(stream))
        return true;

    return false;
}

bool MediaTools::isCamera(const QString &stream) const
{
    QStringList webcams;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, webcams));

    return webcams.contains(stream);
}

bool MediaTools::isDesktop(const QString &stream) const
{
    QStringList screens;

    QMetaObject::invokeMethod(this->m_desktopCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, screens));

    return screens.contains(stream);
}

bool MediaTools::isVideo(const QString &stream) const
{
    return this->m_streams.contains(stream);
}

QStringList MediaTools::availableEffects() const
{
    QStringList effects = AkElement::listPlugins("VideoFilter");

    if (this->m_advancedMode)
        foreach (AkElementPtr effect, this->m_effectsList) {
            int i = effects.indexOf(effect->pluginId());

            if (i < 0
                || effect->property("preview").toBool())
                continue;

            effects.removeAt(i);
        }

    qSort(effects.begin(), effects.end(), sortByDescription);

    return effects;
}

QVariantMap MediaTools::effectInfo(const QString &effectId) const
{
    return AkElement::pluginInfo(effectId);
}

QString MediaTools::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return QString();

    return AkElement::pluginInfo(effectId)["MetaData"].toMap()
                                          ["description"].toString();
}

QStringList MediaTools::currentEffects() const
{
    QStringList effects;

    foreach (AkElementPtr effect, this->m_effectsList)
        if (!effect->property("preview").toBool())
            effects << effect->pluginId();

    return effects;
}

AkElementPtr MediaTools::appendEffect(const QString &effectId, bool preview)
{
    int i = this->m_effectsList.size() - 1;
    AkElementPtr effect = AkElement::create(effectId);

    if (!effect)
        return AkElementPtr();

    if (preview)
        effect->setProperty("preview", preview);

    this->m_effectsList << effect;
    AkElementPtr source = this->sourceElement();
    bool playing = this->isPlaying();

    if (playing)
        source->setState(AkElement::ElementStatePaused);

    if (i < 0) {
        if (this->m_videoMux) {
            this->m_videoMux->unlink(this->m_videoOutput);
            this->m_videoMux->link(effect, Qt::DirectConnection);
        }
    } else {
        this->m_effectsList[i]->unlink(this->m_videoOutput);
        this->m_effectsList[i]->link(effect, Qt::DirectConnection);
    }

    effect->link(this->m_videoOutput, Qt::DirectConnection);

    if (playing)
        source->setState(AkElement::ElementStatePlaying);

    if (!preview)
        emit this->currentEffectsChanged();

    return effect;
}

void MediaTools::removeEffect(const QString &effectId)
{
    AkElementPtr source = this->sourceElement();
    bool playing = this->isPlaying();

    if (playing)
        source->setState(AkElement::ElementStatePaused);

    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            if (i == 0) {
                if (this->m_effectsList.size() == 1) {
                    if (this->m_videoMux)
                        this->m_videoMux->link(this->m_videoOutput, Qt::DirectConnection);
                }
                else
                    this->m_videoMux->link(this->m_effectsList[i + 1], Qt::DirectConnection);
            } else if (i == this->m_effectsList.size() - 1)
                this->m_effectsList[i - 1]->link(this->m_videoOutput, Qt::DirectConnection);
            else
                this->m_effectsList[i - 1]->link(this->m_effectsList[i + 1], Qt::DirectConnection);

            bool isPreview = this->m_effectsList[i]->property("preview").toBool();
            this->m_effectsList.removeAt(i);

            if (!isPreview)
                emit this->currentEffectsChanged();

            break;
        }

    if (playing)
        source->setState(AkElement::ElementStatePlaying);
}

void MediaTools::moveEffect(const QString &effectId, int index)
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            AkElementPtr effect = this->m_effectsList.takeAt(i);
            this->m_effectsList.insert(index, effect);

            break;
        }

    emit this->currentEffectsChanged();
}

void MediaTools::showPreview(const QString &effectId)
{
    this->removePreview();
    this->appendEffect(effectId, true);
}

void MediaTools::setAsPreview(const QString &effectId, bool preview)
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            this->m_effectsList[i]->setProperty("preview", preview);

            if (!preview)
                emit this->currentEffectsChanged();

            break;
        }
}

void MediaTools::removePreview(const QString &effectId)
{
    QList<AkElementPtr> effectsList = this->m_effectsList;

    foreach (AkElementPtr effect, effectsList)
        if (effect->property("preview").toBool()
            && (effectId.isEmpty()
                || effect->pluginId() == effectId)) {
            this->removeEffect(effect->pluginId());
        }
}

bool MediaTools::isPlaying()
{
    if (this->m_source
        && this->m_source->state() != AkElement::ElementStateNull)
        return true;

    if (this->m_videoCapture
        && this->m_videoCapture->state() != AkElement::ElementStateNull)
        return true;

    if (this->m_desktopCapture
        && this->m_desktopCapture->state() != AkElement::ElementStateNull)
        return true;

    return false;
}

QString MediaTools::fileNameFromUri(const QString &uri) const
{
    return QFileInfo(uri).baseName();
}

bool MediaTools::matches(const QString &pattern, const QStringList &strings) const
{
    if (pattern.isEmpty())
        return true;

    foreach (QString str, strings)
        if (str.contains(QRegExp(pattern,
                                 Qt::CaseInsensitive,
                                 QRegExp::Wildcard)))
            return true;

    return false;
}

QString MediaTools::currentTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
}

QStringList MediaTools::standardLocations(const QString &type) const
{
    if (type == "movies")
        return QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    else if (type == "pictures")
        return QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

    return QStringList();
}

QString MediaTools::saveFileDialog(const QString &caption,
                                   const QString &fileName,
                                   const QString &directory,
                                   const QString &suffix,
                                   const QString &filters) const
{
    QFileDialog saveFileDialog(NULL,
                               caption,
                               fileName,
                               filters);

    saveFileDialog.setModal(true);
    saveFileDialog.setDefaultSuffix(suffix);
    saveFileDialog.setDirectory(directory);
    saveFileDialog.setFileMode(QFileDialog::AnyFile);
    saveFileDialog.setAcceptMode(QFileDialog::AcceptSave);

    if (saveFileDialog.exec() != QDialog::Accepted)
        return QString();

    QStringList selectedFiles = saveFileDialog.selectedFiles();

    return selectedFiles.isEmpty()? QString(): selectedFiles.at(0);
}

QString MediaTools::readFile(const QString &fileName) const
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = file.readAll();
    file.close();

    return data;
}

QString MediaTools::urlToLocalFile(const QUrl &url) const
{
    return url.toLocalFile();
}

bool MediaTools::embedEffectControls(const QString &where,
                                     const QString &effectId,
                                     const QString &name) const
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            QObject *interface =
                    this->m_effectsList[i]->controlInterface(this->m_appEngine,
                                                             effectId);

            if (!interface)
                return false;

            if (!name.isEmpty())
                interface->setObjectName(name);

            return this->embedInterface(this->m_appEngine, interface, where);
        }

    return false;
}

bool MediaTools::embedMediaControls(const QString &where,
                                     const QString &stream,
                                     const QString &name) const
{
    if (!this->m_appEngine
        || !this->m_videoCapture
        || !this->m_desktopCapture
        || !this->m_source)
        return false;

    QObject *interface = NULL;

    if (this->isCamera(stream))
        interface = this->m_videoCapture->controlInterface(this->m_appEngine,
                                                           stream);
    else if (this->isDesktop(stream))
        interface = this->m_desktopCapture->controlInterface(this->m_appEngine, "");
    else if (this->isVideo(stream))
        interface = this->m_source->controlInterface(this->m_appEngine, "");
    else
        return false;

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    return this->embedInterface(this->m_appEngine, interface, where);
}

bool MediaTools::embedRecordControls(const QString &where,
                                     const QString &format,
                                     const QString &name) const
{
    if (!this->m_record)
        return false;

    QObject *interface = this->m_record->controlInterface(this->m_appEngine,
                                                          format);

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    return this->embedInterface(this->m_appEngine, interface, where);
}

bool MediaTools::embedVirtualCameraControls(const QString &where, const QString &name) const
{
    if (!this->m_virtualCamera)
        return false;

    QObject *interface = this->m_virtualCamera->controlInterface(this->m_appEngine, "");

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    return this->embedInterface(this->m_appEngine, interface, where);
}

void MediaTools::removeInterface(const QString &where,
                                 QQmlApplicationEngine *engine) const
{
    engine = engine? engine: this->m_appEngine;

    if (!engine)
        return;

    foreach (QObject *obj, engine->rootObjects()) {
        QQuickItem *item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        QList<QQuickItem *> childItems = item->childItems();

        foreach (QQuickItem *child, childItems) {
            child->setParentItem(NULL);
            child->setParent(NULL);

            delete child;
        }
    }
}

AkElementPtr MediaTools::sourceElement() const
{
    if (this->isCamera(this->m_curStream))
        return this->m_videoCapture;
    else if (this->isDesktop(this->m_curStream))
        return this->m_desktopCapture;
    else
        return this->m_source;

    return AkElementPtr();
}

bool MediaTools::embedInterface(QQmlApplicationEngine *engine,
                                QObject *interface,
                                const QString &where) const
{
    if (!engine || !interface)
        return false;

    foreach (QObject *obj, engine->rootObjects()) {
        // First, find where to embed the UI.
        QQuickItem *item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        QQuickItem *interfaceItem = qobject_cast<QQuickItem *>(interface);

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

bool MediaTools::sortByDescription(const QString &pluginId1,
                                   const QString &pluginId2)
{
    QString desc1 = AkElement::pluginInfo(pluginId1)["MetaData"].toMap()
                                                    ["description"].toString();

    QString desc2 = AkElement::pluginInfo(pluginId2)["MetaData"].toMap()
                                                    ["description"].toString();

    return desc1 < desc2;
}

void MediaTools::setRecordAudioFrom(const QString &recordAudioFrom)
{
    RecordFrom recordAudio = recordFromMap->key(recordAudioFrom, RecordFromMic);

    if (this->m_recordAudioFrom == recordAudio)
        return;

    if (!this->m_mic ||
        !this->m_audioSwitch ||
        !this->m_record) {
        this->m_recordAudioFrom = recordAudio;
        emit this->recordAudioFromChanged(recordAudioFrom);

        return;
    }

    if (recordAudio == RecordFromNone) {
        if (this->m_recordAudioFrom == RecordFromMic)
            this->m_mic->setState(AkElement::ElementStateNull);

        this->m_audioSwitch->setState(AkElement::ElementStateNull);

        QObject::disconnect(this->m_record.data(),
                            SIGNAL(stateChanged(AkElement::ElementState)),
                            this->m_audioSwitch.data(),
                            SLOT(setState(AkElement::ElementState)));

        if (this->m_recordAudioFrom == RecordFromMic)
            QObject::disconnect(this->m_record.data(),
                                SIGNAL(stateChanged(AkElement::ElementState)),
                                this->m_mic.data(),
                                SLOT(setState(AkElement::ElementState)));
    } else {
        if (recordAudio == RecordFromSource) {
            if (this->m_recordAudioFrom == RecordFromMic) {
                this->m_mic->setState(AkElement::ElementStateNull);

                QObject::disconnect(this->m_record.data(),
                                    SIGNAL(stateChanged(AkElement::ElementState)),
                                    this->m_mic.data(),
                                    SLOT(setState(AkElement::ElementState)));
            }

            this->m_audioSwitch->setProperty("inputIndex", 0);
        } else if (recordAudio == RecordFromMic) {
            if (this->m_record->state() == AkElement::ElementStatePlaying ||
                this->m_record->state() == AkElement::ElementStatePaused)
                this->m_mic->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this->m_mic.data(),
                             SLOT(setState(AkElement::ElementState)));

            this->m_audioSwitch->setProperty("inputIndex", 1);
        }

        if (this->m_recordAudioFrom == RecordFromNone) {
            if (this->m_record->state() == AkElement::ElementStatePlaying ||
                this->m_record->state() == AkElement::ElementStatePaused)
                this->m_audioSwitch->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(AkElement::ElementState)),
                             this->m_audioSwitch.data(),
                             SLOT(setState(AkElement::ElementState)));
        }
    }

    this->m_recordAudioFrom = recordAudio;
    this->updateRecordingParams();
    emit this->recordAudioFromChanged(recordAudioFrom);
}

void MediaTools::setCurRecordingFormat(const QString &curRecordingFormat)
{
    if (this->m_record)
        this->m_record->setProperty("outputFormat", curRecordingFormat);
}

void MediaTools::setRecording(bool recording)
{
    if (this->m_recording == recording)
        return;

    this->m_recording = recording;
    emit this->recordingChanged(recording);
}

void MediaTools::mutexLock()
{
    this->m_mutex.lock();
}

void MediaTools::mutexUnlock()
{
    this->m_mutex.unlock();
}

void MediaTools::takePhoto()
{
    this->m_photo = AkUtils::packetToImage(this->m_curPacket).copy();
}

void MediaTools::savePhoto(const QString &fileName)
{
    QString path = fileName;
    path.replace("file://", "");

    if (path.isEmpty())
        return;

    this->m_photo.save(path);
}

bool MediaTools::start()
{
    if (this->m_curStream.isEmpty()
        || !this->m_source
        || !this->m_videoCapture
        || !this->m_desktopCapture)
        return false;

    // Setup the recording streams caps.
    AkElementPtr source;

    if (this->isCamera(this->m_curStream))
        source = this->m_videoCapture;
    else if (this->isDesktop(this->m_curStream))
        source = this->m_desktopCapture;
    else {
        source = this->m_source;
        source->setProperty("loop", true);
    }

    QList<int> streams;
    QMetaObject::invokeMethod(source.data(),
                              "streams", Qt::DirectConnection,
                              Q_RETURN_ARG(QList<int>, streams));

    int audioStream = -1;
    int videoStream = -1;
    AkCaps audioCaps;
    AkCaps videoCaps;

    if (streams.isEmpty()) {
        // Find the defaults audio and video streams.
        QMetaObject::invokeMethod(source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(QString, "video/x-raw"));

        QMetaObject::invokeMethod(source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, audioStream),
                                  Q_ARG(QString, "audio/x-raw"));

        // Read streams caps.
        if (videoStream >= 0) {
            QMetaObject::invokeMethod(source.data(),
                                      "caps", Qt::DirectConnection,
                                      Q_RETURN_ARG(AkCaps, videoCaps),
                                      Q_ARG(int, videoStream));
            streams << videoStream;
        }

        if (audioStream >= 0) {
            QMetaObject::invokeMethod(source.data(),
                                      "caps", Qt::DirectConnection,
                                      Q_RETURN_ARG(AkCaps, audioCaps),
                                      Q_ARG(int, audioStream));
            streams << audioStream;
        }

        // Only decode the default streams.
        QMetaObject::invokeMethod(source.data(),
                                  "setStreams", Qt::DirectConnection,
                                  Q_ARG(QList<int>, streams));
    } else {
        foreach (int stream, streams) {
            AkCaps caps;
            QMetaObject::invokeMethod(source.data(),
                                      "caps", Qt::DirectConnection,
                                      Q_RETURN_ARG(AkCaps, caps),
                                      Q_ARG(int, stream));

            if (videoStream < 0 && caps.mimeType() == "video/x-raw") {
                videoStream = stream;
                videoCaps = caps;
            } else if (audioStream < 0 && caps.mimeType() == "audio/x-raw") {
                audioStream = stream;
                audioCaps = caps;
            }
        }
    }

    this->updateRecordingParams();
    source->setState(AkElement::ElementStatePlaying);
    emit this->stateChanged(source->state());

    return source->state() == AkElement::ElementStatePlaying;
}

void MediaTools::stop()
{
    if (this->m_source)
        this->m_source->setState(AkElement::ElementStateNull);

    if (this->m_videoCapture)
        this->m_videoCapture->setState(AkElement::ElementStateNull);

    if (this->m_desktopCapture)
        this->m_desktopCapture->setState(AkElement::ElementStateNull);

    emit this->stateChanged(AkElement::ElementStateNull);
}

bool MediaTools::startRecording(const QString &fileName)
{
    QVariantList streams = this->m_record->property("streams").toList();
    AkVideoCaps videoCaps = streams[0].toMap()["caps"].value<AkCaps>();

    this->m_videoGen->setProperty("fps", QVariant::fromValue(videoCaps.fps()));
    this->m_record->setProperty("location", fileName);
    this->m_record->setState(AkElement::ElementStatePlaying);

    if (this->m_record->state() == AkElement::ElementStatePlaying) {
        if (this->m_recordAudioFrom != RecordFromNone) {
            this->m_audioSwitch->setState(AkElement::ElementStatePlaying);

            if (this->m_recordAudioFrom == RecordFromMic)
                this->m_mic->setState(AkElement::ElementStatePlaying);
        } else {
            this->m_audioSwitch->setState(AkElement::ElementStateNull);
            this->m_mic->setState(AkElement::ElementStateNull);
        }

        this->setRecording(true);

        return true;
    }

    this->m_record->setState(AkElement::ElementStateNull);
    this->m_audioSwitch->setState(AkElement::ElementStateNull);
    this->m_mic->setState(AkElement::ElementStateNull);

    this->setRecording(false);

    return false;
}

void MediaTools::stopRecording()
{
    if (this->m_record)
        this->m_record->setState(AkElement::ElementStateNull);

    if (this->m_audioSwitch)
        this->m_audioSwitch->setState(AkElement::ElementStateNull);

    if (this->m_mic)
        this->m_mic->setState(AkElement::ElementStateNull);

    this->setRecording(false);
}

bool MediaTools::startVirtualCamera(const QString &fileName)
{
    AkElementPtr source = this->sourceElement();
    QList<int> streams;
    QMetaObject::invokeMethod(source.data(),
                              "streams", Qt::DirectConnection,
                              Q_RETURN_ARG(QList<int>, streams));

    int videoStream = -1;
    AkCaps videoCaps;

    if (streams.isEmpty()) {
        // Find the defaults audio and video streams.
        QMetaObject::invokeMethod(source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(QString, "video/x-raw"));

        // Read streams caps.
        if (videoStream >= 0) {
            QMetaObject::invokeMethod(source.data(),
                                      "caps", Qt::DirectConnection,
                                      Q_RETURN_ARG(AkCaps, videoCaps),
                                      Q_ARG(int, videoStream));
        }
    } else {
        foreach (int stream, streams) {
            AkCaps caps;
            QMetaObject::invokeMethod(source.data(),
                                      "caps", Qt::DirectConnection,
                                      Q_RETURN_ARG(AkCaps, caps),
                                      Q_ARG(int, stream));

            if (videoStream < 0 && caps.mimeType() == "video/x-raw") {
                videoStream = stream;
                videoCaps = caps;

                break;
            }
        }
    }

    this->m_virtualCamera->setProperty("media", fileName);
    QMetaObject::invokeMethod(this->m_virtualCamera.data(),
                              "clearStreams");

    AkVideoCaps oVideoCaps(videoCaps);
    oVideoCaps.format() = AkVideoCaps::Format_yuv420p;

    QMetaObject::invokeMethod(this->m_virtualCamera.data(),
                              "addStream",
                              Q_ARG(int, 0),
                              Q_ARG(AkCaps, oVideoCaps.toCaps()));

    this->m_virtualCamera->setState(AkElement::ElementStatePlaying);

    return true;
}

void MediaTools::stopVirtualCamera()
{
    if (this->m_virtualCamera)
        this->m_virtualCamera->setState(AkElement::ElementStateNull);
}

void MediaTools::setCurStream(const QString &stream)
{
    if (this->m_curStream == stream)
        return;

    this->m_curStream = stream;
    AkElementPtr source = this->sourceElement();

    if (source)
        source->setProperty("media", stream);

    emit this->curStreamChanged(stream);
}

void MediaTools::setPlayAudioFromSource(bool playAudio)
{
    if (this->m_playAudioFromSource == playAudio)
        return;

    this->m_playAudioFromSource = playAudio;
    emit this->playAudioFromSourceChanged(playAudio);

    if (!this->m_source || !this->m_audioOutput)
        return;

    AkElement::ElementState sourceState = this->m_source->state();

    if (playAudio) {
        if (sourceState == AkElement::ElementStatePlaying ||
            sourceState == AkElement::ElementStatePaused)
            this->m_audioOutput->setState(sourceState);

        QObject::connect(this->m_source.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this->m_audioOutput.data(),
                         SLOT(setState(AkElement::ElementState)));
    } else {
        this->m_audioOutput->setState(AkElement::ElementStateNull);

        QObject::disconnect(this->m_source.data(),
                            SIGNAL(stateChanged(AkElement::ElementState)),
                            this->m_audioOutput.data(),
                            SLOT(setState(AkElement::ElementState)));
    }
}

void MediaTools::setWindowWidth(int windowWidth)
{
    if (this->m_windowWidth == windowWidth)
        return;

    this->m_windowWidth = windowWidth;
    emit this->windowWidthChanged(windowWidth);
}

void MediaTools::setWindowHeight(int windowHeight)
{
    if (this->m_windowHeight == windowHeight)
        return;

    this->m_windowHeight = windowHeight;
    emit this->windowHeightChanged(windowHeight);
}

void MediaTools::setAdvancedMode(bool advancedMode)
{
    if (this->m_advancedMode == advancedMode)
        return;

    this->m_advancedMode = advancedMode;
    emit this->advancedModeChanged(advancedMode);
}

void MediaTools::setEnableVirtualCamera(bool enableVirtualCamera)
{
    if (this->m_enableVirtualCamera == enableVirtualCamera)
        return;

    this->m_enableVirtualCamera = enableVirtualCamera;
    emit this->enableVirtualCameraChanged(enableVirtualCamera);
}

void MediaTools::resetCurStream()
{
    this->setCurStream("");
}

void MediaTools::resetPlayAudioFromSource()
{
    this->setPlayAudioFromSource(true);
}

void MediaTools::resetRecordAudioFrom()
{
    this->setRecordAudioFrom("mic");
}

void MediaTools::resetCurRecordingFormat()
{
    this->setCurRecordingFormat("");
}

void MediaTools::resetRecording()
{
    this->setRecording(false);
}

void MediaTools::resetWindowWidth()
{
    this->setWindowWidth(0);
}

void MediaTools::resetWindowHeight()
{
    this->setWindowHeight(0);
}

void MediaTools::resetAdvancedMode()
{
    this->setAdvancedMode(false);
}

void MediaTools::resetEnableVirtualCamera()
{
    this->setEnableVirtualCamera(false);
}

void MediaTools::resetEffects()
{
    if (this->m_effectsList.isEmpty())
        return;

    AkElementPtr source = this->sourceElement();
    bool playing = this->isPlaying();

    if (playing)
        source->setState(AkElement::ElementStatePaused);

    if (this->m_videoMux)
        this->m_videoMux->unlink(this->m_effectsList.first());

    if (this->m_videoOutput)
        this->m_effectsList.last()->unlink(this->m_videoOutput);

    if (this->m_videoMux
        && this->m_videoOutput)
        this->m_videoMux->link(this->m_videoOutput, Qt::DirectConnection);

    if (playing)
        source->setState(AkElement::ElementStatePlaying);

    this->m_effectsList.clear();
    emit this->currentEffectsChanged();
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("AudioConfigs");
    this->setPlayAudioFromSource(config.value("playAudio", true).toBool());
    this->setRecordAudioFrom(config.value("recordAudioFrom", "mic").toString());
    config.endGroup();

    config.beginGroup("OutputConfigs");
    this->setEnableVirtualCamera(config.value("enableVirtualCamera", false).toBool());
    config.endGroup();

    config.beginGroup("GeneralConfigs");

    this->setAdvancedMode(config.value("advancedMode", false).toBool());

    QSize windowSize = config.value("windowSize", QSize(1024, 600)).toSize();
    this->m_windowWidth = windowSize.width();
    this->m_windowHeight = windowSize.height();

    config.endGroup();

    config.beginGroup("Effects");
    int size = config.beginReadArray("effects");
    QStringList effects;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        effects << config.value("effect").toString();

        if (!this->m_advancedMode)
            break;
    }

    config.endArray();
    config.endGroup();

    foreach (QString effectId, effects) {
        AkElementPtr effect = this->appendEffect(effectId);

        if (!effect)
            continue;

        config.beginGroup("Effects_" + effectId);

        foreach (QString key, config.allKeys())
            effect->setProperty(key.toStdString().c_str(), config.value(key));

        config.endGroup();
    }

    config.beginGroup("CustomStreams");
    size = config.beginReadArray("streams");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        QString stream = config.value("dev").toString();
        QString description = config.value("description").toString();
        this->setStream(stream, description);
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("RecordConfigs");

#ifdef USE_GSTREAMER
    QString defaultRecordingFormat("webmmux");
#else
    QString defaultRecordingFormat("webm");
#endif

    this->setCurRecordingFormat(config.value("recordingFormat", defaultRecordingFormat).toString());

    config.endGroup();
}

void MediaTools::saveConfigs()
{
    QSettings config;

    config.beginGroup("AudioConfigs");
    config.setValue("playAudio", this->playAudioFromSource());
    config.setValue("recordAudioFrom", this->recordAudioFrom());
    config.endGroup();

    config.beginGroup("OutputConfigs");
    config.setValue("enableVirtualCamera", this->enableVirtualCamera());
    config.endGroup();

    config.beginGroup("GeneralConfigs");

    config.setValue("advancedMode", this->advancedMode());
    config.setValue("windowSize", QSize(this->m_windowWidth,
                                        this->m_windowHeight));

    config.endGroup();

    config.beginGroup("Effects");
    config.beginWriteArray("effects");

    int ei = 0;

    foreach (AkElementPtr effect, this->m_effectsList)
        if (!effect->property("preview").toBool()) {
            config.setArrayIndex(ei);
            config.setValue("effect", effect->pluginId());
            ei++;
        }

    config.endArray();
    config.endGroup();

    foreach (AkElementPtr effect, this->m_effectsList) {
        config.beginGroup("Effects_" + effect->pluginId());

        for (int property = 0;
             property < effect->metaObject()->propertyCount();
             property++) {
            QMetaProperty metaProperty = effect->metaObject()->property(property);

            if (metaProperty.isWritable()) {
                const char *propertyName = metaProperty.name();
                config.setValue(propertyName, effect->property(propertyName));
            }
        }

        config.endGroup();
    }

    config.beginGroup("CustomStreams");
    config.beginWriteArray("streams");

    int i = 0;

    foreach (QString stream, this->m_streams.keys()) {
        config.setArrayIndex(i);
        config.setValue("dev", stream);
        config.setValue("description", this->m_streams[stream]);
        i++;
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("PluginConfigs");

#ifdef Q_OS_WIN32
    QDir applicationDir(QCoreApplication::applicationDirPath());
    QString qmlPluginPath = applicationDir.relativeFilePath(Ak::qmlPluginPath());
#else
    QString qmlPluginPath = Ak::qmlPluginPath();
#endif

    config.setValue("qmlPluginPath", qmlPluginPath);
    config.setValue("recursive", AkElement::recursiveSearch());

    config.beginWriteArray("paths");

    i = 0;

    foreach (QString path, AkElement::searchPaths(AkElement::SearchPathsExtras)) {
        config.setArrayIndex(i);

#ifdef Q_OS_WIN32
        config.setValue("path", applicationDir.relativeFilePath(path));
#else
        config.setValue("path", path);
#endif

        i++;
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("RecordConfigs");
    config.setValue("recordingFormat", this->curRecordingFormat());
    config.endGroup();
}

void MediaTools::setStream(const QString &stream, const QString &description)
{
    this->m_streams[stream] = description;
    emit this->streamsChanged();
}

void MediaTools::removeStream(const QString &stream)
{
    if (this->m_streams.contains(stream)) {
        this->m_streams.remove(stream);

        if (this->m_curStream == stream)
            this->resetCurStream();

        emit this->streamsChanged();
    }
}

void MediaTools::resetStreams()
{
    this->m_streams.clear();
    emit this->streamsChanged();
}

void MediaTools::cleanAll()
{
    this->resetCurStream();
    this->saveConfigs();
}

void MediaTools::webcamsChanged(const QStringList &webcams)
{
    Q_UNUSED(webcams)

    emit this->streamsChanged();
}

void MediaTools::updateRecordingParams()
{
    if (this->m_curStream.isEmpty()
        || !this->m_source
        || !this->m_videoCapture
        || !this->m_desktopCapture)
        return;

    // Setup the recording streams caps.
    AkElementPtr source = this->sourceElement();

    QList<int> streams;
    QMetaObject::invokeMethod(source.data(),
                              "streams",
                              Q_RETURN_ARG(QList<int>, streams));

    QVector<AkCaps> streamCaps(2);

    if (streams.isEmpty()) {
        int audioStream = -1;
        int videoStream = -1;

        // Find the defaults audio and video streams.
        QMetaObject::invokeMethod(source.data(),
                                  "defaultStream",
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(QString, "video/x-raw"));

        QMetaObject::invokeMethod(source.data(),
                                  "defaultStream",
                                  Q_RETURN_ARG(int, audioStream),
                                  Q_ARG(QString, "audio/x-raw"));

        // Read streams caps.
        if (videoStream >= 0) {
            AkCaps videoCaps;

            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, videoCaps),
                                      Q_ARG(int, videoStream));

            streamCaps[0] = videoCaps;
        }

        if (audioStream >= 0) {
            AkCaps audioCaps;

            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, audioCaps),
                                      Q_ARG(int, audioStream));

            streamCaps[1] = audioCaps;
        }
    } else {
        foreach (int stream, streams) {
            AkCaps caps;

            QMetaObject::invokeMethod(source.data(),
                                      "caps",
                                      Q_RETURN_ARG(AkCaps, caps),
                                      Q_ARG(int, stream));

            if (caps.mimeType() == "audio/x-raw")
                streamCaps[1] = caps;
            else if (caps.mimeType() == "video/x-raw")
                streamCaps[0] = caps;/*
            else if (caps.mimeType() == "text/x-raw")
                streamCaps[2] = caps;*/
        }
    }

    if (this->m_recordAudioFrom == RecordFromMic) {
        AkCaps audioCaps;

        QMetaObject::invokeMethod(this->m_mic.data(),
                                  "caps",
                                  Q_RETURN_ARG(AkCaps, audioCaps));

        streamCaps[1] = audioCaps;
    } else if (this->m_recordAudioFrom == RecordFromNone)
        streamCaps[1] = AkCaps();

    QSettings config;
    QMetaObject::invokeMethod(this->m_record.data(), "clearStreams");

    for (int stream = 0; stream < streamCaps.size(); stream++)
        if (streamCaps[stream]) {
            config.beginGroup(QString("RecordConfigs_%1_%2")
                              .arg(this->curRecordingFormat())
                              .arg(stream));

            QVariantMap streamConfigs;
            QStringList configKeys = config.allKeys();
            configKeys.removeOne("caps");
            configKeys.removeOne("index");
            configKeys.removeOne("timeBase");

            foreach (QString key, configKeys)
                streamConfigs[key] = config.value(key);

            QString mimeType = streamCaps[stream].mimeType();

            if (mimeType == "audio/x-raw")
                streamConfigs["label"] = tr("Audio");
            else if (mimeType == "video/x-raw")
                streamConfigs["label"] = tr("Video");
            else if (mimeType == "text/x-raw")
                streamConfigs["label"] = tr("Subtitle");

            QMetaObject::invokeMethod(this->m_record.data(),
                                      "addStream",
                                      Q_RETURN_ARG(QVariantMap, streamConfigs),
                                      Q_ARG(int, stream),
                                      Q_ARG(AkCaps, streamCaps[stream]),
                                      Q_ARG(QVariantMap, streamConfigs));

            configKeys = streamConfigs.keys();
            configKeys.removeOne("caps");
            configKeys.removeOne("index");
            configKeys.removeOne("timeBase");

            foreach (QString key, configKeys)
                config.setValue(key, streamConfigs[key]);

            config.endGroup();
        }
}
