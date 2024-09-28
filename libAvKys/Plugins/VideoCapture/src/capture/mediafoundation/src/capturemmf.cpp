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

#include <QCoreApplication>
#include <QDateTime>
#include <QMap>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QSize>
#include <QVariant>
#include <QWaitCondition>
#include <QtDebug>
#include <ak.h>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <strmif.h>
#include <dbt.h>
#include <uuids.h>
#include <wmcodecdsp.h>

#include "capturemmf.h"

#define TIME_BASE 1.0e7

#define AK_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

AK_DEFINE_GUID(AK_MEDIASUBTYPE_AVC1, 0x31435641, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_H264, 0x34363248, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_h264, 0x34363268, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_X264, 0x34363258, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_x264, 0x34363278, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

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

using RawFmtToAkFmtMap = QMap<GUID, AkVideoCaps::PixelFormat>;

inline RawFmtToAkFmtMap initRawFmtToAkFmt()
{
    RawFmtToAkFmtMap rawFmtToAkFmt {
        {MEDIASUBTYPE_ARGB1555, AkVideoCaps::Format_argb1555},
        {MEDIASUBTYPE_ARGB32  , AkVideoCaps::Format_argbpack},
        {MEDIASUBTYPE_ARGB4444, AkVideoCaps::Format_argb4444},
        {MEDIASUBTYPE_AYUV    , AkVideoCaps::Format_ayuvpack},
        {MEDIASUBTYPE_IF09    , AkVideoCaps::Format_yvu410p },
        {MEDIASUBTYPE_IYUV    , AkVideoCaps::Format_yuv420p },
        {MEDIASUBTYPE_NV12    , AkVideoCaps::Format_nv12    },
        {MEDIASUBTYPE_RGB24   , AkVideoCaps::Format_bgr24   },
        {MEDIASUBTYPE_RGB32   , AkVideoCaps::Format_xrgbpack},
        {MEDIASUBTYPE_RGB555  , AkVideoCaps::Format_rgb555  },
        {MEDIASUBTYPE_RGB565  , AkVideoCaps::Format_rgb565  },
        {MEDIASUBTYPE_UYVY    , AkVideoCaps::Format_uyvy422 },
        {MEDIASUBTYPE_Y211    , AkVideoCaps::Format_yuyv211 },
        {MEDIASUBTYPE_Y41P    , AkVideoCaps::Format_uyvy411 },
        {MEDIASUBTYPE_YUY2    , AkVideoCaps::Format_yuyv422 },
        {MEDIASUBTYPE_YUYV    , AkVideoCaps::Format_yuyv422 },
        {MEDIASUBTYPE_YV12    , AkVideoCaps::Format_yvu420p },
        {MEDIASUBTYPE_YVU9    , AkVideoCaps::Format_yvu410p },
        {MEDIASUBTYPE_YVYU    , AkVideoCaps::Format_yvyu422 },
    };

    return rawFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(RawFmtToAkFmtMap, rawFmtToAkFmt, (initRawFmtToAkFmt()))

using CompressedFormatToStrMap = QMap<GUID, QString>;

inline CompressedFormatToStrMap initCompressedFormatToStr()
{
    CompressedFormatToStrMap compressedFormatToStr {
        {MEDIASUBTYPE_CFCC   , "mjpg"  },
        {MEDIASUBTYPE_IJPG   , "jpeg"  },
        {MEDIASUBTYPE_MDVF   , "dv"    },
        {MEDIASUBTYPE_MJPG   , "mjpg"  },
        {MEDIASUBTYPE_Plum   , "mjpg"  },
        {MEDIASUBTYPE_QTJpeg , "jpeg"  },
        {MEDIASUBTYPE_QTRle  , "qtrle" },
        {MEDIASUBTYPE_QTRpza , "qtrpza"},
        {MEDIASUBTYPE_QTSmc  , "qtsmc" },
        {MEDIASUBTYPE_TVMJ   , "mjpg"  },
        {MEDIASUBTYPE_WAKE   , "mjpg"  },
        {MEDIASUBTYPE_dv25   , "dv25"  },
        {MEDIASUBTYPE_dv50   , "dv50"  },
        {MEDIASUBTYPE_dvh1   , "dvh1"  },
        {MEDIASUBTYPE_dvhd   , "dvhd"  },
        {MEDIASUBTYPE_dvsd   , "dvsd"  },
        {MEDIASUBTYPE_dvsl   , "dvsl"  },
        {AK_MEDIASUBTYPE_AVC1, "h264"  },
        {AK_MEDIASUBTYPE_H264, "h264"  },
        {AK_MEDIASUBTYPE_h264, "h264"  },
        {AK_MEDIASUBTYPE_X264, "h264"  },
        {AK_MEDIASUBTYPE_x264, "h264"  },
    };

    return compressedFormatToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CompressedFormatToStrMap,
                          compressedFormatToStr,
                          (initCompressedFormatToStr()))

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
        CaptureMMF *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, CaptureVideoCaps> m_devicesCaps;
        qint64 m_id {-1};
        bool m_hasMediaFoundation {false};
        DWORD m_streamIndex {DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM)};
        CaptureMMF::IoMethod m_ioMethod {CaptureMMF::IoMethodSync};
        MediaSourcePtr m_mediaSource;
        SourceReaderPtr m_sourceReader;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        explicit CaptureMMFPrivate(CaptureMMF *self);
        QVector<ActivatePtr> sources() const;
        ActivatePtr source(const QString &sourceId) const;
        MediaSourcePtr mediaSource(const QString &sourceId) const;
        MediaSourcePtr mediaSource(const ActivatePtr &activate) const;
        QVector<MediaTypeHandlerPtr> streams(const QString &webcam) const;
        QVector<MediaTypeHandlerPtr> streams(const ActivatePtr &activate) const;
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
        AkCaps capsFromMediaType(IMFMediaType *mediaType,
                                 bool *isRaw=nullptr,
                                 size_t *lineSize=nullptr,
                                 bool *mirror=nullptr) const;
        QVariantList imageControls(IUnknown *device) const;
        bool setImageControls(IUnknown *device,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(IUnknown *device) const;
        bool setCameraControls(IUnknown *device,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        void updateDevices();
};

CaptureMMF::CaptureMMF(QObject *parent):
    Capture(parent),
    QAbstractNativeEventFilter()
{
    this->d = new CaptureMMFPrivate(this);

    if (SUCCEEDED(MFStartup(MF_VERSION)))
        this->d->m_hasMediaFoundation = true;
    else
        qCritical() << "Failed initilizing MediaFoundation.";

    qApp->installNativeEventFilter(this);
    this->d->updateDevices();
}

CaptureMMF::~CaptureMMF()
{
    qApp->removeNativeEventFilter(this);

    if (this->d->m_hasMediaFoundation)
        MFShutdown();

    delete this->d;
}

QStringList CaptureMMF::webcams() const
{
    return this->d->m_devices;
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
        return {};

    return {0};
}

QList<int> CaptureMMF::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

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
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CaptureMMF::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QVariantList CaptureMMF::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureMMF::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lockForRead();
    auto globalImageControls = this->d->m_globalImageControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        auto control = globalImageControls[i].toList();
        auto controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

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
        auto params = control.toList();
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
    this->d->m_controlsMutex.lockForRead();
    auto globalCameraControls = this->d->m_globalCameraControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        auto control = globalCameraControls[i].toList();
        auto controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

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
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureMMF::readFrame()
{
    if (!this->d->m_sourceReader)
        return AkPacket();

    this->d->m_controlsMutex.lockForRead();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setImageControls(this->d->m_mediaSource.data(), controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lockForRead();
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
    bool isRaw = false;
    size_t srcLineSize = 0;
    bool mirror = false;
    auto caps = this->d->capsFromMediaType(mediaType,
                                           &isRaw,
                                           &srcLineSize,
                                           &mirror);
    mediaType->Release();

    // Read buffer.
    IMFMediaBuffer *buffer = nullptr;
    sample->GetBufferByIndex(0, &buffer);

    DWORD bufferLength = 0;
    buffer->GetMaxLength(&bufferLength);

    // Read pts.
    LONGLONG sampleTime = 0;
    sample->GetSampleTime(&sampleTime);
    AkFrac timeBase(1, TIME_BASE);

    if (sampleTime < 1) {
        AkVideoCaps videoCaps(caps);
        auto timestamp = QDateTime::currentMSecsSinceEpoch();
        sampleTime =
            LONGLONG(qreal(timestamp)
                     * videoCaps.fps().value()
                     / 1e3);
        timeBase = videoCaps.fps().invert();
    }

    if (isRaw) {
        IMF2DBuffer *d2Buffer = nullptr;

        if (SUCCEEDED(buffer->QueryInterface(IID_IMF2DBuffer,
                                             reinterpret_cast<void **>(&d2Buffer)))) {
            // Send packet.
            AkVideoPacket packet(caps);
            BYTE *data = nullptr;
            LONG stride = 0;

            d2Buffer->Lock2D(&data, &stride);
            auto iData = data;

            for (int plane = 0; plane < packet.planes(); ++plane) {
                auto iLineSize = stride
                               * packet.lineSize(plane)
                               / packet.lineSize(0);
                auto oLineSize = packet.lineSize(plane);
                auto lineSize = qMin<size_t>(iLineSize, oLineSize);
                auto heightDiv = packet.heightDiv(plane);

                if (mirror) {
                    for (int y = 0; y < packet.caps().height(); ++y) {
                        int ys = y >> heightDiv;
                        memcpy(packet.line(plane, packet.caps().height() - y - 1),
                               iData + ys * iLineSize,
                               lineSize);
                    }
                } else {
                    for (int y = 0; y < packet.caps().height(); ++y) {
                        int ys = y >> heightDiv;
                        memcpy(packet.line(plane, y),
                               iData + ys * iLineSize,
                               lineSize);
                    }
                }

                iData += (iLineSize * packet.caps().height()) >> heightDiv;
            }

            d2Buffer->Unlock2D();

            packet.setPts(sampleTime);
            packet.setTimeBase(timeBase);
            packet.setIndex(0);
            packet.setId(this->d->m_id);

            d2Buffer->Release();
            buffer->Release();
            sample->Release();

            return packet;
        }

        // Send packet.
        AkVideoPacket packet(caps);
        BYTE *data = nullptr;
        DWORD currentLength = 0;

        buffer->Lock(&data, &bufferLength, &currentLength);
        auto iData = data;

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto iLineSize = srcLineSize
                           * packet.lineSize(plane)
                           / packet.lineSize(0);
            auto oLineSize = packet.lineSize(plane);
            auto lineSize = qMin<size_t>(iLineSize, oLineSize);
            auto heightDiv = packet.heightDiv(plane);

            if (mirror) {
                for (int y = 0; y < packet.caps().height(); ++y) {
                    int ys = y >> heightDiv;
                    memcpy(packet.line(plane, packet.caps().height() - y - 1),
                           iData + ys * iLineSize,
                           lineSize);
                }
            } else {
                for (int y = 0; y < packet.caps().height(); ++y) {
                    int ys = y >> heightDiv;
                    memcpy(packet.line(plane, y),
                           iData + ys * iLineSize,
                           lineSize);
                }
            }

            iData += (iLineSize * packet.caps().height()) >> heightDiv;
        }

        buffer->Unlock();

        packet.setPts(sampleTime);
        packet.setTimeBase(timeBase);
        packet.setIndex(0);
        packet.setId(this->d->m_id);

        buffer->Release();
        sample->Release();

        return packet;
    }

    // Send compressed packet.
    AkCompressedVideoPacket packet(caps, bufferLength);
    BYTE *data = nullptr;
    DWORD currentLength = 0;

    buffer->Lock(&data, &bufferLength, &currentLength);
    memcpy(packet.data(),
           data,
           qMin<size_t>(bufferLength, currentLength));
    buffer->Unlock();

    packet.setPts(sampleTime);
    packet.setTimeBase(timeBase);
    packet.setIndex(0);
    packet.setId(this->d->m_id);

    buffer->Release();
    sample->Release();

    return packet;
}

bool CaptureMMF::nativeEventFilter(const QByteArray &eventType,
                                   void *message,
                                   qintptr *result)
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
            this->d->updateDevices();

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
    if (!this->d->m_hasMediaFoundation)
        return false;

    qDebug() << "Getting media source for" << this->d->m_device;
    auto mediaSource = this->d->mediaSource(this->d->m_device);

    if (!mediaSource) {
        qCritical() << "Error getting the media source for" << this->d->m_device;
        MFShutdown();

        return false;
    }

    qDebug() << "Creating media source attributes.";
    IMFAttributes *attributes = nullptr;

    if (FAILED(MFCreateAttributes(&attributes, 0))) {
        qCritical() << "Failed to create media source attributes.";
        MFShutdown();

        return false;
    }

    qDebug() << "Creating source reader.";
    attributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN,
                          TRUE);
    IMFSourceReader *sourceReader = nullptr;

    if (FAILED(MFCreateSourceReaderFromMediaSource(mediaSource.data(),
                                                   attributes,
                                                   &sourceReader))) {
        qCritical() << "Failed creating the source reader.";
        attributes->Release();
        MFShutdown();

        return false;
    }

    qDebug() << "Configuring the media types.";
    this->d->m_sourceReader =
            SourceReaderPtr(sourceReader,
                            CaptureMMFPrivate::deleteSourceReader);
    this->d->m_sourceReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,
                                                FALSE);

    auto streams = this->d->m_streams;

    if (this->d->m_streams.isEmpty())
        streams << 0;

    DWORD streamIndex = MF_SOURCE_READER_FIRST_VIDEO_STREAM;

    for (auto &stream: streams) {
        qDebug() << "Getting index from the selected stream.";
        DWORD mediaTypeIndex = 0;

        if (!this->d->indexFromTrack(mediaSource.data(),
                                     stream,
                                     &streamIndex,
                                     &mediaTypeIndex)) {
            qCritical() << "Error getting index from the selected stream.";
            this->d->m_sourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
            this->d->m_sourceReader.clear();
            attributes->Release();
            MFShutdown();

            return false;
        }

        qDebug() << "Setting the selected stream.";
        this->d->m_sourceReader->SetStreamSelection(streamIndex, TRUE);
        auto mediaStream = this->d->stream(mediaSource.data(), streamIndex);

        if (!mediaStream) {
            qCritical() << "Error setting the selected stream.";
            this->d->m_sourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
            this->d->m_sourceReader.clear();
            attributes->Release();
            MFShutdown();

            return false;
        }

        qDebug() << "Setting the media type.";
        auto mediaType = this->d->mediaType(mediaStream.data(), mediaTypeIndex);

        if (!mediaType) {
            qCritical() << "Error setting the selected stream.";
            this->d->m_sourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
            this->d->m_sourceReader.clear();
            attributes->Release();
            MFShutdown();

            return false;
        }

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
    attributes->Release();

    qDebug() << "Starting camera capture.";

    return true;
}

