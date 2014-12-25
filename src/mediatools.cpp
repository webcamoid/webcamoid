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

#include <QtXml>

#include "mediatools.h"

MediaTools::MediaTools(QObject *parent): QObject(parent)
{
    this->resetCurStream();
    this->resetVideoSize("");
    this->resetPlayAudioFromSource();
    this->m_recordAudioFrom = RecordFromMic;
    this->resetRecording();
    this->resetVideoRecordFormats();
    this->resetStreams();
    this->resetWindowSize();

    Qb::init();

    QSettings config;

    config.beginGroup("PluginConfigs");
    QbElement::setRecursiveSearch(config.value("recursive", false).toBool());

    int size = config.beginReadArray("paths");
    QStringList searchPaths;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        searchPaths << config.value("path").toString();
    }

    config.endArray();
    config.endGroup();

    if (!searchPaths.isEmpty())
        QbElement::setSearchPaths(searchPaths);

    this->m_pipeline = QbElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QString description("MultiSrc objectName='source' loop=true "
                            "audioAlign=true "
                            "stateChanged>videoMux.setState "
                            "stateChanged>muxAudioInput.setState ! DirectConnection?"
                            "Multiplex objectName='videoMux' "
                            "caps='video/x-raw' outputIndex=0 !"
                            "VideoSync objectName='videoSync' "
                            "audioOutput.elapsedTime>setClock "
                            "source.stateChanged>setState "
                            "videoCapture.stateChanged>setState !"
                            "VCapsConvert objectName='videoConvert' "
                            "caps='video/x-raw,format=bgra' "
                            "source.stateChanged>setState "
                            "videoCapture.stateChanged>setState ,"
                            "source. ! DirectConnection?"
                            "Multiplex objectName='muxAudioInput' "
                            "caps='audio/x-raw' outputIndex=0 !"
                            "Multiplex objectName='audioSwitch' "
                            "outputIndex=1 ,"
                            "muxAudioInput. ! DirectConnection?"
                            "AudioOutput objectName='audioOutput' ,"
                            "AudioInput objectName='mic' !"
                            "Multiplex outputIndex=1 "
                            "mic.stateChanged>setState ! audioSwitch. ,"
                            "videoConvert. ! RtPts record.stateChanged>setState ! DirectConnection?"
                            "MultiSink objectName='record' ,"
                            "audioSwitch. ! record. ,"
                            "VideoCapture objectName='videoCapture' "
                            "stateChanged>videoMux.setState "
                            "stateChanged>muxAudioInput.setState !"
                            "DirectConnection? videoMux.");

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
                                  Q_RETURN_ARG(QbElementPtr, this->m_videoSync),
                                  Q_ARG(QString, "videoSync"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_videoConvert),
                                  Q_ARG(QString, "videoConvert"));

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

        if (this->m_audioSwitch)
            this->m_audioSwitch->setProperty("inputIndex", 1);

        if (this->m_videoCapture)
            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(webcamsChanged(const QStringList &)),
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

    if (sender == "videoConvert")
        emit this->frameReady(packet);
}

QString MediaTools::curStream() const
{
    return this->m_curStream;
}

QSize MediaTools::videoSize(const QString &device)
{
    QSize size;
    QString webcam = device.isEmpty()? this->m_curStream: device;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "size", Qt::DirectConnection,
                              Q_RETURN_ARG(QSize, size),
                              Q_ARG(QString, webcam));

    return size;
}

bool MediaTools::playAudioFromSource() const
{
    return this->m_playAudioFromSource;
}

MediaTools::RecordFrom MediaTools::recordAudioFrom() const
{
    return this->m_recordAudioFrom;
}

bool MediaTools::recording() const
{
    return this->m_recording;
}

QList<QStringList> MediaTools::videoRecordFormats() const
{
    return this->m_videoRecordFormats;
}

QStringList MediaTools::streams() const
{
    QStringList streams;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "webcams",
                              Q_RETURN_ARG(QStringList, streams));

    streams << ":0.0";

    return streams + this->m_streams.keys();
}

