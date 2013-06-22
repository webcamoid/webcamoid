/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include <QtXml>
#include <KSharedConfig>
#include <KConfigGroup>

#include "mediatools.h"

MediaTools::MediaTools(QObject *parent): QObject(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

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
    QStringList pluginsPaths = Qb::pluginsPaths();

    foreach (QString pluginPath, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        pluginsPaths << pluginsDir.absoluteFilePath(pluginPath);

    Qb::setPluginsPaths(pluginsPaths);
#endif

    this->m_pipeline = Qb::create("Bin", "pipeline");

    if (this->m_pipeline)
    {
        QString description("MultiSrc objectName='source' loop=true "
                            "stateChanged>videoMux.setState "
                            "stateChanged>effects.setState "
                            "stateChanged>muxAudioInput.setState !"
//                            "Probe objectName='input' log=true source.stateChanged>setState !"
                            "Multiplex objectName='videoMux' "
                            "caps='video/x-raw' outputIndex=0 !"
                            "Bin objectName='effects' blocking=false !"
                            "VCapsConvert caps='video/x-raw,format=bgra' source.stateChanged>setState !"
                            "Sync source.stateChanged>setState !"
                            "OUT. ,"
                            "source. !"
                            "Multiplex objectName='muxAudioInput' "
                            "caps='audio/x-raw' outputIndex=0 !"
                            "Multiplex objectName='audioSwitch' "
                            "outputIndex=1 ,"
                            "muxAudioInput. !"
                            "AudioOutput objectName='audioOutput' ,"
                            "AudioInput objectName='mic' !"
                            "Multiplex outputIndex=1 "
                            "mic.stateChanged>setState ! audioSwitch. ,"
                            "effects. ! MultiSink objectName='record' ,"
                            "audioSwitch. ! record. ,"
                            "WebcamConfig objectName='webcamConfig'");

        this->m_pipeline->setProperty("description", description);

        this->m_effectsPreview = Qb::create("Bin");

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
                                  Q_RETURN_ARG(QbElementPtr, this->m_webcamConfig),
                                  Q_ARG(QString, "webcamConfig"));

        this->m_pipeline->link(this);

        if (this->m_source)
        {
            this->m_source->link(this->m_effectsPreview);

            QObject::connect(this->m_source.data(),
                             SIGNAL(error(QString)),
                             this,
                             SIGNAL(error(QString)));
        }

        if (this->m_webcamConfig)
            QObject::connect(this->m_webcamConfig.data(),
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
    QString sender = this->sender()->objectName();

    QImage frame(packet.buffer().data(),
                 packet.caps().property("width").toInt(),
                 packet.caps().property("height").toInt(),
                 QImage::Format_ARGB32);

    if (sender == "pipeline")
    {
        emit this->frameReady(frame);

        if (frame.size() != this->m_curFrameSize)
        {
            emit this->frameSizeChanged(frame.size());
            this->m_curFrameSize = frame.size();
        }
    }
    else
    {
        QString name = this->nameFromHash(sender);

        emit this->previewFrameReady(frame, name);
    }
}

QString MediaTools::device()
{
    return this->m_device;
}

QSize MediaTools::videoSize(const QString &device)
{
    QSize size;
    QString webcam = device.isEmpty()? this->device(): device;

    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "size", Qt::DirectConnection,
                              Q_RETURN_ARG(QSize, size),
                              Q_ARG(QString, webcam));

    return size;
}

bool MediaTools::effectsPreview()
{
    return this->m_showEffectsPreview;
}

bool MediaTools::playAudioFromSource()
{
    return this->m_playAudioFromSource;
}

MediaTools::RecordFrom MediaTools::recordAudioFrom()
{
    return this->m_recordAudioFrom;
}

bool MediaTools::recording()
{
    return this->m_recording;
}

QList<QStringList> MediaTools::videoRecordFormats()
{
    return this->m_videoRecordFormats;
}

QList<QStringList> MediaTools::streams()
{
    return this->m_streams;
}

QSize MediaTools::windowSize()
{
    return this->m_windowSize;
}

QVariantList MediaTools::videoSizes(const QString &device)
{
    QVariantList sizes;

    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "availableSizes", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariantList, sizes),
                              Q_ARG(QString, device));

    return sizes;
}