void CaptureMMF::uninit()
{
    this->d->m_sourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
    this->d->m_sourceReader.clear();
    this->d->m_mediaSource.clear();
}

void CaptureMMF::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();
        auto mediaSource = this->d->mediaSource(device);

        if (mediaSource) {
            this->d->m_globalImageControls =
                    this->d->imageControls(mediaSource.data());
            this->d->m_globalCameraControls =
                    this->d->cameraControls(mediaSource.data());
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForRead();
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
    auto supportedCaps = this->caps(this->d->m_device);
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

CaptureMMFPrivate::CaptureMMFPrivate(CaptureMMF *self):
    self(self)
{
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

    return this->mediaSource(source);
}

MediaSourcePtr CaptureMMFPrivate::mediaSource(const ActivatePtr &activate) const
{
    IMFMediaSource *mediaSource = nullptr;

    if (FAILED(activate->ActivateObject(IID_IMFMediaSource,
                                        reinterpret_cast<void **>(&mediaSource))))
        return MediaSourcePtr();

    return MediaSourcePtr(mediaSource, CaptureMMFPrivate::deleteMediaSource);
}

QVector<MediaTypeHandlerPtr> CaptureMMFPrivate::streams(const QString &webcam) const
{
    auto mediaSource = this->mediaSource(webcam);

    return this->streams(mediaSource.data());
}

QVector<MediaTypeHandlerPtr> CaptureMMFPrivate::streams(const ActivatePtr &activate) const
{
    auto mediaSource = this->mediaSource(activate);

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

AkCaps CaptureMMFPrivate::capsFromMediaType(IMFMediaType *mediaType,
                                            bool *isRaw,
                                            size_t *lineSize,
                                            bool *mirror) const
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

    UINT32 width = 0;
    UINT32 height = 0;
    MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);

    UINT32 fpsNum = 0;
    UINT32 fpsDen = 0;
    MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);
    AkFrac fps;

    if (fpsNum > 0 && fpsDen > 0)
        fps = {fpsNum, fpsDen};
    else
        fps = {30, 1};

    auto srcLineSize =
            INT32(MFGetAttributeUINT32(mediaType, MF_MT_DEFAULT_STRIDE, 0));

    if (srcLineSize == 0) {
        GUID subtype = GUID_NULL;
        LONG stride = 0;

        if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subtype)))
            MFGetStrideForBitmapInfoHeader(subtype.Data1,
                                           width,
                                           &stride);

        srcLineSize = INT32(stride);
    }

    if (lineSize)
        *lineSize = qAbs(srcLineSize);

    if (mirror)
        *mirror = srcLineSize < 0;

    WINBOOL isCompressed = FALSE;
    mediaType->IsCompressedFormat(&isCompressed);

    if (isRaw)
        *isRaw = !isCompressed;

    if (!isCompressed && rawFmtToAkFmt->contains(subtype)) {
        return AkVideoCaps(rawFmtToAkFmt->value(subtype),
                           width,
                           height,
                           fps);
    } else if (isCompressed && compressedFormatToStr->contains(subtype)) {
        return AkCompressedVideoCaps(compressedFormatToStr->value(subtype),
                                     width,
                                     height,
                                     fps);
    }

    return {};
}

