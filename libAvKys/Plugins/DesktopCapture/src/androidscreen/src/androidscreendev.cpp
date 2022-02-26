/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QApplication>
#include <QFuture>
#include <QMutex>
#include <QScreen>
#include <QThreadPool>
#include <QTime>
#include <QTimer>
#include <QtConcurrent>
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QAndroidActivityResultReceiver>
#include <ak.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akcaps.h>
#include <akvideopacket.h>

#include "androidscreendev.h"

#define JNAMESPACE "org/webcamoid/plugins/DesktopCapture/submodules/androidscreen"
#define JCLASS(jclass) JNAMESPACE "/" #jclass
#define JLCLASS(jclass) "L" JNAMESPACE "/" jclass ";"

#define MEDIA_PROJECTION_SERVICE "media_projection"
#define SCREEN_CAPTURE_REQUEST_CODE 0
#define RESULT_OK -1
#define BUFFER_SIZE 4

#define VIRTUAL_DISPLAY_FLAG_PUBLIC           (1 << 0)
#define VIRTUAL_DISPLAY_FLAG_PRESENTATION     (1 << 1)
#define VIRTUAL_DISPLAY_FLAG_SECURE           (1 << 2)
#define VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY (1 << 3)
#define VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR      (1 << 4)

enum ImageFormat
{
    TRANSLUCENT       = -3,
    TRANSPARENT       = -2,
    OPAQUE            = -1,
    UNKNOWN           = AK_FOURCC_NULL,
    RGBA_8888         = 1,
    RGBX_8888         = 2,
    RGB_888           = 3,
    RGB_565           = 4,
    RGBA_5551         = 6,
    RGBA_4444         = 7,
    A_8               = 8,
    L_8               = 9,
    LA_88             = 10,
    RGB_332           = 11,
    NV16              = 16,
    NV21              = 17,
    YUY2              = 20,
    RGBA_F16          = 22,
    RAW_SENSOR        = 32,
    YUV_420_888       = 35,
    PRIVATE           = 34,
    RAW_PRIVATE       = 36,
    RAW10             = 37,
    RAW12             = 38,
    YUV_422_888       = 39,
    FLEX_RGB_888      = 41,
    FLEX_RGBA_8888    = 42,
    RGBA_1010102      = 43,
    JPEG              = 256,
    DEPTH_POINT_CLOUD = 257,
    Y8                = AkFourCCR('Y', '8', ' ', ' '),
    YV12              = AkFourCCR('Y', 'V', '1', '2'),
    DEPTH16           = AkFourCCR('Y', '1', '6', 'D'),
    DEPTH_JPEG        = AkFourCCR('c', 'i', 'e', 'i'),
};

class AndroidScreenDevPrivate: public QAndroidActivityResultReceiver
{
    public:
        AndroidScreenDev *self;
        AkFrac m_fps {30000, 1001};
        QString m_curScreen;
        qint64 m_id {-1};
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QWaitCondition m_captureSetupReady;
        QWaitCondition m_packetReady;
        QMutex m_mutex;
        AkPacket m_curPacket;
        QAndroidJniEnvironment m_jenv;
        QAndroidJniObject m_activity;
        QAndroidJniObject m_service;
        QAndroidJniObject m_mediaProjection;
        QAndroidJniObject m_virtualDisplay;
        QAndroidJniObject m_imageReader;
        QAndroidJniObject m_callbacks;
        int m_curScreenNumber {-1};
        bool m_threadedRead {true};
        bool m_canCapture {false};

        explicit AndroidScreenDevPrivate(AndroidScreenDev *self);
        void registerNatives();
        void sendPacket(const AkPacket &packet);
        void handleActivityResult(int requestCode,
                                  int resultCode,
                                  const QAndroidJniObject &intent);
        static void imageAvailable(JNIEnv *env,
                                   jobject obj,
                                   jlong userPtr,
                                   jint format,
                                   jint width,
                                   jint height,
                                   jlong timestampNs,
                                   jbyteArray data);
        static void captureStopped(JNIEnv *env, jobject obj, jlong userPtr);
};

AndroidScreenDev::AndroidScreenDev():
    ScreenDev()
{
    this->d = new AndroidScreenDevPrivate(this);
    this->d->m_activity = QtAndroid::androidActivity();
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));

    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     &AndroidScreenDev::readFrame);
}

AndroidScreenDev::~AndroidScreenDev()
{
    this->uninit();
    delete this->d;
}

AkFrac AndroidScreenDev::fps() const
{
    return this->d->m_fps;
}

QStringList AndroidScreenDev::medias()
{
    QStringList screens;

    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        screens << QString("screen://%1").arg(i);

    return screens;
}

QString AndroidScreenDev::media() const
{
    if (!this->d->m_curScreen.isEmpty())
        return this->d->m_curScreen;

    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    return QString("screen://%1").arg(screen);
}

QList<int> AndroidScreenDev::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int AndroidScreenDev::defaultStream(const QString &mimeType)
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString AndroidScreenDev::description(const QString &media)
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        if (QString("screen://%1").arg(i) == media)
            return QString("Screen %1").arg(i);

    return QString();
}

