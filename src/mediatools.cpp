/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>

#include "mediatools.h"

MediaTools::MediaTools(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->m_appEngine = engine;

    this->m_recordFromMap[RecordFromNone] = "none";
    this->m_recordFromMap[RecordFromSource] = "source";
    this->m_recordFromMap[RecordFromMic] = "mic";

    this->resetCurStream();
    this->m_playAudioFromSource = true;
    this->m_recordAudioFrom = RecordFromMic;
    this->resetCurRecordingFormat();
    this->resetRecording();
    this->resetRecordingFormats();
    this->resetStreams();
    this->m_windowWidth = 0;
    this->m_windowHeight = 0;

    Qb::init(engine);

    QSettings config;

    config.beginGroup("PluginConfigs");
    Qb::setQmlPluginPath(config.value("qmlPluginPath", Qb::qmlPluginPath()).toString());
    QbElement::setRecursiveSearch(config.value("recursive", false).toBool());

    int size = config.beginReadArray("paths");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        QString path = config.value("path").toString();
        QbElement::addSearchPath(path);
    }

    config.endArray();
    config.endGroup();

    this->m_pipeline = QbElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/mainpipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_source),
                                  Q_ARG(QString, "source"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_audioSwitch),
                                  Q_ARG(QString, "audioSwitch"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_audioOutput),
                                  Q_ARG(QString, "audioOutput"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_mic),
                                  Q_ARG(QString, "mic"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_record),
                                  Q_ARG(QString, "record"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_videoCapture),
                                  Q_ARG(QString, "videoCapture"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_desktopCapture),
                                  Q_ARG(QString, "desktopCapture"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_videoSync),
                                  Q_ARG(QString, "videoSync"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_videoConvert),
                                  Q_ARG(QString, "videoConvert"));

        if (this->m_videoConvert)
            this->m_videoConvert->link(this);

        if (this->m_source) {
            QObject::connect(this->m_source.data(),
                             SIGNAL(error(const QString &)),
                             this,
                             SIGNAL(error(const QString &)));

            QObject::connect(this->m_source.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this,
                             SIGNAL(stateChanged()));
        }

        if (this->m_videoCapture) {
            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(error(const QString &)),
                             this,
                             SIGNAL(error(const QString &)));

            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this,
                             SIGNAL(stateChanged()));
        }

        if (this->m_desktopCapture) {
            QObject::connect(this->m_desktopCapture.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this,
                             SIGNAL(stateChanged()));
        }

        QObject::connect(this,
                         SIGNAL(stateChanged()),
                         this,
                         SIGNAL(isPlayingChanged()));

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

    this->loadConfigs();
}

MediaTools::~MediaTools()
{
    this->resetCurStream();
    this->saveConfigs();
}

void MediaTools::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw")
        return;

    if (!this->sender())
        return;

    QString sender = this->sender()->objectName();

    if (sender == "videoConvert") {
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
    return this->m_recordFromMap[this->m_recordAudioFrom];
}

QString MediaTools::curRecordingFormat() const
{
    return this->m_curRecordingFormat;
}

bool MediaTools::recording() const
{
    return this->m_recording;
}

QStringList MediaTools::recordingFormats() const
{
    QStringList formats;

    foreach (RecordingFormat recordingFormat, this->m_recordingFormats)
        formats << recordingFormat.description();

    return formats;
}

QString MediaTools::recordingFormatParams(const QString &formatId) const
{
    foreach (RecordingFormat recordingFormat, this->m_recordingFormats)
        if (recordingFormat.description() == formatId)
            return recordingFormat.params();

    return QString();
}

QStringList MediaTools::recordingFormatSuffix(const QString &formatId) const
{
    foreach (RecordingFormat recordingFormat, this->m_recordingFormats)
        if (recordingFormat.description() == formatId)
            return recordingFormat.suffix();

    return QStringList();
}

