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
#include <QIcon>
#include <QMediaCaptureSession>
#include <QMutex>
#include <QPainter>
#include <QScreen>
#include <QScreenCapture>
#include <QSettings>
#include <QTime>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideopacket.h>

#include "qtscreendev.h"

using ScreenCapturePtr = QSharedPointer<QScreenCapture>;
using MediaCaptureSessionPtr = QSharedPointer<QMediaCaptureSession>;

class QtScreenDevPrivate
{
    public:
        QtScreenDev *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCaps> m_devicesCaps;
        AkFrac m_fps {30000, 1001};
        bool m_showCursor {false};
        int m_cursorSize {24};
        QScreen *m_curScreen {nullptr};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        ScreenCapturePtr m_screenCapture;
        MediaCaptureSessionPtr m_captureSession;
        QVideoSink m_videoSink;
        QVideoFrame m_curFrame;
        AkElementPtr m_rotateFilter {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        QList<QSize> m_availableSizes;
        QString m_iconsPath {":/Webcamoid/share/themes/WebcamoidTheme/icons"};
        QString m_themeName {"hicolor"};

        explicit QtScreenDevPrivate(QtScreenDev *self);
        QList<QSize> availableSizes(const QString &iconsPath,
                                    const QString &themeName) const;
        QSize nearestSize(const QSize &requestedSize) const;
        QSize nearestSize(const QList<QSize> &availableSizes,
                          const QSize &requestedSize) const;
        QImage cursorImage(const QSize &requestedSize) const;
        QImage cursorImage(QSize *size, const QSize &requestedSize) const;
        void setupGeometrySignals();
        qreal screenRotation() const;
        void frameReady(const QVideoFrame &frame);
        void sendFrame(const QVideoFrame &frame);
        void updateDevices();
};

QtScreenDev::QtScreenDev():
    ScreenDev()
{
    this->d = new QtScreenDevPrivate(this);
    this->d->m_availableSizes =
        this->d->availableSizes(this->d->m_iconsPath, this->d->m_themeName);
    this->d->setupGeometrySignals();
    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     [=]() {
                         this->d->setupGeometrySignals();
                         this->d->updateDevices();
                     });
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     [=]() {
                         this->d->setupGeometrySignals();
                         this->d->updateDevices();
                     });
    QObject::connect(&this->d->m_videoSink,
                     &QVideoSink::videoFrameChanged,
                     this,
                     [this] (const QVideoFrame &frame) {
                         this->d->frameReady(frame);
                     },
                     Qt::DirectConnection);

    this->d->updateDevices();
}

QtScreenDev::~QtScreenDev()
{
    this->uninit();
    delete this->d;
}

AkFrac QtScreenDev::fps() const
{
    return this->d->m_fps;
}

QStringList QtScreenDev::medias()
{
    return this->d->m_devices;
}

QString QtScreenDev::media() const
{
    return this->d->m_device;
}

QList<int> QtScreenDev::streams() const
{
    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (!caps)
        return {};

    return {0};
}

int QtScreenDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString QtScreenDev::description(const QString &media)
{
    return this->d->m_descriptions.value(media);
}

AkVideoCaps QtScreenDev::caps(int stream)
{
    Q_UNUSED(stream)

    return this->d->m_devicesCaps.value(this->d->m_device);
}

bool QtScreenDev::canCaptureCursor() const
{
#ifdef Q_OS_ANDROID
    return false;
#else
    return true;
#endif
}

bool QtScreenDev::canChangeCursorSize() const
{
    return true;
}

bool QtScreenDev::showCursor() const
{
#ifdef Q_OS_ANDROID
    return false;
#else
    return this->d->m_showCursor;
#endif
}

int QtScreenDev::cursorSize() const
{
    return this->d->m_cursorSize;
}

void QtScreenDev::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_mutex.lock();
    this->d->m_fps = fps;
    this->d->m_mutex.unlock();
    emit this->fpsChanged(fps);
}

void QtScreenDev::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

void QtScreenDev::setMedia(const QString &media)
{
    if (this->d->m_device == media)
        return;

    this->d->m_device = media;
    emit this->mediaChanged(media);
}