QSize MediaTools::windowSize() const
{
    return this->m_windowSize;
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

    if (QRegExp(":\\d+\\.\\d+(?:\\+\\d+,\\d+)?").exactMatch(stream))
        return this->tr("Desktop") + " " + stream;

    if (this->m_streams.contains(stream))
        return this->m_streams[stream];

    return "";
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
                              "webcams",
                              Q_RETURN_ARG(QStringList, webcams));

    return webcams.contains(stream);
}

QVariantList MediaTools::videoSizes(const QString &device)
{
    QVariantList sizes;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "availableSizes", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariantList, sizes),
                              Q_ARG(QString, device));

    return sizes;
}

QVariantList MediaTools::listImageControls(const QString &device)
{
    QVariantList controls;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "imageControls", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariantList, controls),
                              Q_ARG(QString, device));

    return controls;
}

void MediaTools::setImageControls(const QString &device, const QVariantMap &controls)
{
    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "setImageControls", Qt::DirectConnection,
                              Q_ARG(QString, device),
                              Q_ARG(QVariantMap, controls));
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
        return "";

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
        this->m_videoSync->unlink(this->m_videoConvert);
        this->m_videoSync->link(effect);
    }
    else {
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
                if (this->m_effectsList.size() == 1)
                    this->m_videoSync->link(this->m_videoConvert);
                else
                    this->m_videoSync->link(this->m_effectsList[i + 1]);
            }
            else if (i == this->m_effectsList.size() - 1)
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
    QString ext = QFileInfo(fileName).completeSuffix();

    if (ext.isEmpty())
        return "";

    foreach (QStringList format, this->m_videoRecordFormats)
        foreach (QString s, format[0].split(",", QString::SkipEmptyParts))
            if (s.toLower().trimmed() == ext)
                return format[1];

    return "";
}

bool MediaTools::isPlaying()
{
    if (this->m_source
        && this->m_source->state() != QbElement::ElementStateNull)
        return true;

    if (this->m_videoCapture
        && this->m_videoCapture->state() != QbElement::ElementStateNull)
        return true;

    return false;
}