void MediaTools::removeRecordingFormat(const QString &formatId)
{
    for (int i = 0; i < this->m_recordingFormats.size(); i++)
        if (this->m_recordingFormats[i].description() == formatId) {
            this->m_recordingFormats.removeAt(i);
            emit this->recordingFormatsChanged();

            break;
        }
}

void MediaTools::moveRecordingFormat(const QString &formatId, int index)
{
    for (int i = 0; i < this->m_recordingFormats.size(); i++)
        if (this->m_recordingFormats[i].description() == formatId) {
            RecordingFormat format = this->m_recordingFormats.takeAt(i);
            this->m_recordingFormats.insert(index, format);

            break;
        }

    emit this->recordingFormatsChanged();
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
    QStringList effects = QbElement::listPlugins("VideoFilter");

    foreach (QbElementPtr effect, this->m_effectsList) {
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
    return QbElement::pluginInfo(effectId);
}

QString MediaTools::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return QString();

    return QbElement::pluginInfo(effectId)["MetaData"].toMap()
                                          ["description"].toString();
}

QStringList MediaTools::currentEffects() const
{
    QStringList effects;

    foreach (QbElementPtr effect, this->m_effectsList)
        if (!effect->property("preview").toBool())
            effects << effect->pluginId();

    return effects;
}

QbElementPtr MediaTools::appendEffect(const QString &effectId, bool preview)
{
    int i = this->m_effectsList.size() - 1;
    QbElementPtr effect = QbElement::create(effectId);

    if (!effect)
        return QbElementPtr();

    if (preview)
        effect->setProperty("preview", preview);

    this->m_effectsList << effect;

    if (i < 0) {
        if (this->m_videoSync) {
            this->m_videoSync->unlink(this->m_videoConvert);
            this->m_videoSync->link(effect);
        }
    } else {
        this->m_effectsList[i]->unlink(this->m_videoConvert);
        this->m_effectsList[i]->link(effect);
    }

    effect->link(this->m_videoConvert);

    if (!preview)
        emit this->currentEffectsChanged();

    return effect;
}

void MediaTools::removeEffect(const QString &effectId)
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            if (i == 0) {
                if (this->m_effectsList.size() == 1) {
                    if (this->m_videoSync)
                        this->m_videoSync->link(this->m_videoConvert);
                }
                else
                    this->m_videoSync->link(this->m_effectsList[i + 1]);
            } else if (i == this->m_effectsList.size() - 1)
                this->m_effectsList[i - 1]->link(this->m_videoConvert);
            else
                this->m_effectsList[i - 1]->link(this->m_effectsList[i + 1]);

            bool isPreview = this->m_effectsList[i]->property("preview").toBool();
            this->m_effectsList.removeAt(i);

            if (!isPreview)
                emit this->currentEffectsChanged();

            break;
        }
}

void MediaTools::moveEffect(const QString &effectId, int index)
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            QbElementPtr effect = this->m_effectsList.takeAt(i);
            this->m_effectsList.insert(index, effect);

            break;
        }

    emit this->currentEffectsChanged();
}

bool MediaTools::embedEffectControls(const QString &where, const QString &effectId, const QString &name) const
{
    for (int i = 0; i < this->m_effectsList.size(); i++)
        if (this->m_effectsList[i]->pluginId() == effectId) {
            QObject *interface = this->m_effectsList[i]->controlInterface(this->m_appEngine, effectId);

            if (!interface)
                return false;

            if (!name.isEmpty())
                interface->setObjectName(name);

            return this->embedInterface(this->m_appEngine, interface, where);
        }

    return false;
}

void MediaTools::removeEffectControls(const QString &where) const
{
    this->removeInterface(this->m_appEngine, where);
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
    QList<QbElementPtr> effectsList = this->m_effectsList;

    foreach (QbElementPtr effect, effectsList)
        if (effect->property("preview").toBool()
            && (effectId.isEmpty()
                || effect->pluginId() == effectId)) {
            this->removeEffect(effect->pluginId());
        }
}

