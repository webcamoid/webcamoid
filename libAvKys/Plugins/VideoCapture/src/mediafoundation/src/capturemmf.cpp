/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <QtDebug>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QMap>
#include <QSize>
#include <QDateTime>
#include <QVariant>
#include <QMutex>
#include <QWaitCondition>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <strmif.h>
#include <dbt.h>
#include <uuids.h>

#include "capturemmf.h"

#define TIME_BASE 1.0e7

Q_CORE_EXPORT HINSTANCE qWinAppInst();

__inline bool operator <(REFGUID guid1, REFGUID guid2)
{
    return guid1.Data1 < guid2.Data1;
}

using VideoProcAmpPropertyMap = QMap<VideoProcAmpProperty, QString>;

inline VideoProcAmpPropertyMap initVideoProcAmpPropertyMap()
{
    VideoProcAmpPropertyMap vpapToStr {
        {VideoProcAmp_Brightness           , "Brightness"            },
        {VideoProcAmp_Contrast             , "Contrast"              },
        {VideoProcAmp_Hue                  , "Hue"                   },
        {VideoProcAmp_Saturation           , "Saturation"            },
        {VideoProcAmp_Sharpness            , "Sharpness"             },
        {VideoProcAmp_Gamma                , "Gamma"                 },
        {VideoProcAmp_ColorEnable          , "Color Enable"          },
        {VideoProcAmp_WhiteBalance         , "White Balance"         },
        {VideoProcAmp_BacklightCompensation, "Backlight Compensation"},
        {VideoProcAmp_Gain                 , "Gain"                  }
    };

    return vpapToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(VideoProcAmpPropertyMap, vpapToStr, (initVideoProcAmpPropertyMap()))

using CameraControlMap = QMap<CameraControlProperty, QString>;

inline CameraControlMap initCameraControlMap()
{
    CameraControlMap ccToStr {
        {CameraControl_Pan     , "Pan"     },
        {CameraControl_Tilt    , "Tilt"    },
        {CameraControl_Roll    , "Roll"    },
        {CameraControl_Zoom    , "Zoom"    },
        {CameraControl_Exposure, "Exposure"},
        {CameraControl_Iris    , "Iris"    },
        {CameraControl_Focus   , "Focus"   }
    };

    return ccToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CameraControlMap, ccToStr, (initCameraControlMap()))

using GuidToStrMap = QMap<GUID, QString>;

inline GuidToStrMap initGuidToStrMap()
{
    GuidToStrMap guidToStr {
        {MEDIASUBTYPE_CLPL               , "CLPL"},
        {MEDIASUBTYPE_YUYV               , "YUYV"},
        {MEDIASUBTYPE_IYUV               , "IYUV"},
        {MEDIASUBTYPE_YVU9               , "YVU9"},
        {MEDIASUBTYPE_Y411               , "Y411"},
        {MEDIASUBTYPE_Y41P               , "Y41P"},
        {MEDIASUBTYPE_YUY2               , "YUY2"},
        {MEDIASUBTYPE_YVYU               , "YVYU"},
        {MEDIASUBTYPE_UYVY               , "UYVY"},
        {MEDIASUBTYPE_Y211               , "Y211"},
        {MEDIASUBTYPE_CLJR               , "CLJR"},
        {MEDIASUBTYPE_IF09               , "IF09"},
        {MEDIASUBTYPE_CPLA               , "CPLA"},
        {MEDIASUBTYPE_MJPG               , "MJPG"},
        {MEDIASUBTYPE_TVMJ               , "TVMJ"},
        {MEDIASUBTYPE_WAKE               , "WAKE"},
        {MEDIASUBTYPE_CFCC               , "CFCC"},
        {MEDIASUBTYPE_IJPG               , "IJPG"},
        {MEDIASUBTYPE_Plum               , "Plum"},
        {MEDIASUBTYPE_DVCS               , "DVCS"},
        {MEDIASUBTYPE_DVSD               , "DVSD"},
        {MEDIASUBTYPE_MDVF               , "MDVF"},
        {MEDIASUBTYPE_RGB1               , "RGB1"},
        {MEDIASUBTYPE_RGB4               , "RGB4"},
        {MEDIASUBTYPE_RGB8               , "RGB8"},
        {MEDIASUBTYPE_RGB565             , "RGBP"},
        {MEDIASUBTYPE_RGB555             , "RGBO"},
        {MEDIASUBTYPE_RGB24              , "RGB3"},
        {MEDIASUBTYPE_RGB32              , "RGB4"},
        {MEDIASUBTYPE_ARGB1555           , "AR15"},
        {MEDIASUBTYPE_ARGB4444           , "AR12"},
        {MEDIASUBTYPE_ARGB32             , "BA24"},
        {MEDIASUBTYPE_AYUV               , "AYUV"},
        {MEDIASUBTYPE_AI44               , "AI44"},
        {MEDIASUBTYPE_IA44               , "IA44"},
        {MEDIASUBTYPE_RGB32_D3D_DX7_RT   , "7R32"},
        {MEDIASUBTYPE_RGB16_D3D_DX7_RT   , "7R16"},
        {MEDIASUBTYPE_ARGB32_D3D_DX7_RT  , "7A88"},
        {MEDIASUBTYPE_ARGB4444_D3D_DX7_RT, "7A44"},
        {MEDIASUBTYPE_ARGB1555_D3D_DX7_RT, "7A15"},
        {MEDIASUBTYPE_RGB32_D3D_DX9_RT   , "9R32"},
        {MEDIASUBTYPE_RGB16_D3D_DX9_RT   , "9R16"},
        {MEDIASUBTYPE_ARGB32_D3D_DX9_RT  , "9A88"},
        {MEDIASUBTYPE_ARGB4444_D3D_DX9_RT, "9A44"},
        {MEDIASUBTYPE_ARGB1555_D3D_DX9_RT, "9A15"},
        {MEDIASUBTYPE_YV12               , "YV12"},
        {MEDIASUBTYPE_NV12               , "NV12"},
        {MEDIASUBTYPE_IMC1               , "IMC1"},
        {MEDIASUBTYPE_IMC2               , "IMC2"},
        {MEDIASUBTYPE_IMC3               , "IMC3"},
        {MEDIASUBTYPE_IMC4               , "IMC4"},
        {MEDIASUBTYPE_S340               , "S340"},
        {MEDIASUBTYPE_S342               , "S342"},
        {MEDIASUBTYPE_QTRpza             , "rpza"},
        {MEDIASUBTYPE_QTSmc              , "smc "},
        {MEDIASUBTYPE_QTRle              , "rle "},
        {MEDIASUBTYPE_QTJpeg             , "jpeg"},
        {MEDIASUBTYPE_dvsd               , "dvsd"},
        {MEDIASUBTYPE_dvhd               , "dvhd"},
        {MEDIASUBTYPE_dvsl               , "dvsl"},
        {MEDIASUBTYPE_dv25               , "dv25"},
        {MEDIASUBTYPE_dv50               , "dv50"},
        {MEDIASUBTYPE_dvh1               , "dvh1"}
    };

    return guidToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(GuidToStrMap, guidToStr, (initGuidToStrMap()))

using IoMethodMap = QMap<CaptureMMF::IoMethod, QString>;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr {
        {CaptureMMF::IoMethodSync , "sync" },
        {CaptureMMF::IoMethodASync, "async"}
    };

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

using ActivatePtr = QSharedPointer<IMFActivate>;
using MediaSourcePtr = QSharedPointer<IMFMediaSource>;
using SourceReaderPtr = QSharedPointer<IMFSourceReader>;
using MediaTypeHandlerPtr = QSharedPointer<IMFMediaTypeHandler>;
using MediaTypePtr = QSharedPointer<IMFMediaType>;

class CaptureMMFPrivate
{
    public:
        QStringList m_webcams;
        QString m_device;
        QList<int> m_streams;
        qint64 m_id {-1};
        DWORD m_streamIndex {DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM)};
        CaptureMMF::IoMethod m_ioMethod {CaptureMMF::IoMethodSync};
        MediaSourcePtr m_mediaSource;
        SourceReaderPtr m_sourceReader;
        QMutex m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        QVector<ActivatePtr> sources() const;
        ActivatePtr source(const QString &sourceId) const;
        MediaSourcePtr mediaSource(const QString &sourceId) const;
        QVector<MediaTypeHandlerPtr> streams(const QString &webcam) const;
        QVector<MediaTypeHandlerPtr> streams(IMFMediaSource *mediaSource) const;
        MediaTypeHandlerPtr stream(IMFMediaSource *mediaSource,
                                   DWORD streamIndex) const;
        QVector<MediaTypePtr> mediaTypes(IMFMediaTypeHandler *mediaTypeHandler) const;
        MediaTypePtr mediaType(IMFMediaTypeHandler *mediaTypeHandler,
                               DWORD mediaTypeIndex) const;
        bool indexFromTrack(IMFMediaSource *mediaSource,
                            int track, DWORD *stream, DWORD *mediaType);
        static void deleteActivate(IMFActivate *activate);
        static void deleteMediaSource(IMFMediaSource *mediaSource);
        static void deleteSourceReader(IMFSourceReader *sourceReader);
        static void deleteMediaTypeHandler(IMFMediaTypeHandler *mediaTypeHandler);
        static void deleteMediaType(IMFMediaType *mediaType);
        AkCaps capsFromMediaType(IMFMediaType *mediaType) const;
        QVariantList imageControls(IUnknown *device) const;
        bool setImageControls(IUnknown *device,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(IUnknown *device) const;
        bool setCameraControls(IUnknown *device,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
};

CaptureMMF::CaptureMMF(QObject *parent):
    Capture(parent),
    QAbstractNativeEventFilter()
{
    this->d = new CaptureMMFPrivate;
    qApp->installNativeEventFilter(this);
}

CaptureMMF::~CaptureMMF()
{
    qApp->removeNativeEventFilter(this);
    delete this->d;
}

QStringList CaptureMMF::webcams() const
{
    QStringList webcams;
    auto sources = this->d->sources();

    for (auto &source: sources) {
        WCHAR *deviceId = nullptr;

        if (FAILED(source->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                              &deviceId,
                                              nullptr))) {
            continue;
        }

        webcams << QString::fromWCharArray(deviceId);
        CoTaskMemFree(deviceId);
    }

    return webcams;
}

QString CaptureMMF::device() const
{
    return this->d->m_device;
}

QList<int> CaptureMMF::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> CaptureMMF::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    auto caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureMMF::ioMethod() const
{
    return ioMethodToStr->value(this->d->m_ioMethod, "any");
}

int CaptureMMF::nBuffers() const
{
    return 0;
}

QString CaptureMMF::description(const QString &webcam) const
{
    auto source = this->d->source(webcam);

    if (!source)
        return QString();

    QString description;
    WCHAR *friendlyName = nullptr;

    if (SUCCEEDED(source->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                             &friendlyName,
                                             nullptr))) {
        description = QString::fromWCharArray(friendlyName);
        CoTaskMemFree(friendlyName);
    }

    return description;
}

QVariantList CaptureMMF::caps(const QString &webcam) const
{
    QVariantList caps;

    for (auto &stream: this->d->streams(webcam))
        for (auto &mediaType: this->d->mediaTypes(stream.data())) {
            auto videoCaps = this->d->capsFromMediaType(mediaType.data());

            if (videoCaps)
                caps << QVariant::fromValue(videoCaps);
        }

    return caps;
}

QString CaptureMMF::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureMMF::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureMMF::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lock();
    auto globalImageControls = this->d->m_globalImageControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        QVariantList control = globalImageControls[i].toList();
        QString controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalImageControls == globalImageControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalImageControls = globalImageControls;
    this->d->m_controlsMutex.unlock();

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool CaptureMMF::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureMMF::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureMMF::setCameraControls(const QVariantMap &cameraControls)
{
    this->d->m_controlsMutex.lock();
    auto globalCameraControls = this->d->m_globalCameraControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalCameraControls == globalCameraControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalCameraControls = globalCameraControls;
    this->d->m_controlsMutex.unlock();
    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool CaptureMMF::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureMMF::readFrame()
{
    if (!this->d->m_sourceReader)
        return AkPacket();

    this->d->m_controlsMutex.lock();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setImageControls(this->d->m_mediaSource.data(), controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lock();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls) {
        auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                         cameraControls);
        this->d->setCameraControls(this->d->m_mediaSource.data(), controls);
        this->d->m_localCameraControls = cameraControls;
    }

    DWORD actualStreamIndex = MF_SOURCE_READER_FIRST_VIDEO_STREAM;
    DWORD streamFlags = 0;
    LONGLONG timeStamp = 0;
    IMFSample *sample = nullptr;

    if (FAILED(this->d->m_sourceReader->ReadSample(this->d->m_streamIndex,
                                                   0,
                                                   &actualStreamIndex,
                                                   &streamFlags,
                                                   &timeStamp,
                                                   &sample))) {
        return AkPacket();
    }

    if (!sample)
        return AkPacket();

    DWORD bufferCount = 0;
    sample->GetBufferCount(&bufferCount);

    if (bufferCount < 1) {
        sample->Release();

        return AkPacket();
    }

    // Read stream caps.
    auto stream = this->d->stream(this->d->m_mediaSource.data(),
                                  actualStreamIndex);
    IMFMediaType *mediaType = nullptr;
    stream->GetCurrentMediaType(&mediaType);
    AkCaps caps = this->d->capsFromMediaType(mediaType);
    mediaType->Release();

    // Read buffer.
    IMFMediaBuffer *buffer = nullptr;
    sample->GetBufferByIndex(0, &buffer);

    DWORD bufferLength = 0;
    buffer->GetMaxLength(&bufferLength);

    QByteArray oBuffer(int(bufferLength), 0);
    BYTE  *data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;

    buffer->Lock(&data, &maxLength, &currentLength);
    memcpy(oBuffer.data(),
           data,
           size_t(maxLength));
    buffer->Unlock();
    buffer->Release();

    // Read pts.
    LONGLONG sampleTime = 0;
    sample->GetSampleTime(&sampleTime);
    sample->Release();

    // Send packet.
    AkPacket packet(caps);
    packet.setBuffer(oBuffer);
    packet.setPts(sampleTime);
    packet.setTimeBase(AkFrac(1, TIME_BASE));
    packet.setIndex(0);
    packet.setId(this->d->m_id);

    return packet;
}

bool CaptureMMF::nativeEventFilter(const QByteArray &eventType,
                                     void *message,
                                     long *result)
{
    Q_UNUSED(eventType)

    if (!message)
        return false;

    auto msg = reinterpret_cast<MSG *>(message);

    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVNODES_CHANGED: {
            auto webcams = this->webcams();

            if (webcams != this->d->m_webcams) {
                emit this->webcamsChanged(webcams);

                this->d->m_webcams = webcams;
            }

            if (result)
                *result = TRUE;

            return true;
        }
        default:
            break;
        }
    }

    return false;
}