QString MediaTools::fileNameFromUri(const QString &uri) const
{
    QFileInfo fileInfo(uri);

    return fileInfo.baseName();
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

bool MediaTools::embedInterface(QQmlApplicationEngine *engine,
                                QObject *interface,
                                const QString &where) const
{
    if (!engine || !interface)
        return false;

    foreach (QObject *obj, engine->rootObjects()) {
        // First, find where to enbed the UI.
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

void MediaTools::setRecordAudioFrom(RecordFrom recordAudio)
{
    if (this->m_recordAudioFrom == recordAudio)
        return;

    if (!this->m_mic ||
        !this->m_audioSwitch ||
        !this->m_record) {
        this->m_recordAudioFrom = recordAudio;

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
    }
    else {
        if (recordAudio == RecordFromSource) {
            if (this->m_recordAudioFrom == RecordFromMic) {
                this->m_mic->setState(QbElement::ElementStateNull);

                QObject::disconnect(this->m_record.data(),
                                    SIGNAL(stateChanged(QbElement::ElementState)),
                                    this->m_mic.data(),
                                    SLOT(setState(QbElement::ElementState)));
            }

            this->m_audioSwitch->setProperty("inputIndex", 0);
        }
        else if (recordAudio == RecordFromMic) {
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
}

void MediaTools::setRecording(bool recording, QString fileName)
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

        if (this->recordAudioFrom() != RecordFromNone) {
            this->m_audioSwitch->setState(QbElement::ElementStatePlaying);

            if (this->recordAudioFrom() == RecordFromMic)
                this->m_mic->setState(QbElement::ElementStatePlaying);
        }
        else {
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

bool MediaTools::start()
{
    if (this->m_curStream.isEmpty()
        || !this->m_source
        || !this->m_videoCapture)
        return false;

    bool isCamera = this->isCamera(this->m_curStream);

    // Find the defaults audio and video streams.
    int videoStream = -1;
    int audioStream = -1;

    if (isCamera)
        videoStream = 0;
    else {
        QMetaObject::invokeMethod(this->m_source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(QString, "video/x-raw"));

        QMetaObject::invokeMethod(this->m_source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, audioStream),
                                  Q_ARG(QString, "audio/x-raw"));
    }

    QList<int> streams;

    if (videoStream >= 0)
        streams << videoStream;

    if (audioStream >= 0)
        streams << audioStream;

    // Only decode the default streams.
    if (isCamera)
        QMetaObject::invokeMethod(this->m_source.data(),
                                  "setFilterStreams", Qt::DirectConnection,
                                  Q_ARG(QList<int>, streams));

    // Now setup the recording streams caps.
    QVariantMap streamCaps;

    if (isCamera) {
        QString videoCaps;

        QMetaObject::invokeMethod(this->m_videoCapture.data(),
                                  "caps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, videoCaps));

        streamCaps["0"] = videoCaps;
    }
    else
        QMetaObject::invokeMethod(this->m_source.data(),
                                  "streamCaps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap, streamCaps));

    QVariantMap recordStreams;

    // Stream 0 = Video.
    if (videoStream >= 0)
        recordStreams["0"] = streamCaps[QString("%1").arg(videoStream)];

    // Stream 1 = Audio.
    if (this->recordAudioFrom() == RecordFromMic) {
        QString audioCaps;

        QMetaObject::invokeMethod(this->m_mic.data(),
                                  "streamCaps", Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, audioCaps));

        recordStreams["1"] = audioCaps;
    }
    else if (this->recordAudioFrom() == RecordFromSource &&
             audioStream >= 0)
        recordStreams["1"] = streamCaps[QString("%1").arg(audioStream)];

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
        || !this->m_videoCapture)
        return false;

    if (this->isCamera(this->m_curStream))
        this->m_videoCapture->setState(QbElement::ElementStatePlaying);
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
}

void MediaTools::setCurStream(const QString &stream)
{
    this->m_curStream = stream;

    if (!this->m_source || !this->m_videoCapture) {
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
        this->m_videoCapture->setProperty("device", stream);
    else
        this->m_source->setProperty("location", stream);

    if (isPlaying)
        this->startStream();

    emit this->curStreamChanged();
}

void MediaTools::setVideoSize(const QString &stream, const QSize &size)
{
    bool isPlaying = this->isPlaying();

    if (this->m_curStream == stream
        || stream.isEmpty())
        this->stopStream();

    QString camera = stream.isEmpty()? this->m_curStream: stream;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "setSize",
                              Q_ARG(QString, camera),
                              Q_ARG(QSize, size));

    if (isPlaying
        && (stream.isEmpty()
            || this->m_curStream == stream))
        this->startStream();
}

void MediaTools::setPlayAudioFromSource(bool playAudio)
{
    this->m_playAudioFromSource = playAudio;

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
    }
    else {
        this->m_audioOutput->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_source.data(),
                            SIGNAL(stateChanged(QbElement::ElementState)),
                            this->m_audioOutput.data(),
                            SLOT(setState(QbElement::ElementState)));
    }
}

void MediaTools::setVideoRecordFormats(QList<QStringList> videoRecordFormats)
{
    this->m_videoRecordFormats = videoRecordFormats;
}

void MediaTools::setWindowSize(const QSize &windowSize)
{
    this->m_windowSize = windowSize;
}

void MediaTools::setAppEngine(QQmlApplicationEngine *engine)
{
    this->m_appEngine = engine;
    Qb::qmlRegister(engine);
}

void MediaTools::resetCurStream()
{
    this->setCurStream("");
}

void MediaTools::resetVideoSize(const QString &stream)
{
    bool isPlaying = this->isPlaying();

    if (this->m_curStream == stream
        || stream.isEmpty())
        this->stopStream();

    QString camera = stream.isEmpty()? this->m_curStream: stream;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "resetSize",
                              Q_ARG(QString, camera));

    if (isPlaying
        && (stream.isEmpty()
            || this->m_curStream == stream))
        this->startStream();
}

void MediaTools::resetPlayAudioFromSource()
{
    this->setPlayAudioFromSource(true);
}

void MediaTools::resetRecordAudioFrom()
{
    this->setRecordAudioFrom(RecordFromMic);
}

void MediaTools::resetRecording()
{
    this->setRecording(false);
}

void MediaTools::resetVideoRecordFormats()
{
    this->setVideoRecordFormats(QList<QStringList>());
}

void MediaTools::resetWindowSize()
{
    this->setWindowSize(QSize());
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");

    this->setPlayAudioFromSource(config.value("playAudio", true).toBool());

    int recordFrom = config.value("recordAudioFrom",
                                  static_cast<int>(RecordFromMic)).toInt();

    this->setRecordAudioFrom(static_cast<RecordFrom>(recordFrom));

    this->m_windowSize = config.value("windowSize", QSize(800, 480)).toSize();

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

        config.beginGroup(effectId);

        foreach (QString key, config.allKeys())
            effect->setProperty(key.toStdString().c_str(), config.value(key));

        config.endGroup();
    }

    config.beginGroup("VideoRecordFormats");
    size = config.beginReadArray("formats");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        QString format = config.value("format").toString();
        QString params = config.value("params").toString();
        this->setVideoRecordFormat(format, params);
    }

    if (size < 1) {
        this->setVideoRecordFormat("webm",
                                   "-i 0 -opt quality=realtime -c:v libvpx -b:v 3M -i 1 -c:a libvorbis -o -f webm");

        this->setVideoRecordFormat("ogv, ogg",
                                   "-i 0 -c:v libtheora -b:v 3M -i 1 -c:a libvorbis -o -f ogg");
    }

    config.endArray();
    config.endGroup();

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

    config.setValue("recordAudioFrom",
                    static_cast<int>(this->recordAudioFrom()));

    config.setValue("windowSize", this->m_windowSize);

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
        config.beginGroup(effect->pluginId());

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

    config.beginGroup("VideoRecordFormats");
    config.beginWriteArray("formats");

    for (int i = 0; i < this->m_videoRecordFormats.size(); i++) {
        config.setArrayIndex(i);
        QStringList format = this->m_videoRecordFormats[i];
        config.setValue("format", format[0]);
        config.setValue("params", format[1]);
    }

    config.endArray();
    config.endGroup();

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
    config.setValue("recursive", QbElement::recursiveSearch());

    config.beginWriteArray("paths");

    i = 0;

    foreach (QString path, QbElement::searchPaths()) {
        config.setArrayIndex(i);
        config.setValue("path", path);
        i++;
    }

    config.endArray();
    config.endGroup();
}

void MediaTools::clearVideoRecordFormats()
{
    this->m_videoRecordFormats.clear();
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

void MediaTools::setVideoRecordFormat(QString suffix, QString options)
{
    this->m_videoRecordFormats << (QStringList() << suffix
                                   << options);
}

void MediaTools::cleanAll()
{
    this->resetCurStream();
    this->saveConfigs();
}

void MediaTools::reset(const QString &stream)
{
    bool isPlaying = this->isPlaying();

    if (this->m_curStream == stream
        || stream.isEmpty())
        this->stopStream();

    QString camera = stream.isEmpty()? this->m_curStream: stream;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "reset", Qt::DirectConnection,
                              Q_ARG(QString, camera));

    if (isPlaying
        && (stream.isEmpty()
            || this->m_curStream == stream))
        this->startStream();
}

void MediaTools::webcamsChanged(const QStringList &webcams)
{
    Q_UNUSED(webcams)

    emit this->streamsChanged();
}
