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
        void readFrame();
        void sendPacket(const AkPacket &packet);
};

QtScreenDev::QtScreenDev():
    ScreenDev()
{
    this->d = new QtScreenDevPrivate(this);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    size_t i = 0;

    for (auto &screen: QGuiApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         this,
                         [=]() { this->srceenResized(int(i)); });
        i++;
    }

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &QtScreenDev::screenAdded);
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &QtScreenDev::screenRemoved);
    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     [this] () {
        this->d->readFrame();
    });
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
    QStringList screens;

    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        screens << QString("screen://%1").arg(i);

    return screens;
}

QString QtScreenDev::media() const
{
    if (!this->d->m_curScreen.isEmpty())
        return this->d->m_curScreen;

    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    return QString("screen://%1").arg(screen);
}

QList<int> QtScreenDev::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int QtScreenDev::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString QtScreenDev::description(const QString &media)
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        if (QString("screen://%1").arg(i) == media)
            return QString("Screen %1").arg(i);

    return QString();
}

AkVideoCaps QtScreenDev::caps(int stream)
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

    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       screen->size().width(),
                       screen->size().height(),
                       this->d->m_fps);
}

QtScreenDevPrivate::QtScreenDevPrivate(QtScreenDev *self):
    self(self)
{
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

void QtScreenDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    if (this->d->m_curScreenNumber == screen)
        return;

    this->d->m_curScreen = QString("screen://%1").arg(screen);
    this->d->m_curScreenNumber = screen;

    emit this->mediaChanged(this->d->m_curScreen);
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

void QtScreenDev::screenAdded(QScreen *screen)
{
    Q_UNUSED(screen)
    size_t i = 0;

    for (auto &screen_: QGuiApplication::screens()) {
        if (screen_ == screen)
            QObject::connect(screen_,
                             &QScreen::geometryChanged,
                             this,
                             [=]() { this->srceenResized(int(i)); });

        i++;
    }

    emit this->mediasChanged(this->medias());
}

void QtScreenDev::screenRemoved(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void QtScreenDev::srceenResized(int screen)
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

#include "moc_qtscreendev.cpp"