bool CaptureMMF::init()
{
    if (FAILED(MFStartup(MF_VERSION)))
        return false;

    auto mediaSource = this->d->mediaSource(this->d->m_device);

    if (!mediaSource)
        return false;

    IMFAttributes *attributes = nullptr;

    if (FAILED(MFCreateAttributes(&attributes, 0)))
        return false;

    attributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN,
                          TRUE);

    bool ok = false;
    auto streams = this->d->m_streams;

    DWORD streamIndex = MF_SOURCE_READER_FIRST_VIDEO_STREAM;
    DWORD mediaTypeIndex = 0;
    IMFSourceReader *sourceReader = nullptr;

    if (FAILED(MFCreateSourceReaderFromMediaSource(mediaSource.data(),
                                                   attributes,
                                                   &sourceReader)))
        goto init_failed;

    this->d->m_sourceReader =
            SourceReaderPtr(sourceReader,
                            CaptureMMFPrivate::deleteSourceReader);
    this->d->m_sourceReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,
                                                FALSE);

    if (this->d->m_streams.isEmpty())
        streams << 0;

    for (auto &stream: streams) {
        if (!this->d->indexFromTrack(mediaSource.data(),
                                     stream, &streamIndex, &mediaTypeIndex))
            goto init_failed;

        this->d->m_sourceReader->SetStreamSelection(streamIndex, TRUE);
        auto mediaStream = this->d->stream(mediaSource.data(), streamIndex);

        if (!mediaStream)
            goto init_failed;

        auto mediaType = this->d->mediaType(mediaStream.data(), mediaTypeIndex);

        if (!mediaType)
            goto init_failed;

        this->d->m_sourceReader->SetCurrentMediaType(streamIndex,
                                                     nullptr,
                                                     mediaType.data());

        /* In theory we can handle more than one stream at same time, it can
         * be implemented in the future, but for now we will configure just
         * the first and break the loop.
         */
        break;
    }

    this->d->m_mediaSource = mediaSource;
    this->d->m_id = Ak::id();
    this->d->m_streamIndex = streamIndex;
    ok = true;

