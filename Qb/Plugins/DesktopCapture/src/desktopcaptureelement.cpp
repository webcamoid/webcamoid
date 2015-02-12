/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QApplication>
#include <qbutils.h>

#include "desktopcaptureelement.h"

DesktopCaptureElement::DesktopCaptureElement():
    QbMultimediaSourceElement()
{
    this->m_desktopWidget = QApplication::desktop();
    this->m_curScreenNumber = -1;

    QObject::connect(this->m_desktopWidget,
                     SIGNAL(screenCountChanged(int)),
                     this,
                     SLOT(screenCountChanged(int)));

    QObject::connect(this->m_desktopWidget,
                     SIGNAL(sizeChanged(const QString &, const QSize &)),
                     this,
                     SIGNAL(sizeChanged(const QString &, const QSize &)));

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

    for (int i = 0; i < this->m_desktopWidget->screenCount(); i++)
        screens << QString("screen://%1").arg(i);

    return screens;
}

QString DesktopCaptureElement::media() const
{
    if (!this->m_curScreen.isEmpty())
        return this->m_curScreen;

    int screen = this->m_desktopWidget->screenNumber(this->m_desktopWidget->screen());

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
    for (int i = 0; i < this->m_desktopWidget->screenCount(); i++)
        if (QString("screen://%1").arg(i) == media)
            return QString("Screen %1").arg(i);

    return QString();
}

QbCaps DesktopCaptureElement::caps(int stream) const
{
    if (this->m_curScreenNumber < 0
        || stream != 0)
        return QbCaps();

    QWidget *screen = this->m_desktopWidget->screen(this->m_curScreenNumber);

    if (!screen)
        return QString();

    QbCaps caps(QString("video/x-raw,"
                        "format=bgr0,"
                        "width=%1,"
                        "height=%2,"
                        "fps=30000/1001").arg(screen->width())
                                         .arg(screen->height()));

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
        QMetaObject::invokeMethod(&this->m_timer, "start");
    }
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull) {
        QMetaObject::invokeMethod(&this->m_timer, "stop");
    }
}

void DesktopCaptureElement::setMedia(const QString &media)
{
    for (int i = 0; i < this->m_desktopWidget->screenCount(); i++) {
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
    int screen = this->m_desktopWidget->screenNumber(this->m_desktopWidget->screen());

    if (this->m_curScreenNumber == screen)
        return;

    this->m_curScreen = QString("screen://%1").arg(screen);
    this->m_curScreenNumber = screen;

    emit this->mediaChanged(this->m_curScreen);
}

void DesktopCaptureElement::readFrame()
{
    QWidget *screen = this->m_desktopWidget->screen(this->m_curScreenNumber);

    QbCaps caps(QString("video/x-raw,"
                        "format=bgr0,"
                        "width=%1,"
                        "height=%2,"
                        "fps=30000/1001").arg(screen->width())
                                         .arg(screen->height()));

    QbPacket packet = QbUtils::imageToPacket(screen->grab().toImage(), caps);

    if (!packet)
        return;

    emit this->oStream(packet);
}

void DesktopCaptureElement::screenCountChanged(int count)
{
    Q_UNUSED(count)

    emit this->mediasChanged(this->medias());
}