QList<QStringList> MediaTools::captureDevices()
{
    QStringList webcams;

    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "webcams", Qt::DirectConnection,
                              Q_RETURN_ARG(QStringList, webcams));

    QList<QStringList> webcamsDevices;

    foreach (QString webcam, webcams)
    {
        QString description;

        QMetaObject::invokeMethod(this->m_webcamConfig.data(),
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

QVariantList MediaTools::listControls(const QString &device)
{
    QVariantList controls;

    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "controls", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariantList, controls),
                              Q_ARG(QString, device));

    return controls;
}

void MediaTools::setControls(const QString &device, const QVariantMap &controls)
{
    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "setControls", Qt::DirectConnection,
                              Q_ARG(QString, device),
                              Q_ARG(QVariantMap, controls));
}

QMap<QString, QString> MediaTools::availableEffects()
{
    QMap<QString, QString> effects;

    QDomDocument effectsXml("effects");
    QFile xmlFile(":/webcamoid/share/effects.xml");
    xmlFile.open(QIODevice::ReadOnly);
    effectsXml.setContent(&xmlFile);
    xmlFile.close();

    QDomNodeList effectNodes = effectsXml.documentElement().childNodes();

    for (int effect = 0; effect < effectNodes.count(); effect++)
    {
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

QStringList MediaTools::currentEffects()
{
    return this->m_effectsList;
}

QString MediaTools::bestRecordFormatOptions(QString fileName)
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

QString MediaTools::hashFromName(QString name)
{
    return QString("x") + name.toUtf8().toHex();
}

QString MediaTools::nameFromHash(QString hash)
{
    return QByteArray::fromHex(hash.mid(1).toUtf8());
}

void MediaTools::setRecordAudioFrom(RecordFrom recordAudio)
{
    if (this->m_recordAudioFrom == recordAudio)
        return;

    if (!this->m_mic ||
        !this->m_audioSwitch ||
        !this->m_record)
    {
        this->m_recordAudioFrom = recordAudio;

        return;
    }

    if (recordAudio == RecordFromNone)
    {
        if (this->m_recordAudioFrom == RecordFromMic)
            this->m_mic->setState(QbElement::ElementStateNull);

        this->m_audioSwitch->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_record.data(),
                            SIGNAL(stateChanged(ElementState)),
                            this->m_audioSwitch.data(),
                            SLOT(setState(ElementState)));

        if (this->m_recordAudioFrom == RecordFromMic)
            QObject::disconnect(this->m_record.data(),
                                SIGNAL(stateChanged(ElementState)),
                                this->m_mic.data(),
                                SLOT(setState(ElementState)));
    }
    else
    {
        if (recordAudio == RecordFromSource)
        {
            if (this->m_recordAudioFrom == RecordFromMic)
            {
                this->m_mic->setState(QbElement::ElementStateNull);

                QObject::disconnect(this->m_record.data(),
                                    SIGNAL(stateChanged(ElementState)),
                                    this->m_mic.data(),
                                    SLOT(setState(ElementState)));
            }

            this->m_audioSwitch->setProperty("inputIndex", 0);
        }
        else if (recordAudio == RecordFromMic)
        {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_mic->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(ElementState)),
                             this->m_mic.data(),
                             SLOT(setState(ElementState)));

            this->m_audioSwitch->setProperty("inputIndex", 1);
        }

        if (this->m_recordAudioFrom == RecordFromNone)
        {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_audioSwitch->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(ElementState)),
                             this->m_audioSwitch.data(),
                             SLOT(setState(ElementState)));
        }
    }

    this->m_recordAudioFrom = recordAudio;
}

