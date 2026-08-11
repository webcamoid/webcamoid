/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <QSettings>
#include <QSharedPointer>
#include <QQmlContext>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "desktopcaptureelement.h"
#include "screendev.h"

using ScreenDevPtr = QSharedPointer<ScreenDev>;

class DesktopCaptureElementPrivate
{
    public:
        DesktopCaptureElement *self;
        ScreenDevPtr m_screenCapture;
        QString m_screenCaptureImpl;
        bool m_canCaptureWindows {false};
        QMutex m_mutex;

        explicit DesktopCaptureElementPrivate(DesktopCaptureElement *self);
        void linksChanged(const AkPluginLinks &links);
};

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new DesktopCaptureElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_screenCapture) {
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::mediasChanged,
                         this,
                         &DesktopCaptureElement::mediasChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::mediaChanged,
                         this,
                         &DesktopCaptureElement::mediaChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::showCursorChanged,
                         this,
                         &DesktopCaptureElement::showCursorChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::cursorSizeChanged,
                         this,
                         &DesktopCaptureElement::cursorSizeChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::streamsChanged,
                         this,
                         &DesktopCaptureElement::streamsChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::streamsChanged,
                         this,
                         &DesktopCaptureElement::streamsChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::fpsChanged,
                         this,
                         &DesktopCaptureElement::fpsChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::sizeChanged,
                         this,
                         &DesktopCaptureElement::sizeChanged);
        QObject::connect(this->d->m_screenCapture.data(),
                         &ScreenDev::oStream,
                         this,
                         &DesktopCaptureElement::oStream,
                         Qt::DirectConnection);

        auto medias = this->d->m_screenCapture->medias();

        if (!medias.isEmpty())
            this->d->m_screenCapture->setMedia(medias.first());
    }
}

DesktopCaptureElement::~DesktopCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

AkFrac DesktopCaptureElement::fps() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->fps();
}

QStringList DesktopCaptureElement::medias()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->medias();
}

QString DesktopCaptureElement::media() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->media();
}

QList<int> DesktopCaptureElement::streams()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->streams();
}

int DesktopCaptureElement::defaultStream(AkCaps::CapsType type)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return 0;

    return screenCapture->defaultStream(type);
}

QString DesktopCaptureElement::description(const QString &media)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->description(media);
}

AkCaps DesktopCaptureElement::caps(int stream)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return {};

    return screenCapture->caps(stream);
}

bool DesktopCaptureElement::canCaptureWindows() const
{
    return this->d->m_canCaptureWindows;
}

bool DesktopCaptureElement::canCaptureCursor() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return false;

    return screenCapture->canCaptureCursor();
}

bool DesktopCaptureElement::canChangeCursorSize() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return false;

    return screenCapture->canChangeCursorSize();
}

bool DesktopCaptureElement::showCursor() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return false;

    return screenCapture->showCursor();
}

int DesktopCaptureElement::cursorSize() const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return 0;

    return screenCapture->cursorSize();
}

bool DesktopCaptureElement::isWindow(const QString &media) const
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (!screenCapture)
        return false;

    return screenCapture->isWindow(media);
}

QString DesktopCaptureElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/DesktopCapture/share/qml/main.qml");
}

void DesktopCaptureElement::controlInterfaceConfigure(QQmlContext *context,
                                                      const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("DesktopCapture", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void DesktopCaptureElement::setFps(const AkFrac &fps)
{
    {
        QMutexLocker locker(&this->d->m_mutex);
        auto screenCapture = this->d->m_screenCapture;

        if (screenCapture)
            screenCapture->setFps(fps);
    }

    QSettings settings;
    settings.beginGroup("DesktopCapture");
    settings.setValue("fps", fps.toString());
    settings.endGroup();
}

void DesktopCaptureElement::resetFps()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->resetFps();
}

void DesktopCaptureElement::setMedia(const QString &media)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->setMedia(media);
}

void DesktopCaptureElement::setShowCursor(bool showCursor)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->setShowCursor(showCursor);
}