QVariantList CaptureMMFPrivate::imageControls(IUnknown *device) const
{
    if (!device)
        return QVariantList();

    qint32 min = 0;
    qint32 max = 0;
    qint32 step = 0;
    qint32 defaultValue = 0;
    qint32 value = 0;
    qint32 flags = 0;

    QVariantList controls;
    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (auto it = vpapToStr->begin(); it != vpapToStr->end(); it++) {
            if (SUCCEEDED(pProcAmp->GetRange(it.key(),
                                             reinterpret_cast<LONG *>(&min),
                                             reinterpret_cast<LONG *>(&max),
                                             reinterpret_cast<LONG *>(&step),
                                             reinterpret_cast<LONG *>(&defaultValue),
                                             reinterpret_cast<LONG *>(&flags)))) {
                bool autoSupport = flags & VideoProcAmp_Flags_Auto;
                bool manualSupport = flags & VideoProcAmp_Flags_Manual;

                if (SUCCEEDED(pProcAmp->Get(it.key(),
                                            reinterpret_cast<LONG *>(&value),
                                            reinterpret_cast<LONG *>(&flags)))) {
                    if (autoSupport) {
                        QVariantList control {
                            it.value() + " (Auto)",
                            QString("boolean"),
                            0,
                            1,
                            1,
                            1,
                            flags & VideoProcAmp_Flags_Auto,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }

                    if (manualSupport) {
                        QString type;

                        if (min == 0 && max == 1)
                            type = "boolean";
                        else
                            type = "integer";

                        QVariantList control {
                            it.value(),
                            type,
                            min,
                            max,
                            step,
                            defaultValue,
                            value,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }
                }
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
        for (auto it = vpapToStr->begin(); it != vpapToStr->end(); it++) {
            auto key = it.value();

            if (imageControls.contains(key)) {
                LONG value = 0;
                LONG flags = 0;
                pProcAmp->Get(it.key(),
                              reinterpret_cast<LONG *>(&value),
                              reinterpret_cast<LONG *>(&flags));
                value = imageControls[key].toInt();
                pProcAmp->Set(it.key(), value, flags);
            }

            if (imageControls.contains(key + " (Auto)")) {
                LONG value = 0;
                LONG flags = 0;
                pProcAmp->Get(it.key(),
                              reinterpret_cast<LONG *>(&value),
                              reinterpret_cast<LONG *>(&flags));

                if (imageControls[key + " (Auto)"].toBool())
                    flags |= VideoProcAmp_Flags_Auto;
                else
                    flags &= ~VideoProcAmp_Flags_Auto;

                pProcAmp->Set(it.key(), value, flags);
            }
        }

        pProcAmp->Release();
    }

    return true;
}

QVariantList CaptureMMFPrivate::cameraControls(IUnknown *device) const
{
    if (!device)
        return QVariantList();

    qint32 min = 0;
    qint32 max = 0;
    qint32 step = 0;
    qint32 defaultValue = 0;
    qint32 value = 0;
    qint32 flags = 0;

    QVariantList controls;
    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (auto it = ccToStr->begin(); it != ccToStr->end(); it++) {
            if (SUCCEEDED(pCameraControl->GetRange(it.key(),
                                                   reinterpret_cast<LONG *>(&min),
                                                   reinterpret_cast<LONG *>(&max),
                                                   reinterpret_cast<LONG *>(&step),
                                                   reinterpret_cast<LONG *>(&defaultValue),
                                                   reinterpret_cast<LONG *>(&flags)))) {
                bool autoSupport = flags & CameraControl_Flags_Auto;
                bool manualSupport = flags & CameraControl_Flags_Manual;

                if (SUCCEEDED(pCameraControl->Get(it.key(),
                                                  reinterpret_cast<LONG *>(&value),
                                                  reinterpret_cast<LONG *>(&flags)))) {
                    if (autoSupport) {
                        QVariantList control {
                            it.value() + " (Auto)",
                            QString("boolean"),
                            0,
                            1,
                            1,
                            1,
                            flags & CameraControl_Flags_Auto,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }

                    if (manualSupport) {
                        QString type;

                        if (min == 0 && max == 1)
                            type = "boolean";
                        else
                            type = "integer";

                        QVariantList control {
                            it.value(),
                            type,
                            min,
                            max,
                            step,
                            defaultValue,
                            value,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }
                }
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
        for (auto it = ccToStr->begin(); it != ccToStr->end(); it++) {
            auto key = it.value();

            if (cameraControls.contains(key)) {
                LONG value = 0;
                LONG flags = 0;
                pCameraControl->Get(it.key(),
                                    reinterpret_cast<LONG *>(&value),
                                    reinterpret_cast<LONG *>(&flags));
                value = cameraControls[key].toInt();
                pCameraControl->Set(it.key(), value, flags);
            }

            if (cameraControls.contains(key + " (Auto)")) {
                LONG value = 0;
                LONG flags = 0;
                pCameraControl->Get(it.key(),
                                    reinterpret_cast<LONG *>(&value),
                                    reinterpret_cast<LONG *>(&flags));

                if (cameraControls[key + " (Auto)"].toBool())
                    flags |= CameraControl_Flags_Auto;
                else
                    flags &= ~CameraControl_Flags_Auto;

                pCameraControl->Set(it.key(), value, flags);
            }
        }

        pCameraControl->Release();
    }

    return true;
}

QVariantMap CaptureMMFPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
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

void CaptureMMFPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    if (this->d->m_hasMediaFoundation) {
        auto sources = this->sources();

        for (auto &source: sources) {
            WCHAR *sourceLink = nullptr;

            if (FAILED(source->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                                &sourceLink,
                                                nullptr))) {
                continue;
            }

            auto deviceId = QString::fromWCharArray(sourceLink);
            CoTaskMemFree(sourceLink);

            QString description;
            WCHAR *friendlyName = nullptr;

            if (SUCCEEDED(source->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                                    &friendlyName,
                                                    nullptr))) {
                description = QString::fromWCharArray(friendlyName);
                CoTaskMemFree(friendlyName);
            }

            CaptureVideoCaps caps;

            for (auto &stream: this->streams(source))
                for (auto &mediaType: this->mediaTypes(stream.data())) {
                    auto videoCaps = this->capsFromMediaType(mediaType.data());

                    if (videoCaps)
                        caps << videoCaps;
                }

            if (!caps.isEmpty()) {
                devices << deviceId;
                descriptions[deviceId] = description;
                devicesCaps[deviceId] = caps;
            }
        }

        if (devicesCaps.isEmpty()) {
            devices.clear();
            descriptions.clear();
        }
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

#include "moc_capturemmf.cpp"