void MediaTools::setRecording(bool recording, QString fileName)
{
    if (!this->m_pipeline || !this->m_record)
    {
        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);

        return;
    }

    if (this->m_record->state() != QbElement::ElementStateNull)
    {
        this->m_record->setState(QbElement::ElementStateNull);
        this->m_audioSwitch->setState(QbElement::ElementStateNull);

        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);
    }

    if (recording)
    {
        QString options = this->bestRecordFormatOptions(fileName);

        if (options == "")
        {
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

        if (this->recordAudioFrom() != RecordFromNone)
            this->m_audioSwitch->setState(QbElement::ElementStatePlaying);
        else
            this->m_audioSwitch->setState(QbElement::ElementStateNull);

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

void MediaTools::setDevice(QString device)
{
    if (device.isEmpty())
    {
        this->resetRecording();
        this->resetEffectsPreview();

        if (this->m_source)
            this->m_source->setState(QbElement::ElementStateNull);

        this->m_device = "";
        emit this->deviceChanged(this->m_device);
    }
    else
    {
        if (!this->m_source)
            return;

        this->m_source->setProperty("location", device);

        int videoStream;

        QMetaObject::invokeMethod(this->m_source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, videoStream),
                                  Q_ARG(QString, "video/x-raw"));
        int audioStream;

        QMetaObject::invokeMethod(this->m_source.data(),
                                  "defaultStream", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, audioStream),
                                  Q_ARG(QString, "audio/x-raw"));

        QStringList streams;

        if (videoStream >= 0)
            streams << QString("%1").arg(videoStream);

        if (audioStream >= 0)
            streams << QString("%1").arg(audioStream);

        QMetaObject::invokeMethod(this->m_source.data(),
                                  "setFilterStreams", Qt::DirectConnection,
                                  Q_ARG(QStringList, streams));

        this->m_source->setState(QbElement::ElementStatePlaying);

        if (this->m_source->state() == QbElement::ElementStatePlaying)
            this->m_device = device;
        else
            this->m_device = "";

        emit this->deviceChanged(this->m_device);
    }
}

void MediaTools::setVideoSize(const QString &device, const QSize &size)
{
    QString curDevice = this->device();
    QbElement::ElementState state = this->m_source->state();

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                              "setSize", Qt::DirectConnection,
                              Q_ARG(QString, webcam),
                              Q_ARG(QSize, size));

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::setEffectsPreview(bool effectsPreview)
{
    this->m_showEffectsPreview = effectsPreview;

    if (!this->m_effectsPreview || !this->m_source)
        return;

    this->m_effectsPreview->setState(QbElement::ElementStateNull);

    if (effectsPreview && this->m_source->state() == QbElement::ElementStatePlaying)
    {
        QString description = this->m_effectsPreview->property("description").toString();

        if (description.isEmpty())
        {
            description = QString("IN. ! VCapsConvert objectName='preview' "
                                  "caps='video/x-raw,width=%1,height=%2'").arg(128)
                                                                          .arg(96);

            QStringList effects = this->availableEffects().keys();

            foreach (QString effect, effects)
            {
                QString previewHash = this->hashFromName(effect);

                description += QString(", preview. !"
                                       "%1 !"
                                       "VCapsConvert objectName='%2' "
                                       "caps='video/x-raw,format=bgra'").arg(effect)
                                                                        .arg(previewHash);
            }

            this->m_effectsPreview->setProperty("description", description);

            foreach (QString effect, effects)
            {
                QString previewHash = this->hashFromName(effect);
                QbElementPtr preview;

                QMetaObject::invokeMethod(this->m_effectsPreview.data(),
                                          "element", Qt::DirectConnection,
                                          Q_RETURN_ARG(QbElementPtr, preview),
                                          Q_ARG(QString, previewHash));

                preview->link(this);
            }
        }

        this->m_effectsPreview->setState(QbElement::ElementStatePlaying);
    }
}