void DesktopCaptureElement::setCursorSize(int cursorSize)
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->setCursorSize(cursorSize);
}

void DesktopCaptureElement::resetMedia()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->resetMedia();
}

void DesktopCaptureElement::resetShowCursor()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->resetShowCursor();
}

void DesktopCaptureElement::resetCursorSize()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->resetCursorSize();
}

bool DesktopCaptureElement::setState(AkElement::ElementState state)
{
    {
        QMutexLocker locker(&this->d->m_mutex);
        auto screenCapture = this->d->m_screenCapture;

        if (!screenCapture)
            return false;
    }

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying: {
            bool result = false;

            {
                QMutexLocker locker(&this->d->m_mutex);
                result = this->d->m_screenCapture->init();
            }

            if (!result)
                return false;

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying: {
            bool result = false;

            {
                QMutexLocker locker(&this->d->m_mutex);
                result = this->d->m_screenCapture->init();
            }

            if (!result)
                return false;

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
        case AkElement::ElementStatePaused: {
            QMutexLocker locker(&this->d->m_mutex);
            this->d->m_screenCapture->uninit();

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void DesktopCaptureElement::updateDevices()
{
    QMutexLocker locker(&this->d->m_mutex);
    auto screenCapture = this->d->m_screenCapture;

    if (screenCapture)
        screenCapture->updateDevices();
}

DesktopCaptureElementPrivate::DesktopCaptureElementPrivate(DesktopCaptureElement *self):
    self(self)
{
    this->m_screenCapture = akPluginManager->create<ScreenDev>("VideoSource/DesktopCapture/Impl/*");
    this->m_screenCaptureImpl = akPluginManager->defaultPlugin("VideoSource/DesktopCapture/Impl/*",
                                                               {"DesktopCaptureImpl"}).id();

    if (this->m_screenCapture)
        this->m_canCaptureWindows = this->m_screenCapture->canCaptureWindows();
}

void DesktopCaptureElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("VideoSource/DesktopCapture/Impl/*")
        || links["VideoSource/DesktopCapture/Impl/*"] == this->m_screenCaptureImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    {
        QMutexLocker locker(&this->m_mutex);
        this->m_screenCapture =
                akPluginManager->create<ScreenDev>("VideoSource/DesktopCapture/Impl/*");
    }

    this->m_screenCaptureImpl = links["VideoSource/DesktopCapture/Impl/*"];

    if (!this->m_screenCapture)
        return;

    QSettings settings;
    settings.beginGroup("DesktopCapture");
    auto fps = settings.value("fps", 30).toString();
    this->m_screenCapture->setFps(AkFrac(fps));
    settings.endGroup();

    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::mediasChanged,
                     self,
                     &DesktopCaptureElement::mediasChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::mediaChanged,
                     self,
                     &DesktopCaptureElement::mediaChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::showCursorChanged,
                     self,
                     &DesktopCaptureElement::showCursorChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::cursorSizeChanged,
                     self,
                     &DesktopCaptureElement::cursorSizeChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::streamsChanged,
                     self,
                     &DesktopCaptureElement::streamsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::streamsChanged,
                     self,
                     &DesktopCaptureElement::streamsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::fpsChanged,
                     self,
                     &DesktopCaptureElement::fpsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::sizeChanged,
                     self,
                     &DesktopCaptureElement::sizeChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::oStream,
                     self,
                     &DesktopCaptureElement::oStream,
                     Qt::DirectConnection);

    emit self->mediasChanged(self->medias());
    emit self->streamsChanged(self->streams());
    auto canCaptureWindows = this->m_screenCapture->canCaptureWindows();

    if (canCaptureWindows != this->m_canCaptureWindows) {
        this->m_canCaptureWindows = canCaptureWindows;
        emit self->canCaptureWindowsChanged(canCaptureWindows);
    }

    auto medias = self->medias();

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

#include "moc_desktopcaptureelement.cpp"