QString MediaTools::bestRecordFormatOptions(const QString &fileName) const
{
    QString ext = QFileInfo(fileName).completeSuffix().toLower().trimmed();

    if (ext.isEmpty())
        return QString();

    foreach (RecordingFormat format, this->m_recordingFormats)
        foreach (QString s, format.suffix())
            if (s.toLower().trimmed() == ext)
                return format.params();

    return QString();
}

bool MediaTools::isPlaying()
{
    if (this->m_source
        && this->m_source->state() != QbElement::ElementStateNull)
        return true;

    if (this->m_videoCapture
        && this->m_videoCapture->state() != QbElement::ElementStateNull)
        return true;

    if (this->m_desktopCapture
        && this->m_desktopCapture->state() != QbElement::ElementStateNull)
        return true;

    return false;
}

QString MediaTools::fileNameFromUri(const QString &uri) const
{
    return QFileInfo(uri).baseName();
}

bool MediaTools::embedCameraControls(const QString &where,
                                     const QString &stream,
                                     const QString &name) const
{
    QObject *interface = this->m_videoCapture->controlInterface(this->m_appEngine, stream);

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    return this->embedInterface(this->m_appEngine, interface, where);
}

void MediaTools::removeCameraControls(const QString &where) const
{
    this->removeInterface(this->m_appEngine, where);
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

void MediaTools::removeInterface(QQmlApplicationEngine *engine, const QString &where) const
{
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

bool MediaTools::sortByDescription(const QString &pluginId1,
                                   const QString &pluginId2)
{
    QString desc1 = QbElement::pluginInfo(pluginId1)["MetaData"].toMap()
                                                    ["description"].toString();

    QString desc2 = QbElement::pluginInfo(pluginId2)["MetaData"].toMap()
                                                    ["description"].toString();

    return desc1 < desc2;
}

void MediaTools::setRecordAudioFrom(const QString &recordAudioFrom)
{
    RecordFrom recordAudio = this->m_recordFromMap.values().contains(recordAudioFrom)?
                                 this->m_recordFromMap.key(recordAudioFrom):
                                 RecordFromMic;

    if (this->m_recordAudioFrom == recordAudio)
        return;

    if (!this->m_mic ||
        !this->m_audioSwitch ||
        !this->m_record) {
        this->m_recordAudioFrom = recordAudio;
        emit this->recordAudioFromChanged();

        return;
    }

    if (recordAudio == RecordFromNone) {
        if (this->m_recordAudioFrom == RecordFromMic)
            this->m_mic->setState(QbElement::ElementStateNull);

        this->m_audioSwitch->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_record.data(),
                            SIGNAL(stateChanged(QbElement::ElementState)),
                            this->m_audioSwitch.data(),
                            SLOT(setState(QbElement::ElementState)));

        if (this->m_recordAudioFrom == RecordFromMic)
            QObject::disconnect(this->m_record.data(),
                                SIGNAL(stateChanged(QbElement::ElementState)),
                                this->m_mic.data(),
                                SLOT(setState(QbElement::ElementState)));
    } else {
        if (recordAudio == RecordFromSource) {
            if (this->m_recordAudioFrom == RecordFromMic) {
                this->m_mic->setState(QbElement::ElementStateNull);

                QObject::disconnect(this->m_record.data(),
                                    SIGNAL(stateChanged(QbElement::ElementState)),
                                    this->m_mic.data(),
                                    SLOT(setState(QbElement::ElementState)));
            }

            this->m_audioSwitch->setProperty("inputIndex", 0);
        } else if (recordAudio == RecordFromMic) {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_mic->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this->m_mic.data(),
                             SLOT(setState(QbElement::ElementState)));

            this->m_audioSwitch->setProperty("inputIndex", 1);
        }

        if (this->m_recordAudioFrom == RecordFromNone) {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_audioSwitch->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this->m_audioSwitch.data(),
                             SLOT(setState(QbElement::ElementState)));
        }
    }

    this->m_recordAudioFrom = recordAudio;
    emit this->recordAudioFromChanged();
}

