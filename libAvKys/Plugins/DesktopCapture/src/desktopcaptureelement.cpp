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

#include "desktopcaptureelement.h"

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement()
{
    this->m_fps = AkFrac(30000, 1001);
    this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
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

QObject *DesktopCaptureElement::controlInterface(QQmlEngine *engine,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/DesktopCapture/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("DesktopCapture", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

AkFrac DesktopCaptureElement::fps() const
{
    return this->m_fps;
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

int DesktopCaptureElement::defaultStream(const QString &mimeType)
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

AkCaps DesktopCaptureElement::caps(int stream)
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

void DesktopCaptureElement::sendPacket(const AkPacket &packet)
{
    emit this->oStream(packet);
}

void DesktopCaptureElement::setFps(const AkFrac &fps)
{
    if (this->m_fps == fps)
        return;

    this->m_mutex.lock();
    this->m_fps = fps;
    this->m_mutex.unlock();
    emit this->fpsChanged(fps);
    this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
}

void DesktopCaptureElement::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
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
            this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
            this->m_timer.start();

            return AkElement::setState(state);
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->m_timer.setInterval(qRound(1.e3 * this->m_fps.invert().value()));
            this->m_timer.start();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
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
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void DesktopCaptureElement::readFrame()
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
                                                 &DesktopCaptureElement::sendPacket,
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