init_failed:
    if (!ok)
        this->uninit();

    attributes->Release();

    return ok;
}

void CaptureMMF::uninit()
{
    this->d->m_sourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
    this->d->m_sourceReader.clear();
    this->d->m_mediaSource.clear();
    MFShutdown();
}

void CaptureMMF::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lock();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();
        auto mediaSource = this->d->mediaSource(device);

        if (mediaSource) {
            this->d->m_globalImageControls = this->d->imageControls(mediaSource.data());
            this->d->m_globalCameraControls = this->d->cameraControls(mediaSource.data());
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void CaptureMMF::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureMMF::setIoMethod(const QString &ioMethod)
{
    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodSync);

    if (this->d->m_ioMethod == ioMethodEnum)
        return;

    this->d->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void CaptureMMF::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void CaptureMMF::resetDevice()
{
    this->setDevice("");
}

void CaptureMMF::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureMMF::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureMMF::resetNBuffers()
{
}

void CaptureMMF::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}



QVector<ActivatePtr> CaptureMMFPrivate::sources() const
{
    QVector<ActivatePtr> sources;
    IMFAttributes *attributes = nullptr;
    IMFActivate **deviceSources = nullptr;
    UINT32 nSources = 0;

    if (FAILED(MFCreateAttributes(&attributes, 1)))
        goto sources_failed;

    if (FAILED(attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                   MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
        goto sources_failed;

    if (FAILED(MFEnumDeviceSources(attributes, &deviceSources, &nSources)))
        goto sources_failed;

    for (UINT32 i = 0; i < nSources; i++)
        sources << ActivatePtr(deviceSources[i],
                               CaptureMMFPrivate::deleteActivate);

sources_failed:
    if (nSources)
        CoTaskMemFree(deviceSources);

    if (attributes)
        attributes->Release();

    return sources;
}

ActivatePtr CaptureMMFPrivate::source(const QString &sourceId) const
{
    ActivatePtr activate;
    auto sources = this->sources();

    for (auto &source: sources) {
        WCHAR *deviceId = nullptr;

        if (FAILED(source->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                              &deviceId,
                                              nullptr))) {
            continue;
        }

        auto id = QString::fromWCharArray(deviceId);
        CoTaskMemFree(deviceId);

        if (id == sourceId) {
            activate = source;

            break;
        }
    }

    return activate;
}

MediaSourcePtr CaptureMMFPrivate::mediaSource(const QString &sourceId) const
{
    auto source = this->source(sourceId);

    if (!source)
        return {};

    IMFMediaSource *mediaSource = nullptr;

    if (FAILED(source->ActivateObject(IID_IMFMediaSource,
                                      reinterpret_cast<void **>(&mediaSource))))
        return MediaSourcePtr();

    return MediaSourcePtr(mediaSource, CaptureMMFPrivate::deleteMediaSource);
}

QVector<MediaTypeHandlerPtr> CaptureMMFPrivate::streams(const QString &webcam) const
{
    auto mediaSource = this->mediaSource(webcam);

    return this->streams(mediaSource.data());
}

QVector<MediaTypeHandlerPtr> CaptureMMFPrivate::streams(IMFMediaSource *mediaSource) const
{
    QVector<MediaTypeHandlerPtr> streams;

    if (!mediaSource)
        return streams;

    DWORD nDescriptors = 0;
    IMFPresentationDescriptor *presentationDescriptor = nullptr;

    if (FAILED(mediaSource->CreatePresentationDescriptor(&presentationDescriptor)))
        goto streams_failed;

    if (FAILED(presentationDescriptor->GetStreamDescriptorCount(&nDescriptors)))
        goto streams_failed;

    for (DWORD i = 0; i < nDescriptors; i++) {
        BOOL selected = FALSE;
        IMFStreamDescriptor *descriptor = nullptr;

        if (FAILED(presentationDescriptor->GetStreamDescriptorByIndex(i,
                                                                      &selected,
                                                                      &descriptor)))
            continue;

        IMFMediaTypeHandler *mediaTypeHandler = nullptr;
        auto result = descriptor->GetMediaTypeHandler(&mediaTypeHandler);
        descriptor->Release();

        if (FAILED(result))
            continue;

        streams << MediaTypeHandlerPtr(mediaTypeHandler,
                                       CaptureMMFPrivate::deleteMediaTypeHandler);
    }

streams_failed:
    if (presentationDescriptor)
        presentationDescriptor->Release();

    return streams;
}

MediaTypeHandlerPtr CaptureMMFPrivate::stream(IMFMediaSource *mediaSource,
                                              DWORD streamIndex) const
{
    if (!mediaSource)
        return {};

    IMFPresentationDescriptor *presentationDescriptor = nullptr;

    if (FAILED(mediaSource->CreatePresentationDescriptor(&presentationDescriptor)))
        return {};

    MediaTypeHandlerPtr stream;
    IMFMediaTypeHandler *mediaTypeHandler = nullptr;
    HRESULT result = E_FAIL;

    BOOL selected = FALSE;
    IMFStreamDescriptor *descriptor = nullptr;

    if (FAILED(presentationDescriptor->GetStreamDescriptorByIndex(streamIndex,
                                                                  &selected,
                                                                  &descriptor)))
        goto stream_failed;

    result = descriptor->GetMediaTypeHandler(&mediaTypeHandler);
    descriptor->Release();

    if (FAILED(result))
        goto stream_failed;

    stream = MediaTypeHandlerPtr(mediaTypeHandler,
                                 CaptureMMFPrivate::deleteMediaTypeHandler);

stream_failed:
    presentationDescriptor->Release();

    return stream;
}

QVector<MediaTypePtr> CaptureMMFPrivate::mediaTypes(IMFMediaTypeHandler *mediaTypeHandler) const
{
    QVector<MediaTypePtr> mediaTypes;

    if (!mediaTypeHandler)
        return mediaTypes;

    DWORD nMediaTypes = 0;

    if (FAILED(mediaTypeHandler->GetMediaTypeCount(&nMediaTypes)))
        nMediaTypes = 0;

    for (DWORD i = 0; i < nMediaTypes; i++) {
        IMFMediaType *mediaType = nullptr;

        if (FAILED(mediaTypeHandler->GetMediaTypeByIndex(i, &mediaType)))
            continue;

        mediaTypes << MediaTypePtr(mediaType,
                                   CaptureMMFPrivate::deleteMediaType);
    }

    return mediaTypes;
}

MediaTypePtr CaptureMMFPrivate::mediaType(IMFMediaTypeHandler *mediaTypeHandler,
                                          DWORD mediaTypeIndex) const
{
    if (!mediaTypeHandler)
        return MediaTypePtr();

    IMFMediaType *mediaType = nullptr;

    if (FAILED(mediaTypeHandler->GetMediaTypeByIndex(mediaTypeIndex,
                                                     &mediaType)))
        return MediaTypePtr();

    return MediaTypePtr(mediaType, CaptureMMFPrivate::deleteMediaType);
}

bool CaptureMMFPrivate::indexFromTrack(IMFMediaSource *mediaSource,
                                       int track,
                                       DWORD *stream,
                                       DWORD *mediaType)
{
    if (stream)
        *stream = MF_SOURCE_READER_FIRST_VIDEO_STREAM;

    if (mediaType)
        *mediaType = 0;

    auto streams = this->streams(mediaSource);

    for (int j = 0; j < streams.count(); j++) {
        auto mediaTypes = this->mediaTypes(streams[j].data());

        for (int i = 0; i < mediaTypes.count(); i++) {
            auto videoCaps = this->capsFromMediaType(mediaTypes[i].data());

            if (videoCaps) {
                if (!track) {
                    if (stream)
                        *stream = DWORD(j);

                    if (mediaType)
                        *mediaType = DWORD(i);

                    return true;
                }

                track--;
            }
        }
    }

    return false;
}

void CaptureMMFPrivate::deleteActivate(IMFActivate *activate)
{
    activate->Release();
}

void CaptureMMFPrivate::deleteMediaSource(IMFMediaSource *mediaSource)
{
    mediaSource->Shutdown();
    mediaSource->Release();
}

void CaptureMMFPrivate::deleteSourceReader(IMFSourceReader *sourceReader)
{
    sourceReader->Release();
}

void CaptureMMFPrivate::deleteMediaTypeHandler(IMFMediaTypeHandler *mediaTypeHandler)
{
    mediaTypeHandler->Release();
}

void CaptureMMFPrivate::deleteMediaType(IMFMediaType *mediaType)
{
    mediaType->Release();
}

AkCaps CaptureMMFPrivate::capsFromMediaType(IMFMediaType *mediaType) const
{
    if (!mediaType)
        return AkCaps();

    GUID majorType;
    memset(&majorType, 0, sizeof(GUID));

    if (FAILED(mediaType->GetMajorType(&majorType)))
        return AkCaps();

    if (!IsEqualGUID(majorType, MFMediaType_Video))
        return AkCaps();

    GUID subtype;
    memset(&subtype, 0, sizeof(GUID));
    mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);

    QString fourcc = guidToStr->value(subtype);

    if (fourcc.isEmpty())
        return AkCaps();

    UINT32 width = 0;
    UINT32 height = 0;
    MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);

    UINT32 fpsNum = 0;
    UINT32 fpsDen = 0;
    MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");
    videoCaps.setProperty("fourcc", fourcc);
    videoCaps.setProperty("width", int(width));
    videoCaps.setProperty("height", int(height));
    AkFrac fps(fpsNum, fpsDen);
    videoCaps.setProperty("fps", fps.toString());

    return videoCaps;
}