void MediaTools::setCurRecordingFormat(const QString &curRecordingFormat)
{
    if (this->m_curRecordingFormat == curRecordingFormat)
        return;

    this->m_curRecordingFormat = curRecordingFormat;
    emit this->curRecordingFormatChanged();
}

void MediaTools::setRecording(bool recording, const QString &fileName)
{
    if (!this->m_pipeline || !this->m_record) {
        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);

        return;
    }

    if (this->m_record->state() != QbElement::ElementStateNull) {
        this->m_record->setState(QbElement::ElementStateNull);
        this->m_audioSwitch->setState(QbElement::ElementStateNull);
        this->m_mic->setState(QbElement::ElementStateNull);

        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);
    }

    if (recording) {
        QString options = this->bestRecordFormatOptions(fileName);

        if (options == "") {
            this->m_recording = false;
            emit this->recordingChanged(this->m_recording);

            return;
        }

        this->m_record->setProperty("location", fileName);
        this->m_record->setProperty("options", options);
        this->m_record->setState(QbElement::ElementStatePlaying);

        if (this->m_record->state() == QbElement::ElementStatePlaying)
            this->m_recording = true;
        else
            this->m_recording = false;

        if (this->m_recordAudioFrom != RecordFromNone) {
            this->m_audioSwitch->setState(QbElement::ElementStatePlaying);

            if (this->m_recordAudioFrom == RecordFromMic)
                this->m_mic->setState(QbElement::ElementStatePlaying);
        } else {
            this->m_audioSwitch->setState(QbElement::ElementStateNull);
            this->m_mic->setState(QbElement::ElementStateNull);
        }

        emit this->recordingChanged(this->m_recording);
    }
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
    this->m_photo = QbUtils::packetToImage(this->m_curPacket).copy();
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
    QbElementPtr source;

    if (this->isCamera(this->m_curStream))
        source = this->m_videoCapture;
    else if (this->isDesktop(this->m_curStream))
        source = this->m_desktopCapture;
    else
        source = this->m_source;

    // Find the defaults audio and video streams.
    int videoStream = -1;

    QMetaObject::invokeMethod(source.data(),
                              "defaultStream", Qt::DirectConnection,
                              Q_RETURN_ARG(int, videoStream),
                              Q_ARG(QString, "video/x-raw"));
    int audioStream = -1;

    QMetaObject::invokeMethod(source.data(),
                              "defaultStream", Qt::DirectConnection,
                              Q_RETURN_ARG(int, audioStream),
                              Q_ARG(QString, "audio/x-raw"));

    QVariantMap streamCaps;
    QList<int> streams;

    if (videoStream >= 0) {
        QbCaps videoCaps;

        QMetaObject::invokeMethod(source.data(),
                                  "caps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbCaps, videoCaps),
                                  Q_ARG(int, videoStream));

        streamCaps["0"] = videoCaps.toString();
        streams << videoStream;
    }

    if (audioStream >= 0) {
        QbCaps audioCaps;

        QMetaObject::invokeMethod(source.data(),
                                  "caps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbCaps, audioCaps),
                                  Q_ARG(int, audioStream));

        streamCaps["1"] = audioCaps.toString();
        streams << audioStream;
    }

    // Only decode the default streams.
    if (this->isVideo(this->m_curStream))
        QMetaObject::invokeMethod(source.data(),
                                  "setStreams", Qt::DirectConnection,
                                  Q_ARG(QList<int>, streams));

    QVariantMap recordStreams;

    // Stream 0 = Video.
    if (streamCaps.contains("0"))
        recordStreams["0"] = streamCaps["0"];

    // Stream 1 = Audio.
    if (this->m_recordAudioFrom == RecordFromMic) {
        QString audioCaps;

        QMetaObject::invokeMethod(this->m_mic.data(),
                                  "streamCaps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, audioCaps));

        recordStreams["1"] = audioCaps;
    } else if (this->m_recordAudioFrom == RecordFromSource
               && streamCaps.contains("1"))
        recordStreams["1"] = streamCaps["1"];

    // Set recording caps.
    QMetaObject::invokeMethod(this->m_record.data(),
                              "setStreamCaps", Qt::DirectConnection,
                              Q_ARG(QVariantMap, recordStreams));

    bool r = this->startStream();
    emit this->stateChanged();

    return r;
}

