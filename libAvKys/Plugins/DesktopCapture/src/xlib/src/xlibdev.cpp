/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
#include <QTime>
#include <QtConcurrent>
#include <QThreadPool>
#include <QFuture>
#include <QMutex>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

#ifdef HAVE_XEXT_SUPPORT
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#ifdef HAVE_XFIXES_SUPPORT
#include <X11/extensions/Xfixes.h>
#endif

#include "xlibdev.h"

#define DEFAULT_XIMAGE_FORMAT ZPixmap

class XlibDevPrivate
{
    public:
        XlibDev *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCaps> m_devicesCaps;
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        QTimer m_timer;
        QMutex m_mutex;
        Display *m_display {nullptr};
        Window m_rootWindow {0};
        XWindowAttributes m_windowAttributes;
#ifdef HAVE_XEXT_SUPPORT
        XShmSegmentInfo m_shmInfo;
#endif
        XImage *m_xImage {nullptr};
        bool m_haveShmExtension {false};
        bool m_showCursor {false};
        bool m_followCursor {false};

        explicit XlibDevPrivate(XlibDev *self);
        AkVideoCaps::PixelFormat pixelFormat(int depth, int bpp) const;
        void readFrame();
        void updateDevices();
};

XlibDev::XlibDev():
    ScreenDev()
{
    this->d = new XlibDevPrivate(this);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     [this] () {
                         this->d->readFrame();
                     });
    this->d->m_display = XOpenDisplay(nullptr);
    this->d->m_rootWindow = DefaultRootWindow(this->d->m_display);
    this->d->updateDevices();
}

XlibDev::~XlibDev()
{
    this->uninit();

    if (this->d->m_display)
        XCloseDisplay(this->d->m_display);

    delete this->d;
}

AkFrac XlibDev::fps() const
{
    return this->d->m_fps;
}

QStringList XlibDev::medias()
{
    return this->d->m_devices;
}

QString XlibDev::media() const
{
    return this->d->m_device;
}

QList<int> XlibDev::streams() const
{
    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (!caps)
        return {};

    return {0};
}

int XlibDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString XlibDev::description(const QString &media)
{
    return this->d->m_descriptions.value(media);
}

AkVideoCaps XlibDev::caps(int stream)
{
    Q_UNUSED(stream)

    return this->d->m_devicesCaps.value(this->d->m_device);
}

bool XlibDev::canCaptureCursor() const
{
#ifdef HAVE_XFIXES_SUPPORT
    bool canCaptureCursor = false;

    if (this->d->m_display) {
        int event = 0;
        int error = 0;
        canCaptureCursor = XFixesQueryExtension(this->d->m_display,
                                                &event,
                                                &error);
    }

    return canCaptureCursor;
#else
    return false;
#endif
}

bool XlibDev::canChangeCursorSize() const
{
    return false;
}

bool XlibDev::showCursor() const
{
    return this->d->m_showCursor;
}

int XlibDev::cursorSize() const
{
    return 0;
}

void XlibDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));}

void XlibDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void XlibDev::setMedia(const QString &media)
{
    if (this->d->m_device == media)
        return;

    this->d->m_device = media;
    emit this->mediaChanged(media);
}

void XlibDev::setShowCursor(bool showCursor)
{
    if (this->d->m_showCursor == showCursor)
        return;

    this->d->m_showCursor = showCursor;
    emit this->showCursorChanged(showCursor);

    if (this->d->m_timer.isActive()) {
        this->uninit();
        this->init();
    }
}

void XlibDev::setCursorSize(int cursorSize)
{
    Q_UNUSED(cursorSize)
}

void XlibDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
    this->setMedia(QString("screen://%1").arg(screen));
}

void XlibDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void XlibDev::resetStreams()
{

}

void XlibDev::resetShowCursor()
{
    this->setShowCursor(false);
}

void XlibDev::resetCursorSize()
{

}

