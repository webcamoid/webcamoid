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

#ifndef CAPTUREDSHOW_H
#define CAPTUREDSHOW_H

#include <sys/time.h>
#include <QSize>
#include <QMutex>
#include <QWaitCondition>

#include <ak.h>

#include "capture.h"
#include "framegrabber.h"

DEFINE_GUID(CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);
DEFINE_GUID(CLSID_NullRenderer, 0xc1f400a4, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);

Q_CORE_EXPORT HINSTANCE qWinAppInst();

typedef QSharedPointer<IGraphBuilder> GraphBuilderPtr;
typedef QSharedPointer<IBaseFilter> BaseFilterPtr;
typedef QSharedPointer<ISampleGrabber> SampleGrabberPtr;
typedef QSharedPointer<IAMStreamConfig> StreamConfigPtr;
typedef QSharedPointer<FrameGrabber> FrameGrabberPtr;
typedef QSharedPointer<IMoniker> MonikerPtr;
typedef QMap<QString, MonikerPtr> MonikersMap;
typedef QSharedPointer<AM_MEDIA_TYPE> MediaTypePtr;
typedef QList<MediaTypePtr> MediaTypesList;
typedef QSharedPointer<IPin> PinPtr;
typedef QList<PinPtr> PinList;

__inline bool operator <(REFGUID guid1, REFGUID guid2)
{
    return guid1.Data1 < guid2.Data1;
}

class CaptureDShow: public Capture
{
    Q_OBJECT

    public:
        enum IoMethod
        {
            IoMethodUnknown = -1,
            IoMethodDirectRead,
            IoMethodGrabSample,
            IoMethodGrabBuffer
        };

        explicit CaptureDShow(QObject *parent=NULL);
        ~CaptureDShow();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList caps(const QString &webcam) const;
        Q_INVOKABLE QString capsDescription(const AkCaps &caps) const;
        Q_INVOKABLE QVariantList imageControls() const;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls);
        Q_INVOKABLE bool resetImageControls();
        Q_INVOKABLE QVariantList cameraControls() const;
        Q_INVOKABLE bool setCameraControls(const QVariantMap &cameraControls);
        Q_INVOKABLE bool resetCameraControls();
        Q_INVOKABLE AkPacket readFrame();

    private:
        QStringList m_webcams;
        QString m_device;
        QList<int> m_streams;
        qint64 m_id;
        AkFrac m_timeBase;
        IoMethod m_ioMethod;
        QMap<QString, QSize> m_resolution;
        IGraphBuilder *m_graph;
        SampleGrabberPtr m_grabber;
        FrameGrabber m_frameGrabber;
        QByteArray m_curBuffer;
        QMutex m_mutex;
        QMutex m_controlsMutex;
        QWaitCondition m_waitCondition;

        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        AkCaps capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const;
        AkCaps capsFromMediaType(const MediaTypePtr &mediaType) const;
        HRESULT enumerateCameras(IEnumMoniker **ppEnum) const;
        MonikersMap listMonikers() const;
        MonikerPtr findMoniker(const QString &webcam) const;
        IBaseFilter *findFilterP(const QString &webcam) const;
        BaseFilterPtr findFilter(const QString &webcam) const;
        MediaTypesList listMediaTypes(const QString &webcam) const;
        MediaTypesList listMediaTypes(IBaseFilter *filter) const;
        bool isPinConnected(IPin *pPin, bool *ok=NULL) const;
        PinPtr findUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir) const;
        bool connectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest) const;
        PinList enumPins(IBaseFilter *filter, PIN_DIRECTION direction) const;
        bool createDeviceNotifier();
        static LRESULT CALLBACK deviceEvents(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static void deleteUnknown(IUnknown *unknown);
        static void deleteMediaType(AM_MEDIA_TYPE *mediaType);
        static void deletePin(IPin *pin);
        QVariantList imageControls(IBaseFilter *filter) const;
        bool setImageControls(IBaseFilter *filter, const QVariantMap &imageControls) const;
        QVariantList cameraControls(IBaseFilter *filter) const;
        bool setCameraControls(IBaseFilter *filter, const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1, const QVariantMap &map2) const;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setStreams(const QList<int> &streams);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetStreams();
        void resetIoMethod();
        void resetNBuffers();
        void reset();

    private slots:
        void frameReceived(qreal time, const QByteArray &buffer);
};

#endif // CAPTUREDSHOW_H