void MediaTools::stop()
{
    // Clear previous device.
    this->resetRecording();
    this->stopStream();
    emit this->stateChanged();
}

bool MediaTools::startStream()
{
    if (this->m_curStream.isEmpty()
        || !this->m_source
        || !this->m_videoCapture
        || !this->m_desktopCapture)
        return false;

    if (this->isCamera(this->m_curStream))
        this->m_videoCapture->setState(QbElement::ElementStatePlaying);
    else if (this->isDesktop(this->m_curStream))
        this->m_desktopCapture->setState(QbElement::ElementStatePlaying);
    else
        this->m_source->setState(QbElement::ElementStatePlaying);

    return true;
}

void MediaTools::stopStream()
{
    if (this->m_source)
        this->m_source->setState(QbElement::ElementStateNull);

    if (this->m_videoCapture)
        this->m_videoCapture->setState(QbElement::ElementStateNull);

    if (this->m_desktopCapture)
        this->m_desktopCapture->setState(QbElement::ElementStateNull);
}

void MediaTools::setCurStream(const QString &stream)
{
    this->m_curStream = stream;

    if (!this->m_source
        || !this->m_videoCapture
        || !this->m_desktopCapture) {
        emit this->curStreamChanged();

        return;
    }

    bool isPlaying = this->isPlaying();

    if (isPlaying)
        this->stopStream();

    if (stream.isEmpty()) {
        emit this->curStreamChanged();

        return;
    }

    // Set device.
    if (this->isCamera(stream))
        this->m_videoCapture->setProperty("media", stream);
    if (this->isDesktop(stream))
        this->m_desktopCapture->setProperty("media", stream);
    else
        this->m_source->setProperty("media", stream);

    if (isPlaying)
        this->startStream();

    emit this->curStreamChanged();
}

void MediaTools::setPlayAudioFromSource(bool playAudio)
{
    if (this->m_playAudioFromSource == playAudio)
        return;

    this->m_playAudioFromSource = playAudio;
    emit this->playAudioFromSourceChanged();

    if (!this->m_source || !this->m_audioOutput)
        return;

    QbElement::ElementState sourceState = this->m_source->state();

    if (playAudio) {
        if (sourceState == QbElement::ElementStatePlaying ||
            sourceState == QbElement::ElementStatePaused)
            this->m_audioOutput->setState(sourceState);

        QObject::connect(this->m_source.data(),
                         SIGNAL(stateChanged(QbElement::ElementState)),
                         this->m_audioOutput.data(),
                         SLOT(setState(QbElement::ElementState)));
    } else {
        this->m_audioOutput->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_source.data(),
                            SIGNAL(stateChanged(QbElement::ElementState)),
                            this->m_audioOutput.data(),
                            SLOT(setState(QbElement::ElementState)));
    }
}

void MediaTools::setRecordingFormats(const QList<RecordingFormat> &recordingFormats)
{
    if (recordingFormats != this->m_recordingFormats) {
        this->m_recordingFormats = recordingFormats;
        emit this->recordingFormatsChanged();
    }
}

void MediaTools::setRecordingFormat(const QString &description,
                                    const QStringList &suffix,
                                    const QString &params)
{
    for (int i = 0; i < this->m_recordingFormats.size(); i++)
        if (this->m_recordingFormats[i].description() == description) {
            this->m_recordingFormats[i].setSuffix(suffix);
            this->m_recordingFormats[i].setParams(params);
            emit this->recordingFormatsChanged();

            return;
        }

    this->m_recordingFormats << RecordingFormat(description, suffix, params);
    emit this->recordingFormatsChanged();
}

void MediaTools::setWindowWidth(int windowWidth)
{
    if (windowWidth != this->m_windowWidth) {
        this->m_windowWidth = windowWidth;
        emit this->windowWidthChanged();
    }
}

