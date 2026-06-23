/* Webcamoid, camera capture application.
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent>

#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#include "videolayer.h"

using ObjectPtr = QSharedPointer<QObject>;

class VideoLayerPrivate
{
    public:
        VideoLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QString m_videoInput;
        QStringList m_inputs;
        QStringList m_sources;
        QStringList m_availableScreens;
        QStringList m_availableWindows;
        QStringList m_screens;
        QStringList m_windows;
        QMap<QString, QString> m_images;
        QMap<QString, QString> m_uris;
        QStringList m_supportedFileFormats;
        QStringList m_supportedImageFormats;
        QMap<QString, QString> m_formatsDescription;
        AkAudioCaps m_inputAudioCaps;
        AkVideoCaps m_inputVideoCaps;
        AkElementPtr m_cameraCapture {akPluginManager->create<AkElement>("VideoSource/CameraCapture")};
        AkElementPtr m_screenCapture {akPluginManager->create<AkElement>("VideoSource/DesktopCapture")};
        AkElementPtr m_imageCapture {akPluginManager->create<AkElement>("VideoSource/ImageSrc")};
        AkElementPtr m_uriCapture {akPluginManager->create<AkElement>("MultimediaSource/MultiSrc")};
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_playOnStart {true};
        bool m_outputsAsInputs {false};

        explicit VideoLayerPrivate(VideoLayer *self);
        void connectSignals();
        AkElementPtr sourceElement(const QString &stream) const;
        QString sourceId(const QString &stream) const;
        QStringList cameras() const;
        QStringList screens() const;
        QStringList windows() const;
        QString cameraDescription(const QString &camera) const;
        QString screenDescription(const QString &desktop) const;
        bool embedControls(const QString &where,
                           const AkElementPtr &element,
                           const QString &pluginId,
                           const QString &name) const;
        void setInputAudioCaps(const AkAudioCaps &audioCaps);
        void setInputVideoCaps(const AkVideoCaps &videoCaps);
        void loadProperties();
        static QString sanitizeKey(const QString &key);
        void saveVideoInput(const QString &videoInput);
        void saveSources();
        void savePlayOnStart(bool playOnStart);
        void saveOutputsAsInputs(bool outputsAsInputs);
};

VideoLayer::VideoLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerPrivate(this);
    this->setQmlEngine(engine);
    this->d->connectSignals();
    this->d->loadProperties();
}

VideoLayer::~VideoLayer()
{
    this->setState(AkElement::ElementStateNull);
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

QString VideoLayer::videoInput() const
{
    return this->d->m_videoInput;
}

QStringList VideoLayer::inputs() const
{
    return this->d->m_inputs;
}

AkAudioCaps VideoLayer::inputAudioCaps() const
{
    return this->d->m_inputAudioCaps;
}

AkVideoCaps VideoLayer::inputVideoCaps() const
{
    return this->d->m_inputVideoCaps;
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
    if (!this->d->m_screenCapture)
        return false;

    return this->d->m_screenCapture->property("canCaptureWindows").toBool();
}

QStringList VideoLayer::supportedFileFormats() const
{
    return this->d->m_supportedFileFormats;
}

AkElement::ElementState VideoLayer::state() const
{
    return this->d->m_state;
}

bool VideoLayer::isTorchSupported() const
{
    if (!this->d->m_cameraCapture)
        return false;

    return this->d->m_cameraCapture->property("isTorchSupported").toBool();
}

VideoLayer::TorchMode VideoLayer::torchMode() const
{
    if (!this->d->m_cameraCapture)
        return Torch_Off;

    return this->d->m_cameraCapture->property("torchMode").value<TorchMode>();
}

VideoLayer::PermissionStatus VideoLayer::cameraPermissionStatus() const
{
    if (!this->d->m_cameraCapture)
        return PermissionStatus_Granted;

    return this->d->m_cameraCapture->property("permissionStatus").value<PermissionStatus>();
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
    if (this->d->cameras().contains(device))
        return InputCamera;

    if (this->d->m_availableScreens.contains(device)
        || this->d->m_availableWindows.contains(device))
        return InputScreen;

    if (this->d->m_images.contains(device))
        return InputImage;

    if (this->d->m_uris.contains(device))
        return InputStream;

    return InputUnknown;
}

QStringList VideoLayer::devicesByType(InputType type) const
{
    switch (type) {
    case InputCamera:
        return this->d->cameras();

    case InputScreen:
        return this->d->m_screens + this->d->m_windows;

    case InputImage:
        return this->d->m_images.keys();

    case InputStream:
        return this->d->m_uris.keys();

    default:
        break;
    }

    return {};
}

QString VideoLayer::description(const QString &device) const
{
    if (this->d->cameras().contains(device))
        return this->d->cameraDescription(device);

    if (this->d->m_availableScreens.contains(device)
        || this->d->m_availableWindows.contains(device)) {
        return this->d->screenDescription(device);
    }

    if (this->d->m_images.contains(device))
        return this->d->m_images.value(device);

    if (this->d->m_uris.contains(device))
        return this->d->m_uris.value(device);

    return {};
}

QString VideoLayer::inputError() const
{
    auto source = this->d->sourceElement(this->d->m_videoInput);

    if (!source)
        return {};

    QString error;
    QMetaObject::invokeMethod(source.data(),
                              "error",
                              Q_RETURN_ARG(QString, error));

    return error;
}

bool VideoLayer::embedInputControls(const QString &where,
                                    const QString &device,
                                    const QString &name) const
{
    auto element = this->d->sourceElement(device);
    auto id = this->d->sourceId(device);

    return this->d->embedControls(where, element, id, name);
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

void VideoLayer::setInputStream(const QString &stream,
                                const QString &description)
{
    if (stream.isEmpty()
        || description.isEmpty()
        || this->d->m_uris.value(stream) == description
        || this->d->m_images.value(stream) == description) {
        return;
    }

    QFileInfo fileInfo(stream);

    if (!fileInfo.exists())
        return;

    auto suffix = fileInfo.suffix().toLower();

    if (!this->d->m_supportedFileFormats.contains(suffix))
        return;

    if (this->d->m_supportedImageFormats.contains(suffix))
        this->d->m_images[stream] = description;
    else
        this->d->m_uris[stream] = description;

    if (!this->d->m_sources.contains(stream))
        this->d->m_sources << stream;

    this->updateInputs();
    this->d->saveSources();
}

void VideoLayer::removeInputStream(const QString &stream)
{
    if (stream.isEmpty()
        || (!this->d->m_images.contains(stream)
            && !this->d->m_uris.contains(stream)))
        return;

    this->d->m_images.remove(stream);
    this->d->m_uris.remove(stream);
    this->d->m_sources.removeAll(stream);
    this->updateInputs();
    this->d->saveSources();

#ifdef Q_OS_ANDROID
    if (QFile::exists(stream))
        QFile::remove(stream);
#endif
}

bool VideoLayer::addScreenSource(const QString &source)
{
    if (this->d->m_sources.contains(source))
        return false;

    if (this->d->m_availableScreens.contains(source)) {
        if (!this->d->m_screens.contains(source))
            this->d->m_screens << source;
    } else if (this->d->m_availableWindows.contains(source)) {
        if (!this->d->m_windows.contains(source))
            this->d->m_windows << source;
    } else {
        return false;
    }

    this->d->m_sources << source;
    this->updateInputs();
    this->d->saveSources();

    return true;
}

void VideoLayer::removeScreenSource(const QString &source)
{
    this->d->m_screens.removeAll(source);
    this->d->m_windows.removeAll(source);
    this->d->m_sources.removeAll(source);
    this->updateInputs();
    this->d->saveSources();
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
    if (this->d->m_state == state)
        return;

    AkElementPtr source;

    if (this->d->cameras().contains(this->d->m_videoInput)) {
        if (this->d->m_screenCapture)
            this->d->m_screenCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_imageCapture)
            this->d->m_imageCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_cameraCapture;
    } else if (this->d->m_screens.contains(this->d->m_videoInput)
               || this->d->m_windows.contains(this->d->m_videoInput)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_imageCapture)
            this->d->m_imageCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_screenCapture;
    } else if (this->d->m_images.contains(this->d->m_videoInput)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_screenCapture)
            this->d->m_screenCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_uriCapture)
            this->d->m_uriCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_imageCapture;
    } else if (this->d->m_uris.contains(this->d->m_videoInput)) {
        if (this->d->m_cameraCapture)
            this->d->m_cameraCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_screenCapture)
            this->d->m_screenCapture->setState(AkElement::ElementStateNull);

        if (this->d->m_imageCapture)
            this->d->m_imageCapture->setState(AkElement::ElementStateNull);

        source = this->d->m_uriCapture;
    }

    if (source) {
        if (source->setState(state)
            || source->state() != this->d->m_state) {
            auto state = source->state();
            this->d->m_state = state;
            emit this->stateChanged(state);
        }
    } else {
        if (this->d->m_state != AkElement::ElementStateNull) {
            this->d->m_state = AkElement::ElementStateNull;
            emit this->stateChanged(AkElement::ElementStateNull);
        }
    }
}

void VideoLayer::setTorchMode(TorchMode mode)
{
    if (this->d->m_cameraCapture)
        this->d->m_cameraCapture->setProperty("torchMode", mode);
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
    this->updateInputs();
}

void VideoLayer::resetVideoInput()
{
    this->setVideoInput({});
}

void VideoLayer::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoLayer::resetTorchMode()
{
    this->setTorchMode(Torch_Off);
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
        engine->rootContext()->setContextProperty("videoLayer", this);
        qRegisterMetaType<InputType>("VideoInputType");
        qRegisterMetaType<TorchMode>("TorchMode");
        qRegisterMetaType<PermissionStatus>("PermissionStatus");
        qmlRegisterType<VideoLayer>("Webcamoid", 1, 0, "VideoLayer");
    }
}

void VideoLayer::updateInputs()
{
    QStringList inputs;

    if (this->d->m_screenCapture)
        QMetaObject::invokeMethod(this->d->m_screenCapture.data(),
                                  "updateDevices");

    // List the virtual camera outputs
    QStringList videoOutputs;
    QStringList videoOutputsDescription;
    auto cameraOutput = akPluginManager->create<AkElement>("VideoSink/VirtualCamera");

    if (cameraOutput && !this->d->m_outputsAsInputs) {
        videoOutputs = cameraOutput->property("medias").toStringList();

        for (auto &output: videoOutputs) {
            QString description;
            QMetaObject::invokeMethod(cameraOutput.data(),
                                      "description",
                                      Q_RETURN_ARG(QString, description),
                                      Q_ARG(QString, output));
            videoOutputsDescription << output;
        }
    }

    // Read cameras
    for (const auto &camera: this->d->cameras()) {
        auto description = this->d->cameraDescription(camera);

        // Do not include the virtual camera outputs to prevent self blocking.
        if (!videoOutputs.contains(camera)
            && !videoOutputsDescription.contains(description))
            inputs << camera;
    }

    // Append other sources bellow the cameras
    inputs << this->d->m_sources;

    // Update the available screens
    auto availableScreens = this->d->screens();

    if  (availableScreens != this->d->m_availableScreens) {
        this->d->m_availableScreens = availableScreens;
        emit this->screensChanged(this->d->m_availableScreens);
    }

    // Update the available windows
    auto availableWindows = this->d->windows();

    if  (availableWindows != this->d->m_availableWindows) {
        this->d->m_availableWindows = availableWindows;
        emit this->windowsChanged(this->d->m_availableWindows);
    }

    // Update inputs
    if (inputs != this->d->m_inputs) {
        this->d->m_inputs = inputs;
        emit this->inputsChanged(this->d->m_inputs);

        if (!this->d->m_inputs.contains(this->d->m_videoInput))
            this->setVideoInput(this->d->m_inputs.value(0));
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
                                      Q_ARG(AkCaps::CapsType, AkCaps::CapsAudio));
            QMetaObject::invokeMethod(source.data(),
                                      "defaultStream",
                                      Q_RETURN_ARG(int, videoStream),
                                      Q_ARG(AkCaps::CapsType, AkCaps::CapsVideo));

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
            for (auto &stream: streams) {
                AkCaps caps;
                QMetaObject::invokeMethod(source.data(),
                                          "caps",
                                          Q_RETURN_ARG(AkCaps, caps),
                                          Q_ARG(int, stream));

                if (caps.type() == AkCaps::CapsAudio)
                    audioCaps = caps;
                else if (caps.type() == AkCaps::CapsVideo)
                    videoCaps = caps;
            }
        }
    }

    this->setState(state);
    this->d->setInputAudioCaps(audioCaps);
    this->d->setInputVideoCaps(videoCaps);
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
    this->m_supportedFileFormats =
        supportedVideoFormats + QStringList(supportedImageFormats.begin(),
                                            supportedImageFormats.end());
}

void VideoLayerPrivate::connectSignals()
{
    if (this->m_cameraCapture) {
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(oStream(AkPacket)),
                         self,
                         SIGNAL(oStream(AkPacket)),
                         Qt::DirectConnection);
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(mediasChanged(QStringList)),
                         self,
                         SLOT(updateInputs()));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(errorChanged(QString)),
                         self,
                         SIGNAL(inputErrorChanged(QString)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(streamsChanged(QList<int>)),
                         self,
                         SLOT(updateCaps()));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(isTorchSupportedChanged(bool)),
                         self,
                         SIGNAL(isTorchSupportedChanged(bool)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(torchModeChanged(TorchMode)),
                         self,
                         SIGNAL(torchModeChanged(TorchMode)));
        QObject::connect(this->m_cameraCapture.data(),
                         SIGNAL(permissionStatusChanged(PermissionStatus)),
                         self,
                         SIGNAL(cameraPermissionStatusChanged(PermissionStatus)));
    }

    if (this->m_screenCapture) {
        QObject::connect(this->m_screenCapture.data(),
                         SIGNAL(oStream(AkPacket)),
                         self,
                         SIGNAL(oStream(AkPacket)),
                         Qt::DirectConnection);
        QObject::connect(this->m_screenCapture.data(),
                         SIGNAL(mediasChanged(QStringList)),
                         self,
                         SLOT(updateInputs()));
        QObject::connect(this->m_screenCapture.data(),
                         SIGNAL(error(QString)),
                         self,
                         SIGNAL(inputErrorChanged(QString)));
        QObject::connect(this->m_screenCapture.data(),
                         SIGNAL(streamsChanged(QList<int>)),
                         self,
                         SLOT(updateCaps()));
        QObject::connect(this->m_screenCapture.data(),
                         SIGNAL(canCaptureWindowsChanged(bool)),
                         self,
                         SIGNAL(canCaptureWindowsChanged(bool)));
    }

    if (this->m_imageCapture) {
        QObject::connect(this->m_imageCapture.data(),
                         SIGNAL(oStream(AkPacket)),
                         self,
                         SIGNAL(oStream(AkPacket)),
                         Qt::DirectConnection);
        QObject::connect(this->m_imageCapture.data(),
                         SIGNAL(error(QString)),
                         self,
                         SIGNAL(inputErrorChanged(QString)));
        QObject::connect(this->m_imageCapture.data(),
                         SIGNAL(streamsChanged(QList<int>)),
                         self,
                         SLOT(updateCaps()));
        this->m_supportedImageFormats =
                this->m_imageCapture->property("supportedFormats").toStringList();
    }

    if (this->m_uriCapture) {
        this->m_uriCapture->setProperty("loop", true);

        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(oStream(AkPacket)),
                         self,
                         SIGNAL(oStream(AkPacket)),
                         Qt::DirectConnection);
        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(error(QString)),
                         self,
                         SIGNAL(inputErrorChanged(QString)));
        QObject::connect(this->m_uriCapture.data(),
                         SIGNAL(streamsChanged(QList<int>)),
                         self,
                         SLOT(updateCaps()));
    }
}

AkElementPtr VideoLayerPrivate::sourceElement(const QString &stream) const
{
    if (this->cameras().contains(stream))
        return this->m_cameraCapture;

    if (this->m_screens.contains(stream)
        || this->m_windows.contains(stream)) {
        return this->m_screenCapture;
    }

    if (this->m_images.contains(stream))
        return this->m_imageCapture;

    if (this->m_uris.contains(stream))
        return this->m_uriCapture;

    return {};
}

QString VideoLayerPrivate::sourceId(const QString &stream) const
{
    if (this->cameras().contains(stream))
        return {"VideoSource/CameraCapture"};

    if (this->m_screens.contains(stream)
        || this->m_windows.contains(stream)) {
        return {"VideoSource/DesktopCapture"};
    }

    if (this->m_images.contains(stream))
        return {"VideoSource/ImageSrc"};

    if (this->m_uris.contains(stream))
        return {"MultimediaSource/MultiSrc"};

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

QStringList VideoLayerPrivate::screens() const
{
    if (!this->m_screenCapture)
        return {};

    QStringList screens;

    QStringList medias;
    QMetaObject::invokeMethod(this->m_screenCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, medias));

    for (const auto &media: medias) {
        bool isWindow = false;
        QMetaObject::invokeMethod(this->m_screenCapture.data(),
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
    if (!this->m_screenCapture)
        return {};

    QStringList windows;

    QStringList medias;
    QMetaObject::invokeMethod(this->m_screenCapture.data(),
                              "medias",
                              Q_RETURN_ARG(QStringList, medias));

    for (const auto &media: medias) {
        bool isWindow = false;
        QMetaObject::invokeMethod(this->m_screenCapture.data(),
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
    if (!this->m_cameraCapture)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_cameraCapture.data(),
                              "description",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, camera));

    return description;
}

QString VideoLayerPrivate::screenDescription(const QString &desktop) const
{
    if (!this->m_screenCapture)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->m_screenCapture.data(),
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
    if (!element)
        return false;

    auto controlInterface = element->controlInterface(this->m_engine, pluginId);

    if (!controlInterface)
        return false;

    if (!name.isEmpty())
        controlInterface->setObjectName(name);

    for (auto &obj: this->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(controlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
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

void VideoLayerPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("StreamConfigs");
    this->m_videoInput = config.value("stream").toString();
    this->m_playOnStart = config.value("playOnStart", true).toBool();

    // Read media URIs and files
    int size = config.beginReadArray("sources");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        auto source = config.value("source").toString();
        auto description = config.value("description").toString();

        if (source.startsWith("screen://")) {
            if (!this->screens().contains(source))
                continue;

            this->m_screens << source;
        } else if (source.startsWith("window://")) {
            if (!this->windows().contains(source))
                continue;

            this->m_windows << source;
        } else {
            QFileInfo fileInfo(source);

            if (!fileInfo.exists())
                continue;

            auto suffix = fileInfo.suffix().toLower();

            if (!this->m_supportedFileFormats.contains(suffix))
                continue;

            if (this->m_supportedImageFormats.contains(suffix))
                this->m_images[source] = description;
            else
                this->m_uris[source] = description;
        }

        this->m_sources << source;
    }

    config.endArray();
    config.endGroup();

    self->updateInputs();
    self->updateCaps();
}

QString VideoLayerPrivate::sanitizeKey(const QString &key)
{
    QString sanitized(key);

    return sanitized.replace(" ", "_")
                    .replace(".", "_")
                    .replace(",", "_");
}

void VideoLayerPrivate::saveVideoInput(const QString &videoInput)
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.setValue("stream", videoInput);
    config.endGroup();
}

void VideoLayerPrivate::saveSources()
{
    QSettings config;
    config.beginGroup("StreamConfigs");
    config.beginWriteArray("sources");

    int i = 0;

    for (const auto &source: this->m_sources) {
        config.setArrayIndex(i);
        config.setValue("source", source);
        config.setValue("description", self->description(source));
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
    config.beginGroup("VirtualCamera");
    config.setValue("loopback", outputsAsInputs);
    config.endGroup();
}

#include "moc_videolayer.cpp"
