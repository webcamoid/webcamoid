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
#include <QTime>
#include <QTimer>
#include <QtConcurrent>
#include <QThreadPool>
#include <QFuture>
#include <QMutex>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "qtscreendev.h"

using ImageToPixelFormatMap = QMap<QImage::Format, AkVideoCaps::PixelFormat>;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToAkFormat {
        {QImage::Format_RGB32     , AkVideoCaps::Format_0rgbpack},
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argbpack},
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565  },
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555  },
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444  },
        {QImage::Format_Grayscale8, AkVideoCaps::Format_gray8   }
    };

    return imageToAkFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, imageToAkFormat, (initImageToPixelFormatMap()))

class QtScreenDevPrivate
{
    public:
        QtScreenDev *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCaps> m_devicesCaps;
        AkFrac m_fps {30000, 1001};
        QString m_curScreen;
        qint64 m_id {-1};
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        AkPacket m_curPacket;
        int m_curScreenNumber {-1};
        bool m_threadedRead {true};

        explicit QtScreenDevPrivate(QtScreenDev *self);
        void setupGeometrySignals();
        void readFrame();
        void sendPacket(const AkPacket &packet);
        void updateDevices();
};

QtScreenDev::QtScreenDev():
    ScreenDev()
{
    this->d = new QtScreenDevPrivate(this);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
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
    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     [this] () {
                         this->d->readFrame();
                     });

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

void QtScreenDev::setFps(const AkFrac &fps)
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

bool QtScreenDev::init()
{
    auto device = this->d->m_device;
    this->d->m_curScreenNumber = device.remove("screen://").toInt();
    this->d->m_id = Ak::id();
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    this->d->m_timer.start();

    return true;
}

bool QtScreenDev::uninit()
{
    this->d->m_timer.stop();
    this->d->m_threadStatus.waitForFinished();

    return true;
}

QtScreenDevPrivate::QtScreenDevPrivate(QtScreenDev *self):
    self(self)
{
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

void QtScreenDevPrivate::readFrame()
{
    auto curScreen = this->m_curScreenNumber;
    auto screens = QGuiApplication::screens();

    if (curScreen < 0 || curScreen >= screens.size())
        return;

    auto screen = screens[curScreen];

    if (!screen)
        return;

    auto frame =
        screen->grabWindow(QApplication::desktop()->winId(),
                           screen->geometry().x(),
                           screen->geometry().y(),
                           screen->geometry().width(),
                           screen->geometry().height());
    auto oFrame = frame.toImage();

    if (!imageToAkFormat->contains(oFrame.format()))
        oFrame = oFrame.convertToFormat(QImage::Format_ARGB32);

    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

    AkVideoCaps caps(imageToAkFormat->value(oFrame.format()),
                     oFrame.width(),
                     oFrame.height(),
                     fps);
    AkVideoPacket packet(caps);
    auto lineSize = qMin<size_t>(oFrame.bytesPerLine(), packet.lineSize(0));

    for (int y = 0; y < oFrame.height(); ++y) {
        auto srcLine = oFrame.constScanLine(y);
        auto dstLine = packet.line(0, y);
        memcpy(dstLine, srcLine, lineSize);
    }

    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);

    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(this->m_id);

    if (!this->m_threadedRead) {
        emit self->oStream(packet);

        return;
    }

    if (!this->m_threadStatus.isRunning()) {
        this->m_curPacket = packet;

        this->m_threadStatus =
            QtConcurrent::run(&this->m_threadPool,
                              this,
                              &QtScreenDevPrivate::sendPacket,
                              this->m_curPacket);
    }
}

void QtScreenDevPrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
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
        descriptions[deviceId] = QString("Screen %1").arg(i);
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
