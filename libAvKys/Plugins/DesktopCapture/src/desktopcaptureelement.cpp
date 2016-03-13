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

#include <QApplication>
#include <QScreen>
#include <QTime>
#include <akutils.h>

#include "desktopcaptureelement.h"

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement()
{
    this->m_curScreenNumber = -1;
    this->m_threadedRead = true;

    QObject::connect(qApp,
                     &QGuiApplication::screenAdded,
                     this,
                     &DesktopCaptureElement::screenCountChanged);

    QObject::connect(qApp,
                     &QGuiApplication::screenRemoved,
                     this,
                     &DesktopCaptureElement::screenCountChanged);

    QObject::connect(QApplication::desktop(),
                     &QDesktopWidget::resized,
                     this,
                     &DesktopCaptureElement::srceenResized);

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &DesktopCaptureElement::readFrame);
}

DesktopCaptureElement::~DesktopCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
}

QStringList DesktopCaptureElement::medias() const
{
    QStringList screens;

    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        screens << QString("screen://%1").arg(i);

    return screens;
}

QString DesktopCaptureElement::media() const
{
    if (!this->m_curScreen.isEmpty())
        return this->m_curScreen;

    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    return QString("screen://%1").arg(screen);
}

QList<int> DesktopCaptureElement::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int DesktopCaptureElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString DesktopCaptureElement::description(const QString &media) const
{
    for (int i = 0; i < QGuiApplication::screens().size(); i++)
        if (QString("screen://%1").arg(i) == media)
            return QString("Screen %1").arg(i);

    return QString();
}

AkCaps DesktopCaptureElement::caps(int stream) const
{
    if (this->m_curScreenNumber < 0
        || stream != 0)
        return AkCaps();

    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];

    if (!screen)
        return QString();

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_bgr0;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = AkFrac(30000, 1001);

    return caps.toCaps();
}

void DesktopCaptureElement::sendPacket(DesktopCaptureElement *element,
                                       const AkPacket &packet)
{
    emit element->oStream(packet);
}

void DesktopCaptureElement::setMedia(const QString &media)
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

void DesktopCaptureElement::resetMedia()
{
    int screen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());

    if (this->m_curScreenNumber == screen)
        return;

    this->m_curScreen = QString("screen://%1").arg(screen);
    this->m_curScreenNumber = screen;

    emit this->mediaChanged(this->m_curScreen);
}

bool DesktopCaptureElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->m_id = Ak::id();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->m_id = Ak::id();
            this->m_timer.start();

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->m_timer.start();

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
        case AkElement::ElementStatePaused:
            this->m_timer.stop();
            this->m_threadStatus.waitForFinished();

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    default:
        break;
    }

    return false;
}

void DesktopCaptureElement::readFrame()
{
    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];
    AkFrac fps(30000, 1001);

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_bgr0;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = screen->size().width();
    caps.height() = screen->size().height();
    caps.fps() = fps;

    QPixmap frame = screen->grabWindow(QApplication::desktop()->winId());
    AkPacket packet = AkUtils::imageToPacket(frame.toImage(), caps.toCaps());

    if (!packet)
        return;

    qint64 pts = QTime::currentTime().msecsSinceStartOfDay() * fps.value();

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
                                                 this->sendPacket,
                                                 this,
                                                 this->m_curPacket);
    }
}

void DesktopCaptureElement::screenCountChanged(QScreen *screen)
{
    Q_UNUSED(screen)

    emit this->mediasChanged(this->medias());
}

void DesktopCaptureElement::srceenResized(int screen)
{
    QString media = QString("screen://%1").arg(screen);
    QWidget *widget = QApplication::desktop()->screen(screen);

    emit this->sizeChanged(media, widget->size());
}