bool XlibDev::init()
{
    int screen = 0;
    auto device = this->d->m_device;
    device.remove("screen://");
    auto displayScreen = device.split('.');

    if (displayScreen.size() > 1)
        screen = displayScreen[1].toInt();

    XGetWindowAttributes(this->d->m_display,
                         this->d->m_rootWindow,
                         &this->d->m_windowAttributes);
#ifdef HAVE_XEXT_SUPPORT
    this->d->m_haveShmExtension = XShmQueryExtension(this->d->m_display);

    if (this->d->m_haveShmExtension) {
        auto visual = DefaultVisual(this->d->m_display, screen);
        auto depth = DefaultDepth(this->d->m_display, screen);
        this->d->m_shmInfo.shmseg = 0;
        this->d->m_shmInfo.shmid = -1;
        this->d->m_shmInfo.shmaddr = (char *) -1;
        this->d->m_shmInfo.readOnly = false;
        this->d->m_xImage = XShmCreateImage(this->d->m_display,
                                            visual,
                                            depth,
                                            DEFAULT_XIMAGE_FORMAT,
                                            nullptr,
                                            &this->d->m_shmInfo,
                                            this->d->m_windowAttributes.width,
                                            this->d->m_windowAttributes.height);

        if (!this->d->m_xImage)
            return false;

        this->d->m_shmInfo.shmid = shmget(IPC_PRIVATE,
                                          this->d->m_xImage->bytes_per_line
                                          * this->d->m_xImage->height,
                                          IPC_CREAT | 0700);

        if (this->d->m_shmInfo.shmid == -1) {
            XDestroyImage(this->d->m_xImage);

            return false;
        }

        this->d->m_shmInfo.shmaddr =
            reinterpret_cast<char *>(shmat(this->d->m_shmInfo.shmid,
                                           nullptr,
                                           SHM_RND));

        if (this->d->m_shmInfo.shmaddr == reinterpret_cast<char *>(-1)) {
            shmctl(this->d->m_shmInfo.shmid, IPC_RMID, NULL);
            XDestroyImage(this->d->m_xImage);

            return false;
        }

        this->d->m_xImage->data = this->d->m_shmInfo.shmaddr;

        if (!XShmAttach(this->d->m_display, &this->d->m_shmInfo)) {
            shmdt(this->d->m_shmInfo.shmaddr);
            shmctl(this->d->m_shmInfo.shmid, IPC_RMID, NULL);
            XDestroyImage(this->d->m_xImage);

            return false;
        }
    }
#endif

#ifdef HAVE_XFIXES_SUPPORT
    this->d->m_followCursor = false;

    if (this->d->m_showCursor) {
        int event = 0;
        int error = 0;

        if (XFixesQueryExtension(this->d->m_display, &event, &error)) {
            XFixesSelectCursorInput(this->d->m_display,
                                    this->d->m_rootWindow,
                                    XFixesDisplayCursorNotifyMask);
            this->d->m_followCursor = true;
        }
    }
#endif

    this->d->m_id = Ak::id();
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    this->d->m_timer.start();

    return true;
}

bool XlibDev::uninit()
{
    this->d->m_timer.stop();

#ifdef HAVE_XEXT_SUPPORT
    if (this->d->m_haveShmExtension) {
        XShmDetach(this->d->m_display, &this->d->m_shmInfo);
        shmdt(this->d->m_shmInfo.shmaddr);
        shmctl(this->d->m_shmInfo.shmid, IPC_RMID, nullptr);

        if (this->d->m_xImage) {
            XDestroyImage(this->d->m_xImage);
            this->d->m_xImage = nullptr;
        }
    }
#endif

    return true;
}

XlibDevPrivate::XlibDevPrivate(XlibDev *self):
    self(self)
{
}

AkVideoCaps::PixelFormat XlibDevPrivate::pixelFormat(int depth, int bpp) const
{
    if (bpp == 16 && depth == 15)
        return AkVideoCaps::Format_rgb555;
    else if (bpp == 16 && depth == 16)
        return AkVideoCaps::Format_rgb565;
    else if (bpp == 32 && depth == 24)
        return AkVideoCaps::Format_xrgbpack;
    else if (bpp == 32 && depth == 32)
        return AkVideoCaps::Format_argbpack;

    return AkVideoCaps::Format_none;
}

