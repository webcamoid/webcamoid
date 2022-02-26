/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
#include <QScreen>
#include <QThreadPool>
#include <QtConcurrent>
#include <QMutex>
#include <ak.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <CoreGraphics/CoreGraphics.h>

#include "avfoundationscreendev.h"
#import "framegrabber.h"

class AVFoundationScreenDevPrivate
{
    public:
        AVCaptureScreenInput *m_screenInput {nil};
        AVCaptureSession *m_captureSession {nil};
        AVCaptureVideoDataOutput *m_videoOutput {nil};
        id m_frameGrabber {nil};
        AkFrac m_fps {30000, 1001};
        QString m_curScreen;
        int m_curScreenNumber {-1};

        ~AVFoundationScreenDevPrivate();
};

AVFoundationScreenDev::AVFoundationScreenDev():
    ScreenDev()
{
    this->d = new AVFoundationScreenDevPrivate();
    size_t i = 0;

    for (auto screen: QGuiApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         this,
                         [=]() { this->srceenResized(int(i)); });
        i++;
    }

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &AVFoundationScreenDev::screenAdded);
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &AVFoundationScreenDev::screenRemoved);
}

AVFoundationScreenDev::~AVFoundationScreenDev()
{
    this->uninit();
    delete this->d;
}

AkFrac AVFoundationScreenDev::fps() const
{
    return this->d->m_fps;
}

QStringList AVFoundationScreenDev::medias()
{
    QStringList screens;

    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        screens << QString("screen://%1").arg(i);

    return screens;
}

QString AVFoundationScreenDev::media() const
{
    if (!this->d->m_curScreen.isEmpty())
        return this->d->m_curScreen;

    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    return QString("screen://%1").arg(screen);
}

QList<int> AVFoundationScreenDev::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int AVFoundationScreenDev::defaultStream(const QString &mimeType)
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString AVFoundationScreenDev::description(const QString &media)
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        if (QString("screen://%1").arg(i) == media)
            return QString("Screen %1").arg(i);

    return QString();
}

AkCaps AVFoundationScreenDev::caps(int stream)
{
    if (this->d->m_curScreenNumber < 0
        || stream != 0)
        return {};

    auto curScreen = this->d->m_curScreenNumber;
    auto screens = QGuiApplication::screens();

    if (curScreen < 0 || curScreen >= screens.size())
        return {};

    auto screen = screens[curScreen];

    if (!screen)
        return {};

    return AkVideoCaps(AkVideoCaps::Format_argb,
                       screen->size().width(),
                       screen->size().height(),
                       this->d->m_fps);
}

void AVFoundationScreenDev::frameReceived(CGDirectDisplayID screen,
                                          const QByteArray &buffer,
                                          qint64 pts,
                                          const AkFrac &fps,
                                          qint64 id)
{
    CGImageRef image = CGDisplayCreateImage(screen);

    AkVideoPacket videoPacket;
    videoPacket.caps() = {AkVideoCaps::Format_argb,
                          int(CGImageGetWidth(image)),
                          int(CGImageGetHeight(image)),
                          fps};
    videoPacket.buffer() = buffer;
    videoPacket.pts() = pts;
    videoPacket.timeBase() = fps.invert();
    videoPacket.index() = 0;
    videoPacket.id() = id;
    CGImageRelease(image);

    emit this->oStream(videoPacket);
}

void AVFoundationScreenDev::sendPacket(const AkPacket &packet)
{
    emit this->oStream(packet);
}

void AVFoundationScreenDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AVFoundationScreenDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void AVFoundationScreenDev::setMedia(const QString &media)
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++) {
        QString screen = QString("screen://%1").arg(i);

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

void AVFoundationScreenDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    if (this->d->m_curScreenNumber == screen)
        return;

    this->d->m_curScreen = QString("screen://%1").arg(screen);
    this->d->m_curScreenNumber = screen;

    emit this->mediaChanged(this->d->m_curScreen);
}

void AVFoundationScreenDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void AVFoundationScreenDev::resetStreams()
{

}

