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

#include <QSettings>
#include <QSharedPointer>
#include <QQmlContext>
#include <akfrac.h>
#include <akcaps.h>

#include "desktopcaptureelement.h"
#include "desktopcaptureelementsettings.h"
#include "screendev.h"

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

using ScreenDevPtr = QSharedPointer<ScreenDev>;

class DesktopCaptureElementPrivate
{
    public:
        DesktopCaptureElement *self;
        DesktopCaptureElementSettings m_settings;
        ScreenDevPtr m_screenCapture;

        explicit DesktopCaptureElementPrivate(DesktopCaptureElement *self);
        void captureLibUpdated(const QString &captureLib);
};

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new DesktopCaptureElementPrivate(this);
    QObject::connect(&this->d->m_settings,
                     &DesktopCaptureElementSettings::captureLibChanged,
                     [this] (const QString &captureLib) {
                        this->d->captureLibUpdated(captureLib);
                     });

    this->d->captureLibUpdated(this->d->m_settings.captureLib());
}

DesktopCaptureElement::~DesktopCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

AkFrac DesktopCaptureElement::fps() const
{
    if (!this->d->m_screenCapture)
        return AkFrac();

    return this->d->m_screenCapture->fps();
}

QStringList DesktopCaptureElement::medias()
{
    if (!this->d->m_screenCapture)
        return {};

    return this->d->m_screenCapture->medias();
}

QString DesktopCaptureElement::media() const
{
    if (!this->d->m_screenCapture)
        return {};

    return this->d->m_screenCapture->media();
}

QList<int> DesktopCaptureElement::streams()
{
    if (!this->d->m_screenCapture)
        return {};

    return this->d->m_screenCapture->streams();
}

int DesktopCaptureElement::defaultStream(const QString &mimeType)
{
    if (!this->d->m_screenCapture)
        return 0;

    return this->d->m_screenCapture->defaultStream(mimeType);
}

QString DesktopCaptureElement::description(const QString &media)
{
    if (!this->d->m_screenCapture)
        return {};

    return this->d->m_screenCapture->description(media);
}

AkCaps DesktopCaptureElement::caps(int stream)
{
    if (!this->d->m_screenCapture)
        return AkCaps();

    return this->d->m_screenCapture->caps(stream);
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
    if (!this->d->m_screenCapture)
        return;

    this->d->m_screenCapture->setFps(fps);

    QSettings settings;
    settings.beginGroup("DesktopCapture");
    settings.setValue("fps", fps.toString());
    settings.endGroup();
}

void DesktopCaptureElement::resetFps()
{
    if (this->d->m_screenCapture)
        this->d->m_screenCapture->resetFps();
}

void DesktopCaptureElement::setMedia(const QString &media)
{
    if (this->d->m_screenCapture)
        this->d->m_screenCapture->setMedia(media);
}

void DesktopCaptureElement::resetMedia()
{
    if (this->d->m_screenCapture)
        this->d->m_screenCapture->resetMedia();
}

bool DesktopCaptureElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_screenCapture)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!this->d->m_screenCapture->init())
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
            if (!this->d->m_screenCapture->init())
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
            this->d->m_screenCapture->uninit();

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

}

void DesktopCaptureElementPrivate::captureLibUpdated(const QString &captureLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_screenCapture =
            ptr_cast<ScreenDev>(DesktopCaptureElement::loadSubModule("DesktopCapture",
                                                                     captureLib));

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