AkCaps AndroidScreenDev::caps(int stream)
{
    if (this->d->m_curScreenNumber < 0
        || stream != 0)
        return AkCaps();

    auto curScreen = this->d->m_curScreenNumber;
    auto screens = QGuiApplication::screens();

    if (curScreen < 0 || curScreen >= screens.size())
        return {};

    auto screen = screens[curScreen];

    if (!screen)
        return {};

    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       screen->size().width(),
                       screen->size().height(),
                       this->d->m_fps);
}

AndroidScreenDevPrivate::AndroidScreenDevPrivate(AndroidScreenDev *self):
    self(self)
{
    this->registerNatives();
    this->m_callbacks =
            QAndroidJniObject(JCLASS(AkAndroidScreenCallbacks),
                              "(J)V",
                              this);
}

void AndroidScreenDevPrivate::registerNatives()
{
    static bool ready = false;

    if (ready)
        return;

    QAndroidJniEnvironment jenv;

    if (auto jclass = jenv.findClass(JCLASS(AkAndroidScreenCallbacks))) {
        QVector<JNINativeMethod> methods {
            {"imageAvailable", "(JIIIJ[B)V", reinterpret_cast<void *>(AndroidScreenDevPrivate::imageAvailable)},
            {"captureStopped", "(J)V"      , reinterpret_cast<void *>(AndroidScreenDevPrivate::captureStopped)},
        };

        jenv->RegisterNatives(jclass, methods.data(), methods.size());
    }

    ready = true;
}

void AndroidScreenDevPrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
}

void AndroidScreenDevPrivate::handleActivityResult(int requestCode,
                                                   int resultCode,
                                                   const QAndroidJniObject &intent)
{
    if (requestCode != SCREEN_CAPTURE_REQUEST_CODE)
        return;

    QAndroidJniObject mediaProjectionCallback;
    QAndroidJniObject resources;
    QAndroidJniObject metrics;
    QAndroidJniObject surface;
    auto displayName = QAndroidJniObject::fromString("VirtualDisplay");
    jint width;
    jint height;
    jint density;

    if (resultCode != RESULT_OK)
        goto handleActivityResult_fail;

    this->m_mediaProjection =
            this->m_service.callObjectMethod("getMediaProjection",
                                             "(ILandroid/content/Intent;)Landroid/media/projection/MediaProjection;",
                                             resultCode,
                                             intent.object());

    if (!this->m_mediaProjection.isValid())
        goto handleActivityResult_fail;

    mediaProjectionCallback = this->m_callbacks.callObjectMethod("mediaProjectionCallback",
                                                                 "()"
                                                                 JLCLASS("AkAndroidScreenCallbacks$MediaProjectionCallback"));
    this->m_mediaProjection.callMethod<void>("registerCallback",
                                             "(Landroid/media/projection/MediaProjection$Callback;"
                                             "Landroid/os/Handler;)V",
                                             mediaProjectionCallback.object(),
                                             nullptr);
    resources =
            this->m_activity.callObjectMethod("getResources",
                                              "()Landroid/content/res/Resources;");
    metrics = resources.callObjectMethod("getDisplayMetrics",
                                         "()Landroid/util/DisplayMetrics;");
    width = metrics.getField<jint>("widthPixels");
    height = metrics.getField<jint>("heightPixels");
    density = metrics.getField<jint>("densityDpi");

    this->m_imageReader =
            QAndroidJniObject::callStaticObjectMethod("android/media/ImageReader",
                                                      "newInstance",
                                                      "(IIII)Landroid/media/ImageReader;",
                                                      width,
                                                      height,
                                                      ImageFormat::RGBA_8888,
                                                      BUFFER_SIZE);

    if (!this->m_imageReader.isValid())
        goto handleActivityResult_fail;

    surface =
            this->m_imageReader.callObjectMethod("getSurface",
                                                 "()Landroid/view/Surface;");

    if (!surface.isValid())
        goto handleActivityResult_fail;

    this->m_imageReader.callMethod<void>("setOnImageAvailableListener",
                                         "(Landroid/media/ImageReader$OnImageAvailableListener;"
                                         "Landroid/os/Handler;)V",
                                         this->m_callbacks.object(),
                                         nullptr);

    this->m_virtualDisplay =
            this->m_mediaProjection.callObjectMethod("createVirtualDisplay",
                                                     "(Ljava/lang/String;"
                                                     "IIII"
                                                     "Landroid/view/Surface;"
                                                     "Landroid/hardware/display/VirtualDisplay$Callback;"
                                                     "Landroid/os/Handler;)"
                                                     "Landroid/hardware/display/VirtualDisplay;",
                                                     displayName.object(),
                                                     width,
                                                     height,
                                                     density,
                                                     VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                                                     surface.object(),
                                                     nullptr,
                                                     nullptr);

    if (!this->m_virtualDisplay.isValid())
        goto handleActivityResult_fail;

    this->m_canCapture = true;

handleActivityResult_fail:
    this->m_mutex.lock();
    this->m_captureSetupReady.wakeAll();
    this->m_mutex.unlock();
}

