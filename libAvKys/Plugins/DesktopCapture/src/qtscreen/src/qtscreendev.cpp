/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <akutils.h>

#include "qtscreendev.h"

QtScreenDev::QtScreenDev():
    ScreenDev()
{
    this->m_fps = AkFrac(30000, 1001);
    this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
    this->m_curScreenNumber = -1;
    this->m_threadedRead = true;

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
    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &QtScreenDev::readFrame);
}

QtScreenDev::~QtScreenDev()
{
    this->uninit();
}

AkFrac QtScreenDev::fps() const
{
    return this->m_fps;
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
    if (!this->m_curScreen.isEmpty())
        return this->m_curScreen;

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
    if (this->m_curScreenNumber < 0
        || stream != 0)
        return AkCaps();

    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];

    if (!screen)
        return QString();

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_rgb24;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = this->m_fps;

    return caps.toCaps();
}

void QtScreenDev::sendPacket(const AkPacket &packet)
{
    emit this->oStream(packet);
}

void QtScreenDev::setFps(const AkFrac &fps)
{
    if (this->m_fps == fps)
        return;

    this->m_mutex.lock();
    this->m_fps = fps;
    this->m_mutex.unlock();
    emit this->fpsChanged(fps);
    this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
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
            if (this->m_curScreenNumber == i)
                break;

            this->m_curScreen = screen;
            this->m_curScreenNumber = i;

            emit this->mediaChanged(media);

            break;
        }
    }
}

void QtScreenDev::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    if (this->m_curScreenNumber == screen)
        return;

    this->m_curScreen = QString("screen://%1").arg(screen);
    this->m_curScreenNumber = screen;

    emit this->mediaChanged(this->m_curScreen);
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
    this->m_id = Ak::id();
    this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
    this->m_timer.start();

    return true;
}

bool QtScreenDev::uninit()
{
    this->m_timer.stop();
    this->m_threadStatus.waitForFinished();

    return true;
}

void QtScreenDev::readFrame()
{
    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];
    this->m_mutex.lock();
    auto fps = this->m_fps;
    this->m_mutex.unlock();

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
    QImage frameImg= frame.toImage().convertToFormat(QImage::Format_RGB888);
    AkPacket packet = AkUtils::imageToPacket(frameImg, caps.toCaps());

    if (!packet)
        return;

    qint64 pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                        * fps.value() / 1e3);

    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(this->m_id);

    if (!this->m_threadedRead) {
        emit this->oStream(packet);

        return;
    }

    if (!this->m_threadStatus.isRunning()) {
        this->m_curPacket = packet;

        this->m_threadStatus = QtConcurrent::run(&this->m_threadPool,
                                                 this,
                                                 &QtScreenDev::sendPacket,
                                                 this->m_curPacket);
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