QVariantList CaptureMMFPrivate::imageControls(IUnknown *device) const
{
    if (!device)
        return QVariantList();

    qint32 min;
    qint32 max;
    qint32 step;
    qint32 defaultValue;
    qint32 flags;
    qint32 value;

    QVariantList controls;
    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (auto &property: vpapToStr->keys()) {
            if (SUCCEEDED(pProcAmp->GetRange(property,
                                             reinterpret_cast<LONG *>(&min),
                                             reinterpret_cast<LONG *>(&max),
                                             reinterpret_cast<LONG *>(&step),
                                             reinterpret_cast<LONG *>(&defaultValue),
                                             reinterpret_cast<LONG *>(&flags))))
                if (SUCCEEDED(pProcAmp->Get(property,
                                            reinterpret_cast<LONG *>(&value),
                                            reinterpret_cast<LONG *>(&flags)))) {
                    QVariantList control;

                    QString type;

                    if (property == VideoProcAmp_ColorEnable
                        || property == VideoProcAmp_BacklightCompensation)
                        type = "boolean";
                    else
                        type = "integer";

                    control << vpapToStr->value(property)
                            << type
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
                }
        }

        pProcAmp->Release();
    }

    return controls;
}

bool CaptureMMFPrivate::setImageControls(IUnknown *device,
                                         const QVariantMap &imageControls) const
{
    if (!device)
        return false;

    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (auto &property: vpapToStr->keys()) {
            QString propertyStr = vpapToStr->value(property);

            if (imageControls.contains(propertyStr))
                pProcAmp->Set(property,
                              imageControls[propertyStr].toInt(),
                              VideoProcAmp_Flags_Manual);
        }

        pProcAmp->Release();
    }

    return true;
}