void MediaTools::setPlayAudioFromSource(bool playAudio)
{
    this->m_playAudioFromSource = playAudio;

    if (!this->m_source || !this->m_audioOutput)
        return;

    QbElement::ElementState sourceState = this->m_source->state();

    if (playAudio)
    {
        if (sourceState == QbElement::ElementStatePlaying ||
            sourceState == QbElement::ElementStatePaused)
            this->m_audioOutput->setState(sourceState);

        QObject::connect(this->m_source.data(),
                         SIGNAL(stateChanged(ElementState)),
                         this->m_audioOutput.data(),
                         SLOT(setState(ElementState)));
    }
    else
    {
        this->m_audioOutput->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_source.data(),
                            SIGNAL(stateChanged(ElementState)),
                            this->m_audioOutput.data(),
                            SLOT(setState(ElementState)));
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

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    if (!webcam.isEmpty())
        QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                                  "resetSize", Qt::DirectConnection,
                                  Q_ARG(QString, webcam));

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::resetEffectsPreview()
{
    this->setEffectsPreview(false);
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

void MediaTools::loadConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");
    this->setPlayAudioFromSource(webcamConfigs.readEntry("playAudio", true));

    int recordFrom = webcamConfigs.readEntry("recordAudioFrom",
                                             static_cast<int>(RecordFromMic));

    this->setRecordAudioFrom(static_cast<RecordFrom>(recordFrom));

    QStringList windowSize = webcamConfigs.readEntry("windowSize", "320,240").split(",", QString::SkipEmptyParts);
    this->m_windowSize = QSize(windowSize.at(0).trimmed().toInt(),
                               windowSize.at(1).trimmed().toInt());

    KConfigGroup effectsConfigs = config->group("Effects");

    QString effcts = effectsConfigs.readEntry("effects", "");

    if (!effcts.isEmpty())
        this->setEffects(effcts.split("&&", QString::SkipEmptyParts));

    KConfigGroup videoFormatsConfigs = config->group("VideoRecordFormats");

    QString videoRecordFormats = videoFormatsConfigs.
                    readEntry("formats",
                              "webm::"
                              "-i 0 -vcodec libvpx -i 1 -acodec libvorbis -o -f webm&&"
                              "ogv, ogg::"
                              "-i 0 -vcodec libtheora -i 1 -acodec libvorbis -o -f ogg");

    if (!videoRecordFormats.isEmpty())
        foreach (QString fmt, videoRecordFormats.split("&&", QString::SkipEmptyParts))
        {
            QStringList params = fmt.split("::", QString::SkipEmptyParts);

            this->setVideoRecordFormat(params.at(0),
                                       params.at(1));
        }

    KConfigGroup streamsConfig = config->group("CustomStreams");
    QString streams = streamsConfig.readEntry("streams", "");

    if (!streams.isEmpty())
        foreach (QString fmt, streams.split("&&"))
        {
            QStringList params = fmt.split("::");

            this->setStream(params.at(0).trimmed(),
                            params.at(1).trimmed());
        }
}

void MediaTools::saveConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");

    webcamConfigs.writeEntry("playAudio", this->playAudioFromSource());

    webcamConfigs.writeEntry("recordAudioFrom",
                             static_cast<int>(this->recordAudioFrom()));

    webcamConfigs.writeEntry("windowSize", QString("%1,%2").arg(this->m_windowSize.width())
                                                           .arg(this->m_windowSize.height()));

    KConfigGroup effectsConfigs = config->group("Effects");

    effectsConfigs.writeEntry("effects", this->m_effectsList.join("&&"));

    KConfigGroup videoFormatsConfigs = config->group("VideoRecordFormats");

    QStringList videoRecordFormats;

    foreach (QStringList format, this->m_videoRecordFormats)
        videoRecordFormats << QString("%1::%2").arg(format[0])
                                               .arg(format[1]);

    videoFormatsConfigs.writeEntry("formats",
                                   videoRecordFormats.join("&&"));

    KConfigGroup streamsConfigs = config->group("CustomStreams");

    QStringList streams;

    foreach (QStringList stream, this->m_streams)
        streams << QString("%1::%2").arg(stream.at(0))
                                    .arg(stream.at(1));

    streamsConfigs.writeEntry("streams", streams.join("&&"));

    config->sync();
}

void MediaTools::setEffects(QStringList effects)
{
    if (this->m_effectsList == effects)
        return;

    this->m_effectsList = effects;

    if (this->m_effectsList.isEmpty())
        this->m_effects->setProperty("description", "");
    else
    {
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

void MediaTools::aboutToQuit()
{
    this->resetDevice();
    this->saveConfigs();
}

void MediaTools::reset(QString device)
{
    QString curDevice = this->device();
    QbElement::ElementState state = this->m_source->state();

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QString webcam = device.isEmpty()? curDevice: device;

    if (!webcam.isEmpty())
        QMetaObject::invokeMethod(this->m_webcamConfig.data(),
                                  "reset", Qt::DirectConnection,
                                  Q_ARG(QString, webcam));

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::webcamsChanged(const QStringList &webcams)
{
    Q_UNUSED(webcams)

    emit this->devicesModified();
}