void AndroidScreenDevPrivate::imageAvailable(JNIEnv *env,
                                             jobject obj,
                                             jlong userPtr,
                                             jint format,
                                             jint width,
                                             jint height,
                                             jlong timestampNs,
                                             jbyteArray data)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(format)

    auto dataSize = env->GetArrayLength(data);

    if (dataSize < 1)
        return;

    QByteArray oBuffer(dataSize, Qt::Uninitialized);
    env->GetByteArrayRegion(data,
                            0,
                            dataSize,
                            reinterpret_cast<jbyte *>(oBuffer.data()));

    auto self = reinterpret_cast<AndroidScreenDevPrivate *>(userPtr);
    AkVideoCaps caps(AkVideoCaps::Format_0bgr,
                     width,
                     height,
                     self->m_fps);
    auto pts = qint64(timestampNs * self->m_fps.value() / 1e9);

    AkVideoPacket packet;
    packet.caps() = caps;
    packet.buffer() = oBuffer;
    packet.setPts(pts);
    packet.setTimeBase(self->m_fps.invert());
    packet.setIndex(0);
    packet.setId(self->m_id);
    packet = packet.convert(AkVideoCaps::Format_rgb24);

    self->m_mutex.lock();
    self->m_curPacket = packet;
    self->m_packetReady.wakeAll();
    self->m_mutex.unlock();
}

void AndroidScreenDevPrivate::captureStopped(JNIEnv *env,
                                             jobject obj,
                                             jlong userPtr)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(userPtr)
}

void AndroidScreenDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
}

void AndroidScreenDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void AndroidScreenDev::setMedia(const QString &media)
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++) {
        auto screen = QString("screen://%1").arg(i);

        if (screen == media) {
            if (this->d->m_curScreenNumber == i)
                break;

            this->d->m_curScreen = screen;
            this->d->m_curScreenNumber = i;
            emit this->mediaChanged(media);

            break;
        }
    }
}

void AndroidScreenDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    if (this->d->m_curScreenNumber == screen)
        return;

    this->d->m_curScreen = QString("screen://%1").arg(screen);
    this->d->m_curScreenNumber = screen;

    emit this->mediaChanged(this->d->m_curScreen);
}

void AndroidScreenDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void AndroidScreenDev::resetStreams()
{

}

bool AndroidScreenDev::init()
{
    this->uninit();

    this->d->m_canCapture = false;
    auto serviceName = QAndroidJniObject::fromString(MEDIA_PROJECTION_SERVICE);
    this->d->m_service =
            this->d->m_activity.callObjectMethod("getSystemService",
                                                 "(Ljava/lang/String;)Ljava/lang/Object;",
                                                 serviceName.object());

    if (!this->d->m_service.isValid())
        return false;

    auto intent =
            this->d->m_service.callObjectMethod("createScreenCaptureIntent",
                                                "()Landroid/content/Intent;");

    if (!intent.isValid())
        return false;

    QtAndroid::startActivity(intent,
                             SCREEN_CAPTURE_REQUEST_CODE,
                             this->d);

    this->d->m_mutex.lock();
    this->d->m_captureSetupReady.wait(&this->d->m_mutex);
    this->d->m_mutex.unlock();

    if (!this->d->m_canCapture)
        return false;

    this->d->m_id = Ak::id();
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    this->d->m_timer.start();

    return true;
}

bool AndroidScreenDev::uninit()
{
    this->d->m_timer.stop();
    this->d->m_threadStatus.waitForFinished();

    if (this->d->m_mediaProjection.isValid()) {
        this->d->m_mediaProjection.callMethod<void>("stop");
        this->d->m_mediaProjection = {};
    }

    if (this->d->m_virtualDisplay.isValid()) {
        this->d->m_virtualDisplay.callMethod<void>("release");
        this->d->m_virtualDisplay = {};
    }

    if (this->d->m_imageReader.isValid()) {
        this->d->m_imageReader.callMethod<void>("close");
        this->d->m_imageReader = {};
    }

    this->d->m_service = {};

    return true;
}

void AndroidScreenDev::readFrame()
{
    this->d->m_mutex.lock();

    if (!this->d->m_curPacket)
        if (!this->d->m_packetReady.wait(&this->d->m_mutex, 1000)) {
            this->d->m_mutex.unlock();

            return;
        }

    auto packet = this->d->m_curPacket;
    this->d->m_curPacket = {};
    this->d->m_mutex.unlock();

    if (!this->d->m_threadedRead) {
        emit this->oStream(packet);

        return;
    }

    if (!this->d->m_threadStatus.isRunning()) {
        this->d->m_threadStatus =
                QtConcurrent::run(&this->d->m_threadPool,
                                  this->d,
                                  &AndroidScreenDevPrivate::sendPacket,
                                  packet);
    }
}

#include "moc_androidscreendev.cpp"
