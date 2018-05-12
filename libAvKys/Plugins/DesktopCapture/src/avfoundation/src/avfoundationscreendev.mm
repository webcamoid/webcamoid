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
#include <QDesktopWidget>
#include <QScreen>
#include <QThreadPool>
#include <QtConcurrent>
#include <QMutex>
#include <ak.h>
#include <akutils.h>
#include <akcaps.h>
#include <akvideopacket.h>
#include <CoreGraphics/CoreGraphics.h>

#include "avfoundationscreendev.h"
#import "framegrabber.h"

class AVFoundationScreenDevPrivate
{
    public:
        AVCaptureScreenInput *m_screenInput;
        AVCaptureSession *m_captureSession;
        AVCaptureVideoDataOutput *m_videoOutput;
        id m_frameGrabber;
        AkFrac m_fps;
        QString m_curScreen;
        int m_curScreenNumber;

        explicit AVFoundationScreenDevPrivate():
            m_screenInput(nil),
            m_captureSession(nil),
            m_videoOutput(nil),
            m_frameGrabber(nil),
            m_fps(AkFrac(30000, 1001)),
            m_curScreenNumber(-1)
        {
        }

        ~AVFoundationScreenDevPrivate()
        {
            [this->m_captureSession stopRunning];
            [this->m_frameGrabber release];
            [this->m_videoOutput release];
            [this->m_captureSession release];
            [this->m_screenInput release];
        }
};

AVFoundationScreenDev::AVFoundationScreenDev():
    ScreenDev()
{
    this->d = new AVFoundationScreenDevPrivate();

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &AVFoundationScreenDev::screenCountChanged);
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &AVFoundationScreenDev::screenCountChanged);
    QObject::connect(QApplication::desktop(),
                     &QDesktopWidget::resized,
                     this,
                     &AVFoundationScreenDev::srceenResized);
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
        return AkCaps();

    QScreen *screen = QGuiApplication::screens()[this->d->m_curScreenNumber];

    if (!screen)
        return QString();

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_argb;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = this->d->m_fps;

    return caps.toCaps();
}

void AVFoundationScreenDev::frameReceived(CGDirectDisplayID screen,
                                          const QByteArray &buffer,
                                          qint64 pts,
                                          const AkFrac &fps,
                                          qint64 id)
{
    CGImageRef image = CGDisplayCreateImage(screen);
    QImage frameImg(int(CGImageGetWidth(image)),
                    int(CGImageGetHeight(image)),
                    QImage::Format_RGB32);
    auto bufferSize = size_t(qMin(buffer.size(),
                                  frameImg.bytesPerLine()
                                  * frameImg.height()));
    memcpy(frameImg.bits(), buffer.constData(), bufferSize);

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_argb;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = frameImg.width();
    caps.height() = frameImg.height();
    caps.fps() = fps;

    AkPacket packet = AkUtils::imageToPacket(frameImg, caps.toCaps());

    if (!packet)
        return;

    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(id);

    emit this->oStream(packet);
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
    CGGetActiveDisplayList(0, NULL, &nScreens);
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
    auto queue = dispatch_queue_create("frame_queue", NULL);
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

void AVFoundationScreenDev::screenCountChanged(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void AVFoundationScreenDev::srceenResized(int screen)
{
    QString media = QString("screen://%1").arg(screen);
    QWidget *widget = QApplication::desktop()->screen(screen);

    emit this->sizeChanged(media, widget->size());
}

#include "moc_avfoundationscreendev.cpp"
