/* Webcamoid, webcam capture application.
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
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    AkFrac fps;

    if (screenCapture)
        fps = screenCapture->fps();

    return fps;
}

QStringList DesktopCaptureElement::medias()
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    QStringList medias;

    if (screenCapture)
        medias = screenCapture->medias();

    return medias;
}

QString DesktopCaptureElement::media() const
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    QString media;

    if (screenCapture)
        media = screenCapture->media();

    return media;
}

QList<int> DesktopCaptureElement::streams()
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    QList<int> streams;

    if (screenCapture)
        streams = screenCapture->streams();

    return streams;
}

int DesktopCaptureElement::defaultStream(AkCaps::CapsType type)
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    int stream = 0;

    if (screenCapture)
        stream = screenCapture->defaultStream(type);

    return stream;
}

QString DesktopCaptureElement::description(const QString &media)
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    QString description;

    if (screenCapture)
        description = screenCapture->description(media);

    return description;
}

AkCaps DesktopCaptureElement::caps(int stream)
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    AkVideoCaps caps;

    if (screenCapture)
        caps = screenCapture->caps(stream);

    return caps;
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
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    if (screenCapture)
        screenCapture->setFps(fps);

    QSettings settings;
    settings.beginGroup("DesktopCapture");
    settings.setValue("fps", fps.toString());
    settings.endGroup();
}

void DesktopCaptureElement::resetFps()
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    if (screenCapture)
        screenCapture->resetFps();
}

void DesktopCaptureElement::setMedia(const QString &media)
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    if (screenCapture)
        screenCapture->setMedia(media);
}

void DesktopCaptureElement::resetMedia()
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    if (screenCapture)
        screenCapture->resetMedia();
}

bool DesktopCaptureElement::setState(AkElement::ElementState state)
{
    this->d->m_mutex.lock();
    auto screenCapture = this->d->m_screenCapture;
    this->d->m_mutex.unlock();

    if (!screenCapture)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!screenCapture->init())
                return false;

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
            if (!screenCapture->init())
                return false;

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
            screenCapture->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

DesktopCaptureElementPrivate::DesktopCaptureElementPrivate(DesktopCaptureElement *self):
    self(self)
{
    this->m_screenCapture = akPluginManager->create<ScreenDev>("VideoSource/DesktopCapture/Impl/*");
    this->m_screenCaptureImpl = akPluginManager->defaultPlugin("VideoSource/DesktopCapture/Impl/*",
                                                               {"DesktopCaptureImpl"}).id();
}

void DesktopCaptureElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("VideoSource/DesktopCapture/Impl/*")
        || links["VideoSource/DesktopCapture/Impl/*"] == this->m_screenCaptureImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutex.lock();
    this->m_screenCapture =
            akPluginManager->create<ScreenDev>("VideoSource/DesktopCapture/Impl/*");
    this->m_mutex.unlock();

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

    auto medias = self->medias();

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

#include "moc_desktopcaptureelement.cpp"
