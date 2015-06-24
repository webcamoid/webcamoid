/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <sys/time.h>
#include <QApplication>
#include <QScreen>
#include <qbutils.h>

#include "desktopcaptureelement.h"

DesktopCaptureElement::DesktopCaptureElement():
    QbMultimediaSourceElement()
{
    this->m_curScreenNumber = -1;

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

    this->m_thread = ThreadPtr(new QThread, this->deleteThread);
    this->m_timer.moveToThread(this->m_thread.data());

    this->m_thread->start();

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readFrame()),
                     Qt::DirectConnection);
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

QbCaps DesktopCaptureElement::caps(int stream) const
{
    if (this->m_curScreenNumber < 0
        || stream != 0)
        return QbCaps();

    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];

    if (!screen)
        return QString();

    QbFrac fps(30000, 1001);

    QbCaps caps(QString("video/x-raw,"
                        "format=bgr0,"
                        "width=%1,"
                        "height=%2,"
                        "fps=%3/%4").arg(screen->size().width())
                                    .arg(screen->size().height())
                                    .arg(fps.num())
                                    .arg(fps.den()));

    return caps;
}

void DesktopCaptureElement::deleteThread(QThread *thread)
{
    thread->requestInterruption();
    thread->quit();
    thread->wait();
    delete thread;
}

void DesktopCaptureElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused) {
        this->m_id = Qb::id();
        QMetaObject::invokeMethod(&this->m_timer, "start");
    }
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull) {
        QMetaObject::invokeMethod(&this->m_timer, "stop");
    }
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

void DesktopCaptureElement::readFrame()
{
    QScreen *screen = QGuiApplication::screens()[this->m_curScreenNumber];
    QbFrac fps(30000, 1001);

    QbCaps caps(QString("video/x-raw,"
                        "format=bgr0,"
                        "width=%1,"
                        "height=%2,"
                        "fps=%3/%4").arg(screen->size().width())
                                    .arg(screen->size().height())
                                    .arg(fps.num())
                                    .arg(fps.den()));

    QPixmap frame = screen->grabWindow(QApplication::desktop()->winId());
    QbPacket packet = QbUtils::imageToPacket(frame.toImage(), caps);

    if (!packet)
        return;

    timeval timestamp;
    gettimeofday(&timestamp, NULL);

    qint64 pts = (timestamp.tv_sec
                  + 1e-6 * timestamp.tv_usec)
                  * fps.value();

    packet.setPts(pts);
    packet.setTimeBase(fps.invert());
    packet.setIndex(0);
    packet.setId(this->m_id);

    emit this->oStream(packet);
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