void MediaTools::setWindowHeight(int windowHeight)
{
    if (windowHeight != this->m_windowHeight) {
        this->m_windowHeight = windowHeight;
        emit this->windowHeightChanged();
    }
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

void MediaTools::resetRecordingFormats()
{
    this->setRecordingFormats(QList<RecordingFormat>());
}

void MediaTools::resetWindowWidth()
{
    this->setWindowWidth(0);
}

void MediaTools::resetWindowHeight()
{
    this->setWindowHeight(0);
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");

    this->setPlayAudioFromSource(config.value("playAudio", true).toBool());
    this->setRecordAudioFrom(config.value("recordAudioFrom", "mic").toString());

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
    }

    config.endArray();
    config.endGroup();

    foreach (QString effectId, effects) {
        QbElementPtr effect = this->appendEffect(effectId);

        if (!effect)
            continue;

        config.beginGroup("Effects_" + effectId);

        foreach (QString key, config.allKeys())
            effect->setProperty(key.toStdString().c_str(), config.value(key));

        config.endGroup();
    }

    config.beginGroup("RecordingFormats");
    size = config.beginReadArray("formats");
    QStringList recordingFormats;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        recordingFormats << config.value("format").toString();
    }

    config.endArray();
    config.endGroup();

    if (size < 1) {
        this->setRecordingFormat("Webm",
                                 QStringList() << "webm",
                                 "-i 0 -opt quality=realtime -c:v libvpx -b:v 3M -i 1 -c:a libvorbis -o -f webm");

        this->setRecordingFormat("Ogg",
                                 QStringList() << "ogv" << "ogg",
                                 "-i 0 -c:v libtheora -b:v 3M -i 1 -c:a libvorbis -o -f ogg");
        this->setCurRecordingFormat("Webm");
    } else
        foreach (QString formatId, recordingFormats) {
            if (this->m_curRecordingFormat.isEmpty())
                this->setCurRecordingFormat(formatId);

            config.beginGroup("RecordingFormats_" + formatId);
            QString params = config.value("params").toString();
            QStringList suffix = config.value("suffix").toStringList();
            this->setRecordingFormat(formatId, suffix, params);
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
}

void MediaTools::saveConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");

    config.setValue("playAudio", this->playAudioFromSource());
    config.setValue("recordAudioFrom", this->recordAudioFrom());
    config.setValue("windowSize", QSize(this->m_windowWidth,
                                        this->m_windowHeight));

    config.endGroup();

    config.beginGroup("Effects");
    config.beginWriteArray("effects");

    int ei = 0;

    foreach (QbElementPtr effect, this->m_effectsList)
        if (!effect->property("preview").toBool()) {
            config.setArrayIndex(ei);
            config.setValue("effect", effect->pluginId());
            ei++;
        }

    config.endArray();
    config.endGroup();

    foreach (QbElementPtr effect, this->m_effectsList) {
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

    config.beginGroup("RecordingFormats");
    config.beginWriteArray("formats");

    ei = 0;

    foreach (RecordingFormat recordingFormat, this->m_recordingFormats) {
        config.setArrayIndex(ei);
        config.setValue("format", recordingFormat.description());
        ei++;
    }

    config.endArray();
    config.endGroup();

    foreach (RecordingFormat recordingFormat, this->m_recordingFormats) {
        config.beginGroup("RecordingFormats_" + recordingFormat.description());
        config.setValue("params", recordingFormat.params());
        config.setValue("suffix", recordingFormat.suffix());
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
    config.setValue("qmlPluginPath", Qb::qmlPluginPath());
    config.setValue("recursive", QbElement::recursiveSearch());

    config.beginWriteArray("paths");

    i = 0;

    foreach (QString path, QbElement::searchPaths(QbElement::SearchPathsExtras)) {
        config.setArrayIndex(i);
        config.setValue("path", path);
        i++;
    }

    config.endArray();
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