void QtScreenDev::setShowCursor(bool showCursor)
{
#ifdef Q_OS_ANDROID
    Q_UNUSED(showCursor)
#else
    if (this->d->m_showCursor == showCursor)
        return;

    this->d->m_showCursor = showCursor;
    emit this->showCursorChanged(showCursor);
#endif
}

void QtScreenDev::setCursorSize(int cursorSize)
{
    if (this->d->m_cursorSize == cursorSize)
        return;

    this->d->m_cursorSize = cursorSize;
    emit this->cursorSizeChanged(cursorSize);
}

void QtScreenDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
    auto defaultMedia = QString("screen://%1").arg(screen);
    this->setMedia(defaultMedia);
}

void QtScreenDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void QtScreenDev::resetStreams()
{

}

void QtScreenDev::resetShowCursor()
{
    this->setShowCursor(false);
}

void QtScreenDev::resetCursorSize()
{
    this->setCursorSize(24);
}

bool QtScreenDev::init()
{
    auto device = this->d->m_device;
    auto curScreen = device.remove("screen://").toInt();
    auto screens = QGuiApplication::screens();

    if (curScreen < 0 || curScreen >= screens.size())
        return false;

    auto screen = screens.value(curScreen);

    if (!screen)
        return false;

    this->d->m_id = Ak::id();
    this->d->m_curScreen = screen;
    this->d->m_screenCapture = ScreenCapturePtr::create(screen);
    this->d->m_captureSession = MediaCaptureSessionPtr(new QMediaCaptureSession);
    this->d->m_captureSession->setScreenCapture(this->d->m_screenCapture.data());
    this->d->m_captureSession->setVideoSink(&this->d->m_videoSink);
    this->d->m_screenCapture->start();

    QObject::connect(this->d->m_screenCapture.data(),
                     &QScreenCapture::errorOccurred,
                     [=] (QScreenCapture::Error error,
                          const QString &errorString) {
        Q_UNUSED(error)
        qDebug() << "Error starting screen capture:"
                 << errorString;
    });

    return true;
}

bool QtScreenDev::uninit()
{
    if (this->d->m_screenCapture) {
        this->d->m_screenCapture->stop();
        this->d->m_screenCapture = {};
    }

    this->d->m_captureSession = {};
    this->d->m_threadStatus.waitForFinished();

    return true;
}

QtScreenDevPrivate::QtScreenDevPrivate(QtScreenDev *self):
    self(self)
{
}

QList<QSize> QtScreenDevPrivate::availableSizes(const QString &iconsPath,
                                                const QString &themeName) const
{
    QList<QSize> availableSizes;
    QSettings theme(iconsPath + "/" + themeName + "/index.theme",
                    QSettings::IniFormat);
    theme.beginGroup("Icon Theme");

    for (auto &size: theme.value("Directories").toStringList()) {
        auto dims = size.split('x');

        if (dims.size() < 2)
            continue;

        auto width = dims.value(0).toInt();
        auto height = dims.value(1).toInt();

        if (width < 1 || height < 1)
            continue;

        availableSizes << QSize(width, height);
    }

    theme.endGroup();

    return availableSizes;
}

QSize QtScreenDevPrivate::nearestSize(const QSize &requestedSize) const
{
    return this->nearestSize(this->m_availableSizes, requestedSize);
}

QSize QtScreenDevPrivate::nearestSize(const QList<QSize> &availableSizes,
                                      const QSize &requestedSize) const
{
    QSize nearestSize;
    QSize nearestGreaterSize;
    int r = std::numeric_limits<int>::max();
    int s = std::numeric_limits<int>::max();
    int requestedArea = requestedSize.width() * requestedSize.height();

    for (auto &size: availableSizes) {
        int area = size.width() * size.height();
        int diffWidth = size.width() - requestedSize.width();
        int diffHeight = size.height() - requestedSize.height();
        int k = diffWidth * diffWidth + diffHeight * diffHeight;

        if (k < r) {
            nearestSize = size;
            r = k;
        }

        if (area >= requestedArea && k < s) {
            nearestGreaterSize = size;
            s = k;
        }
    }

    if (nearestGreaterSize.isEmpty())
        nearestGreaterSize = nearestSize;

    return nearestGreaterSize;
}

