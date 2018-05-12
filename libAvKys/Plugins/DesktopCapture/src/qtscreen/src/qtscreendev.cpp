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
#include <akutils.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akcaps.h>

#include "qtscreendev.h"

class QtScreenDevPrivate
{
    public:
        QtScreenDev *self;
        AkFrac m_fps;
        QString m_curScreen;
        int m_curScreenNumber;
        qint64 m_id;
        bool m_threadedRead;
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QMutex m_mutex;
        AkPacket m_curPacket;

        QtScreenDevPrivate(QtScreenDev *self):
            self(self),
            m_curScreenNumber(-1),
            m_id(-1),
            m_threadedRead(false)
        {
        }

        inline void sendPacket(const AkPacket &packet);
};

QtScreenDev::QtScreenDev():
    ScreenDev()
{
    this->d = new QtScreenDevPrivate(this);
    this->d->m_fps = AkFrac(30000, 1001);
    this->d->m_timer.setInterval(qRound(1.e3 *
                                        this->d->m_fps.invert().value()));
    this->d->m_curScreenNumber = -1;
    this->d->m_threadedRead = true;

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &QtScreenDev::screenCountChanged);
    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &QtScreenDev::screenCountChanged);
    QObject::connect(QApplication::desktop(),
                     &QDesktopWidget::resized,
                     this,
                     &QtScreenDev::srceenResized);
    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     &QtScreenDev::readFrame);
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

int QtScreenDev::defaultStream(const QString &mimeType)
{
    if (mimeType == "video/x-raw")
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

AkCaps QtScreenDev::caps(int stream)
{
    if (this->d->m_curScreenNumber < 0
        || stream != 0)
        return AkCaps();

    QScreen *screen = QGuiApplication::screens()[this->d->m_curScreenNumber];

    if (!screen)
        return QString();

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_rgb24;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = this->d->m_fps;

    return caps.toCaps();
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

void QtScreenDev::readFrame()
{
    auto screen = QGuiApplication::screens()[this->d->m_curScreenNumber];
    this->d->m_mutex.lock();
    auto fps = this->d->m_fps;
    this->d->m_mutex.unlock();

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_rgb24;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = fps;

    auto frame =
            screen->grabWindow(QApplication::desktop()->winId(),
                               screen->geometry().x(),
                               screen->geometry().y(),
                               screen->geometry().width(),
                               screen->geometry().height());
    QImage frameImg = frame.toImage().convertToFormat(QImage::Format_RGB888);
    AkPacket packet = AkUtils::imageToPacket(frameImg, caps.toCaps());

    if (!packet)
        return;

    qint64 pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);

    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(this->d->m_id);

    if (!this->d->m_threadedRead) {
        emit this->oStream(packet);

        return;
    }

    if (!this->d->m_threadStatus.isRunning()) {
        this->d->m_curPacket = packet;

        this->d->m_threadStatus =
                QtConcurrent::run(&this->d->m_threadPool,
                                  this->d,
                                  &QtScreenDevPrivate::sendPacket,
                                  this->d->m_curPacket);
    }
}

void QtScreenDev::screenCountChanged(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void QtScreenDev::srceenResized(int screen)
{
    QString media = QString("screen://%1").arg(screen);
    QWidget *widget = QApplication::desktop()->screen(screen);

    emit this->sizeChanged(media, widget->size());
}

#include "moc_qtscreendev.cpp"