bool AVFoundationScreenDev::init()
{
    uint32_t nScreens = 0;
    CGGetActiveDisplayList(0, nullptr, &nScreens);
    QVector<CGDirectDisplayID> screens;
    screens.resize(int(nScreens));
    CGGetActiveDisplayList(nScreens, screens.data(), &nScreens);

    if (this->d->m_curScreenNumber >= screens.size())
        return false;

    CGDirectDisplayID screen = screens[this->d->m_curScreenNumber < 0?
                                       0: this->d->m_curScreenNumber];

    this->d->m_screenInput = [[AVCaptureScreenInput alloc]
                              initWithDisplayID: screen];

    if (!this->d->m_screenInput)
        return false;

    auto fps = this->d->m_fps;

    this->d->m_screenInput.minFrameDuration = CMTimeMake(int(fps.den()),
                                                         int(fps.num()));
    this->d->m_screenInput.capturesCursor = NO;
    this->d->m_screenInput.capturesMouseClicks = NO;

    this->d->m_captureSession = [[AVCaptureSession alloc] init];

    if ([this->d->m_captureSession canAddInput: this->d->m_screenInput]) {
        [this->d->m_captureSession addInput: this->d->m_screenInput];
    } else {
        [this->d->m_captureSession release];
        [this->d->m_screenInput release];

        return false;
    }

    this->d->m_videoOutput = [[AVCaptureVideoDataOutput alloc] init];

    auto videoOutputSettings =
            [NSDictionary
             dictionaryWithObject: [NSNumber numberWithUnsignedInt: kCVPixelFormatType_32BGRA]
             forKey: id(kCVPixelBufferPixelFormatTypeKey)];
    [this->d->m_videoOutput setVideoSettings: videoOutputSettings];
    [this->d->m_videoOutput setAlwaysDiscardsLateVideoFrames: YES];

    this->d->m_frameGrabber = [[FrameGrabber alloc]
                               initWithScreenDev: this
                               onScreen: screen
                               withFps: fps];
    auto queue = dispatch_queue_create("frame_queue", nullptr);
    [this->d->m_videoOutput setSampleBufferDelegate: this->d->m_frameGrabber queue: queue];
    dispatch_release(queue);

    if ([this->d->m_captureSession canAddOutput: this->d->m_videoOutput]) {
        [this->d->m_captureSession addOutput: this->d->m_videoOutput];
    } else {
        [this->d->m_frameGrabber release];
        [this->d->m_videoOutput release];
        [this->d->m_captureSession release];
        [this->d->m_screenInput release];

        return false;
    }

    [this->d->m_captureSession startRunning];

    return true;
}

bool AVFoundationScreenDev::uninit()
{
    if (this->d->m_captureSession)
        [this->d->m_captureSession stopRunning];

    [this->d->m_frameGrabber release];
    [this->d->m_videoOutput release];
    [this->d->m_captureSession release];
    [this->d->m_screenInput release];

    this->d->m_frameGrabber = nil;
    this->d->m_videoOutput = nil;
    this->d->m_captureSession = nil;
    this->d->m_screenInput = nil;

    return true;
}

void AVFoundationScreenDev::screenAdded(QScreen *screen)
{
    Q_UNUSED(screen)
    size_t i = 0;

    for (auto screen_: QGuiApplication::screens()) {
        if (screen_ == screen)
            QObject::connect(screen_,
                             &QScreen::geometryChanged,
                             this,
                             [=]() { this->srceenResized(int(i)); });

        i++;
    }

    emit this->mediasChanged(this->medias());
}

void AVFoundationScreenDev::screenRemoved(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void AVFoundationScreenDev::srceenResized(int screen)
{
    auto media = QString("screen://%1").arg(screen);
    auto screens = QGuiApplication::screens();

    if (screen < 0 || screen >= screens.size())
        return;

    auto widget = screens[screen];

    if (!widget)
        return;

    emit this->sizeChanged(media, widget->size());
}

AVFoundationScreenDevPrivate::~AVFoundationScreenDevPrivate()
{
    [this->m_captureSession stopRunning];
    [this->m_frameGrabber release];
    [this->m_videoOutput release];
    [this->m_captureSession release];
    [this->m_screenInput release];
}

#include "moc_avfoundationscreendev.cpp"