QImage QtScreenDevPrivate::cursorImage(const QSize &requestedSize) const
{
    return this->cursorImage(nullptr, requestedSize);
}

QImage QtScreenDevPrivate::cursorImage(QSize *size,
                                       const QSize &requestedSize) const
{
    auto iconSize = this->nearestSize(requestedSize);

    if (size)
        *size = iconSize;

    if (iconSize.isEmpty())
        return {};

    auto path = QString("%1/%2/%3x%4/%5.png")
                    .arg(this->m_iconsPath)
                    .arg(this->m_themeName)
                    .arg(iconSize.width())
                    .arg(iconSize.height())
                    .arg("cursor");
    QImage icon(path);

    return icon.convertToFormat(QImage::Format_ARGB32);
}

void QtScreenDevPrivate::setupGeometrySignals()
{
    size_t i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         [=]() { this->updateDevices(); });
        i++;
    }
}

qreal QtScreenDevPrivate::screenRotation() const
{
    return this->m_curScreen->angleBetween(this->m_curScreen->primaryOrientation(),
                                           this->m_curScreen->orientation());
}

void QtScreenDevPrivate::frameReady(const QVideoFrame &frame)
{
    if (!this->m_threadStatus.isRunning()) {
        this->m_curFrame = frame;

        this->m_threadStatus =
            QtConcurrent::run(&this->m_threadPool,
                              &QtScreenDevPrivate::sendFrame,
                              this,
                              this->m_curFrame);
    }
}

void QtScreenDevPrivate::sendFrame(const QVideoFrame &frame)
{
    auto frameImage = frame.toImage();
    frameImage = frameImage.convertToFormat(QImage::Format_ARGB32);

    if (this->m_showCursor) {
        auto cursorPos = QCursor::pos();
        auto cursorScreen = qApp->screenAt(cursorPos);

        if (cursorScreen == this->m_curScreen) {
            QSize cursorSize(this->m_cursorSize, this->m_cursorSize);
            auto cursor = this->cursorImage(cursorSize);
            auto cursorScaled =
                cursor.scaled(cursorSize,
                              Qt::IgnoreAspectRatio,
                              Qt::SmoothTransformation);

            QPainter painter;
            painter.begin(&frameImage);
            painter.drawImage(cursorPos, cursorScaled);
            painter.end();
        }
    }

    AkVideoCaps videoCaps(AkVideoCaps::Format_argbpack,
                          frameImage.width(),
                          frameImage.height(),
                          this->m_fps);
    AkVideoPacket videoPacket(videoCaps);
    videoPacket.setPts(frame.startTime());
    videoPacket.setTimeBase({1, 1000000});
    videoPacket.setIndex(0);
    videoPacket.setId(this->m_id);

    auto lineSize = qMin<size_t>(frameImage.bytesPerLine(), videoPacket.lineSize(0));

    for (int y = 0; y < frameImage.height(); ++y) {
        auto srcLine = frameImage.constScanLine(y);
        auto dstLine = videoPacket.line(0, y);
        memcpy(dstLine, srcLine, lineSize);
    }

    auto angle = -this->screenRotation();
    this->m_rotateFilter->setProperty("angle", angle);
    videoPacket = this->m_rotateFilter->iStream(videoPacket);

    emit self->oStream(videoPacket);
}

void QtScreenDevPrivate::updateDevices()
{
    decltype(this->m_device) device;
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    int i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        auto deviceId = QString("screen://%1").arg(i);
        devices << deviceId;
        descriptions[deviceId] = QString("Screen %1").arg(screen->name());
        devicesCaps[deviceId] = AkVideoCaps(AkVideoCaps::Format_rgb24,
                                            screen->size().width(),
                                            screen->size().height(),
                                            this->m_fps);

        if (screen == QGuiApplication::primaryScreen())
            device = deviceId;

        i++;
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

#include "moc_qtscreendev.cpp"