void XlibDevPrivate::readFrame()
{
    XImage *image = nullptr;

#ifdef HAVE_XEXT_SUPPORT
    if (this->m_haveShmExtension) {
        XShmGetImage(this->m_display,
                     this->m_rootWindow,
                     this->m_xImage,
                     0,
                     0,
                     AllPlanes);
        image = this->m_xImage;
    } else {
#endif
        image = XGetImage(this->m_display,
                          this->m_rootWindow,
                          0,
                          0,
                          this->m_windowAttributes.width,
                          this->m_windowAttributes.height,
                          AllPlanes,
                          DEFAULT_XIMAGE_FORMAT);
#ifdef HAVE_XEXT_SUPPORT
    }
#endif

    if (!image)
        return;

    if (image->bitmap_pad != 32)
        return;

    bool drawCursor = false;
    int cursorX = 0;
    int cursorY = 0;

    if (this->m_followCursor) {
        Window rootWindow = 0;
        Window childWindow = 0;
        int windowX = 0;
        int windowY = 0;
        unsigned int mask = 0;
        drawCursor = XQueryPointer(this->m_display,
                                   this->m_rootWindow,
                                   &rootWindow,
                                   &childWindow,
                                   &cursorX,
                                   &cursorY,
                                   &windowX,
                                   &windowY,
                                   &mask);
    }

    auto &rMask = image->red_mask;
    auto &gMask = image->green_mask;
    auto &bMask = image->blue_mask;

#ifdef HAVE_XFIXES_SUPPORT
    if (drawCursor) {
        auto cursorImage = XFixesGetCursorImage(this->m_display);

        if (cursorImage) {
            int cursorWidth = qBound(0, cursorX + cursorImage->width, image->width) - cursorX;
            int cursorHeight = qBound(0, cursorY + cursorImage->height, image->height) - cursorY;

            for (int y = 0; y < cursorHeight; y++) {
                auto line = cursorImage->pixels
                            + y * cursorImage->width;

                for (int x = 0; x < cursorWidth; x++) {
                    auto &cursorPixel = line[x];
                    auto cursorA = quint8((cursorPixel >> 24) & 0xff);
                    auto cursorR = quint8((cursorPixel >> 16) & 0xff);
                    auto cursorG = quint8((cursorPixel >> 8) & 0xff);
                    auto cursorB = quint8(cursorPixel & 0xff);

                    auto imagePixel = XGetPixel(image, x + cursorX, y + cursorY);
                    auto imageR = quint8((imagePixel & rMask) >> 16);
                    auto imageG = quint8((imagePixel & gMask) >> 8) ;
                    auto imageB = quint8(imagePixel & bMask);

                    quint8 r = (cursorA * (cursorR - imageR) + 255 * imageR) / 255;
                    quint8 g = (cursorA * (cursorG - imageG) + 255 * imageG) / 255;
                    quint8 b = (cursorA * (cursorB - imageB) + 255 * imageB) / 255;

                    auto pixel = (quint32(r) << 16)
                                 | (quint32(g) << 8)
                                 | quint32(b);
                    XPutPixel(image, x + cursorX, y + cursorY, pixel);
                }
            }

            XFree(cursorImage);
        }
    }
#endif

    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

    AkVideoCaps videoCaps(AkVideoCaps::Format_rgb24,
                          image->width,
                          image->height,
                          this->m_fps);
    AkVideoPacket videoPacket(videoCaps);

    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);
    videoPacket.setPts(pts);
    videoPacket.setTimeBase(fps.invert());
    videoPacket.setIndex(0);
    videoPacket.setId(this->m_id);

    for (int y = 0; y < image->height; y++) {
        auto line_r = videoPacket.line(0, y);
        auto line_g = line_r + 1;
        auto line_b = line_r + 2;

        for (int x = 0; x < image->width; x++) {
            auto pixel = XGetPixel(image, x, y);
            line_r[3 * x] = quint8(((pixel & rMask) >> 16) & 0xff);
            line_g[3 * x] = quint8(((pixel & gMask) >> 8) & 0xff);
            line_b[3 * x] = quint8((pixel & bMask) & 0xff);
        }
    }

#ifdef HAVE_XEXT_SUPPORT
    if (!this->m_haveShmExtension)
        XDestroyImage(image);
#endif

    emit self->oStream(videoPacket);
}

void XlibDevPrivate::updateDevices()
{
    decltype(this->m_device) device;
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    if (this->m_display) {
        auto displayName = DisplayString(this->m_display);

        AkVideoCaps::PixelFormat pixelFormat = AkVideoCaps::Format_none;
        int depth = 0;
        int bpp = 0;
        int nFormats = 0;
        auto x11PixmapFormats = XListPixmapFormats(this->m_display, &nFormats);

        if (x11PixmapFormats) {
            for (int i = 0; i < nFormats; i++)
                if (x11PixmapFormats[i].depth >= depth
                    && x11PixmapFormats[i].bits_per_pixel >= bpp) {
                    pixelFormat =
                        this->pixelFormat(x11PixmapFormats[i].depth,
                                          x11PixmapFormats[i].bits_per_pixel);
                    depth = x11PixmapFormats[i].depth;
                    bpp = x11PixmapFormats[i].bits_per_pixel;
                }

            XFree((char *)x11PixmapFormats);
        }

        if (pixelFormat != AkVideoCaps::Format_none)
            for (int screen = 0; screen < ScreenCount(this->m_display); screen++) {
                auto deviceId = QString("%1.%2").arg(displayName).arg(screen);
                devices << deviceId;
                descriptions[deviceId] = QString("Screen %1").arg(deviceId);
                devicesCaps[deviceId] =
                    AkVideoCaps(pixelFormat,
                                XDisplayWidth(this->m_display, screen),
                                XDisplayHeight(this->m_display, screen),
                                this->m_fps);
            }

        auto defaultScreen = XDefaultScreenOfDisplay(this->m_display);

        if (defaultScreen) {
            int screenNumber = XScreenNumberOfScreen(defaultScreen);
            device = QString("%1.%2").arg(displayName).arg(screenNumber);
        }
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->mediasChanged(devices);
    }

    if (!this->m_devices.contains(this->m_device)) {
        this->m_device = device;
        emit self->mediaChanged(device);
    }
}
