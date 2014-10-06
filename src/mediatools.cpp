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
    QObject::connect(QCoreApplication::instance(),
                     SIGNAL(aboutToQuit()),
                     this,
                     SLOT(aboutToQuit()));

    this->resetDevice();
    this->resetVideoSize("");
    this->resetEffectsPreview();
    this->resetPlayAudioFromSource();
    this->m_recordAudioFrom = RecordFromMic;
    this->resetRecording();
    this->resetVideoRecordFormats();
    this->resetStreams();
    this->resetWindowSize();

    Qb::init();

#ifdef QT_DEBUG
    QDir pluginsDir("Qb/Plugins");
    QStringList pluginsPaths = QbElement::searchPaths();

    foreach (QString pluginPath, pluginsDir.entryList(QDir::Dirs |
                                                      QDir::NoDotAndDotDot,
                                                      QDir::Name))
        pluginsPaths << pluginsDir.absoluteFilePath(pluginPath);

    QbElement::setSearchPaths(pluginsPaths);
#endif

    this->m_pipeline = QbElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QString description("MultiSrc objectName='source' loop=true "
                            "audioAlign=true "
                            "stateChanged>videoMux.setState "
                            "stateChanged>effects.setState "
                            "stateChanged>muxAudioInput.setState ! DirectConnection?"
                            "Multiplex objectName='videoMux' "
                            "caps='video/x-raw' outputIndex=0 !"
                            "VideoSync objectName='videoSync' "
                            "audioOutput.elapsedTime>setClock "
                            "source.stateChanged>setState "
                            "videoCapture.stateChanged>setState !"
                            "Bin objectName='effects' blocking=false !"
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
                            "effects. ! RtPts record.stateChanged>setState ! DirectConnection?"
                            "MultiSink objectName='record' ,"
                            "audioSwitch. ! record. ,"
                            "VideoCapture objectName='videoCapture' "
                            "stateChanged>videoMux.setState "
                            "stateChanged>effects.setState "
                            "stateChanged>muxAudioInput.setState !"
                            "DirectConnection? videoMux.");

        this->m_pipeline->setProperty("description", description);
        this->m_effectsPreview = QbElement::create("Bin", "effectPreview");
        this->m_effectsPreview->setProperty("blocking", true);

        this->m_applyPreview = QbElement::create("VCapsConvert", "applyPreview");
        this->m_applyPreview->setProperty("caps", "video/x-raw,format=bgra,width=128,height=96");

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_source),
                                  Q_ARG(QString, "source"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_effects),
                                  Q_ARG(QString, "effects"));

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
                             SLOT(sourceStateChanged(QbElement::ElementState)));
        }

        if (this->m_videoCapture) {
            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(error(const QString &)),
                             this,
                             SIGNAL(error(const QString &)));

            QObject::connect(this->m_videoCapture.data(),
                             SIGNAL(stateChanged(QbElement::ElementState)),
                             this,
                             SLOT(sourceStateChanged(QbElement::ElementState)));
        }

        this->m_effectsPreview->link(this);
        this->m_applyPreview->link(this);

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
    this->resetDevice();
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
    else if (sender == "effectPreview")
        emit this->effectPreviewReady(packet);
    else if (sender == "applyPreview")
        emit this->applyPreviewReady(packet);
}

void MediaTools::sourceStateChanged(QbElement::ElementState state)
{
    if (state == QbElement::ElementStateNull) {
        emit this->deviceChanged("");

        return;
    }

    emit this->deviceChanged(this->device());
}

QString MediaTools::device() const
{
    QString device;

    if (this->m_source)
        device = this->m_source->property("location").toString();

    if (device.isEmpty() && this->m_videoCapture)
        device = this->m_videoCapture->property("device").toString();

    return device;
}