QVariantList CaptureMMFPrivate::cameraControls(IUnknown *device) const
{
    if (!device)
        return QVariantList();

    qint32 min;
    qint32 max;
    qint32 step;
    qint32 defaultValue;
    qint32 flags;
    qint32 value;

    QVariantList controls;
    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (auto &cameraControl: ccToStr->keys()) {
            if (SUCCEEDED(pCameraControl->GetRange(cameraControl,
                                                   reinterpret_cast<LONG *>(&min),
                                                   reinterpret_cast<LONG *>(&max),
                                                   reinterpret_cast<LONG *>(&step),
                                                   reinterpret_cast<LONG *>(&defaultValue),
                                                   reinterpret_cast<LONG *>(&flags))))
                if (SUCCEEDED(pCameraControl->Get(cameraControl,
                                                  reinterpret_cast<LONG *>(&value),
                                                  reinterpret_cast<LONG *>(&flags)))) {
                    QVariantList control;

                    control << ccToStr->value(cameraControl)
                            << QString("integer")
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
                }
        }

        pCameraControl->Release();
    }

    return controls;
}

bool CaptureMMFPrivate::setCameraControls(IUnknown *device,
                                          const QVariantMap &cameraControls) const
{
    if (!device)
        return false;

    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (auto &cameraControl: ccToStr->keys()) {
            QString cameraControlStr = ccToStr->value(cameraControl);

            if (cameraControls.contains(cameraControlStr))
                pCameraControl->Set(cameraControl,
                                    cameraControls[cameraControlStr].toInt(),
                                    CameraControl_Flags_Manual);
        }

        pCameraControl->Release();
    }

    return true;
}

QVariantMap CaptureMMFPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        QVariantList params = control.toList();
        QString controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureMMFPrivate::mapDiff(const QVariantMap &map1,
                                       const QVariantMap &map2) const
{
    QVariantMap map;

    for (auto &control: map2.keys())
        if (!map1.contains(control)
            || map1[control] != map2[control]) {
            map[control] = map2[control];
        }

    return map;
}

#include "moc_capturemmf.cpp"