QSize MediaTools::videoSize(const QString &device)
{
    QSize size;
    QString webcam = device.isEmpty()? this->device(): device;

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

QList<QStringList> MediaTools::streams() const
{
    return this->m_streams;
}

QSize MediaTools::windowSize() const
{
    return this->m_windowSize;
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

QList<QStringList> MediaTools::captureDevices()
{
    QStringList webcams;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "webcams", Qt::DirectConnection,
                              Q_RETURN_ARG(QStringList, webcams));

    QList<QStringList> webcamsDevices;

    foreach (QString webcam, webcams) {
        QString description;

        QMetaObject::invokeMethod(this->m_videoCapture.data(),
                                  "description", Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, webcam));

        QStringList cap;

        cap << webcam << description;
        webcamsDevices << cap;
    }

    QStringList desktopDevice;

    desktopDevice << ":0.0" << this->tr("Desktop");

    QList<QStringList> allDevices = webcamsDevices +
                                    this->m_streams +
                                    QList<QStringList>() << desktopDevice;

    return allDevices;
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

QMap<QString, QString> MediaTools::availableEffects() const
{
    QMap<QString, QString> effects;

    QDomDocument effectsXml("effects");
    QFile xmlFile(":/Webcamoid/share/effects.xml");
    xmlFile.open(QIODevice::ReadOnly);
    effectsXml.setContent(&xmlFile);
    xmlFile.close();

    QDomNodeList effectNodes = effectsXml.documentElement().childNodes();

    for (int effect = 0; effect < effectNodes.count(); effect++) {
        QDomNode effectNode = effectNodes.item(effect);

        if (!effectNode.isElement())
            continue;

        QDomNamedNodeMap attributtes = effectNode.attributes();
        QString effectName = attributtes.namedItem("name").nodeValue();
        QString effectDescription = effectNode.firstChild().toText().data();

        effects[effectDescription] = effectName;
    }

    return effects;
}

QStringList MediaTools::currentEffects() const
{
    return this->m_effectsList;
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

void MediaTools::setDevice(const QString &device)
{
    // Clear previous device.
    this->resetRecording();
    this->resetEffectsPreview();

    if (this->m_source) {
        this->m_source->setState(QbElement::ElementStateNull);
        this->m_source->setProperty("location", "");
    }

    if (this->m_videoCapture) {
        this->m_videoCapture->setState(QbElement::ElementStateNull);
        this->m_videoCapture->setProperty("device", "");
    }

    // Prepare the device.
    if (!device.isEmpty()) {
        if (!this->m_source || !this->m_videoCapture)
            return;

        bool isWebcam = QRegExp("/dev/video\\d*").exactMatch(device);

        // Set device.
        if (isWebcam) {
            if (this->m_source)
                this->m_source->setState(QbElement::ElementStateNull);

            this->m_videoCapture->setProperty("device", device);
        }
        else {
            if (this->m_videoCapture)
                this->m_videoCapture->setState(QbElement::ElementStateNull);

            this->m_source->setProperty("location", device);
        }

        // Find the defaults audio and video streams.
        int videoStream = -1;
        int audioStream = -1;

        if (isWebcam)
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
        if (isWebcam)
            QMetaObject::invokeMethod(this->m_source.data(),
                                      "setFilterStreams", Qt::DirectConnection,
                                      Q_ARG(QList<int>, streams));

        // Now setup the recording streams caps.
        QVariantMap streamCaps;

        if (isWebcam) {
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

        // Start capturing.
        if (isWebcam)
            QMetaObject::invokeMethod(this->m_videoCapture.data(),
                                      "setState",
                                      Q_ARG(QbElement::ElementState, QbElement::ElementStatePlaying));
        else
            QMetaObject::invokeMethod(this->m_source.data(),
                                      "setState",
                                      Q_ARG(QbElement::ElementState, QbElement::ElementStatePlaying));
    }
}

void MediaTools::setVideoSize(const QString &device, const QSize &size)
{
    QString curDevice = this->device();

    if (!curDevice.isEmpty() &&
        (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    QMetaObject::invokeMethod(this->m_videoCapture.data(),
                              "setSize", Qt::DirectConnection,
                              Q_ARG(QString, webcam),
                              Q_ARG(QSize, size));

    if (!curDevice.isEmpty() &&
        (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::setEffectsPreview(const QString &effect)
{
    if (!this->m_effectsPreview)
        return;

    this->m_effectsPreview->setState(QbElement::ElementStateNull);

    if (!effect.isEmpty() && !this->device().isEmpty()) {
        QString description = QString("IN. ! VCapsConvert "
                                      "caps='video/x-raw,width=%1,height=%2' !"
                                      "%3 !"
                                      "VCapsConvert "
                                      "caps='video/x-raw,format=bgra' ! OUT.").arg(128)
                                                                              .arg(96)
                                                                              .arg(effect);

        this->m_effectsPreview->setProperty("description", description);
        this->m_effectsPreview->setState(QbElement::ElementStatePlaying);
    }
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

void MediaTools::setStreams(QList<QStringList> streams)
{
    this->m_streams = streams;

    emit this->devicesModified();
}

void MediaTools::setWindowSize(QSize windowSize)
{
    this->m_windowSize = windowSize;
}

void MediaTools::resetDevice()
{
    this->setDevice("");
}

void MediaTools::resetVideoSize(const QString &device)
{
    QString curDevice = this->device();
    QbElement::ElementState state = this->m_source?
                                        this->m_source->state():
                                        QbElement::ElementStateNull;

    if (state == QbElement::ElementStatePlaying &&
        (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    if (!webcam.isEmpty())
        QMetaObject::invokeMethod(this->m_videoCapture.data(),
                                  "resetSize", Qt::DirectConnection,
                                  Q_ARG(QString, webcam));

    if (state == QbElement::ElementStatePlaying &&
        (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::resetEffectsPreview()
{
    this->setEffectsPreview("");
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

void MediaTools::resetStreams()
{
    this->setStreams(QList<QStringList>());
}

void MediaTools::resetWindowSize()
{
    this->setWindowSize(QSize());
}

void MediaTools::connectPreview(bool link)
{
    if (link) {
        if (this->m_source)
            this->m_source->link(this->m_effectsPreview);

        if (this->m_videoCapture)
            this->m_videoCapture->link(this->m_effectsPreview);

        if (this->m_effects)
            this->m_effects->link(this->m_applyPreview);
    }
    else {
        if (this->m_source)
            this->m_source->unlink(this->m_effectsPreview);

        if (this->m_videoCapture)
            this->m_videoCapture->unlink(this->m_effectsPreview);

        if (this->m_effects)
            this->m_effects->unlink(this->m_applyPreview);
    }
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");

    this->setPlayAudioFromSource(config.value("playAudio", true).toBool());

    int recordFrom = config.value("recordAudioFrom",
                                  static_cast<int>(RecordFromMic)).toInt();

    this->setRecordAudioFrom(static_cast<RecordFrom>(recordFrom));

    this->m_windowSize = config.value("windowSize", QSize(320, 240)).toSize();

    config.endGroup();

    config.beginGroup("Effects");
    int size = config.beginReadArray("effects");
    QStringList effects;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        effects << config.value("effect").toString();
    }

    this->setEffects(effects);
    config.endArray();
    config.endGroup();

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
        QString devName = config.value("dev").toString();
        QString description = config.value("description").toString();
        this->setStream(devName, description);
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

    for (int i = 0; i < this->m_effectsList.size(); i++) {
        config.setArrayIndex(i);
        QString effect = this->m_effectsList[i];
        config.setValue("effect", effect);
    }

    config.endArray();
    config.endGroup();

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

    for (int i = 0; i < this->m_streams.size(); i++) {
        config.setArrayIndex(i);
        QStringList stream = this->m_streams[i];
        config.setValue("dev", stream[0]);
        config.setValue("description", stream[1]);
    }

    config.endArray();
    config.endGroup();
}

void MediaTools::setEffects(const QStringList &effects)
{
    if (this->m_effectsList == effects)
        return;

    this->m_effectsList = effects;

    if (this->m_effectsList.isEmpty())
        this->m_effects->setProperty("description", "");
    else {
        QString description = "IN.";

        foreach (QString effect, this->m_effectsList)
            description += QString(" ! %1").arg(effect);

        description += " ! OUT.";

        this->m_effects->setProperty("description", description);
    }
}

void MediaTools::clearVideoRecordFormats()
{
    this->m_videoRecordFormats.clear();
}

void MediaTools::setStream(QString dev_name, QString description)
{
    this->m_streams << (QStringList() << dev_name
                                      << description);

    emit this->devicesModified();
}

void MediaTools::setVideoRecordFormat(QString suffix, QString options)
{
    this->m_videoRecordFormats << (QStringList() << suffix
                                   << options);
}

void MediaTools::cleanAll()
{
    this->resetDevice();
    this->saveConfigs();
}

void MediaTools::aboutToQuit()
{
    this->cleanAll();
}

void MediaTools::reset(const QString &device)
{
    QString curDevice = this->device();
    QbElement::ElementState state = this->m_source->state();

    if (state == QbElement::ElementStatePlaying &&
        (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    if (!webcam.isEmpty())
        QMetaObject::invokeMethod(this->m_videoCapture.data(),
                                  "reset", Qt::DirectConnection,
                                  Q_ARG(QString, webcam));

    if (state == QbElement::ElementStatePlaying &&
        (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::webcamsChanged(const QStringList &webcams)
{
    Q_UNUSED(webcams)

    emit this->devicesModified();
}
