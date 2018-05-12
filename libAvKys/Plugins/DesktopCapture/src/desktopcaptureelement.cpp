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

#include <QSharedPointer>
#include <QQmlContext>
#include <akfrac.h>
#include <akcaps.h>

#include "desktopcaptureelement.h"
#include "desktopcaptureglobals.h"
#include "screendev.h"

Q_GLOBAL_STATIC(DesktopCaptureGlobals, globalDesktopCapture)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

typedef QSharedPointer<ScreenDev> ScreenDevPtr;

class DesktopCaptureElementPrivate
{
    public:
        ScreenDevPtr m_screenCapture;
};

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new DesktopCaptureElementPrivate;

    QObject::connect(globalDesktopCapture,
                     SIGNAL(captureLibChanged(const QString &)),
                     this,
                     SIGNAL(captureLibChanged(const QString &)));
    QObject::connect(globalDesktopCapture,
                     SIGNAL(captureLibChanged(const QString &)),
                     this,
                     SLOT(captureLibUpdated(const QString &)));

    this->captureLibUpdated(globalDesktopCapture->captureLib());
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

QString DesktopCaptureElement::captureLib() const
{
    return globalDesktopCapture->captureLib();
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
    if (this->d->m_screenCapture)
        this->d->m_screenCapture->setFps(fps);
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

void DesktopCaptureElement::setCaptureLib(const QString &captureLib)
{
    globalDesktopCapture->setCaptureLib(captureLib);
}

void DesktopCaptureElement::resetCaptureLib()
{
    globalDesktopCapture->resetCaptureLib();
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

void DesktopCaptureElement::captureLibUpdated(const QString &captureLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_screenCapture =
            ptr_cast<ScreenDev>(this->loadSubModule("DesktopCapture",
                                                    captureLib));

    if (!this->d->m_screenCapture)
        return;

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

    emit this->mediasChanged(this->medias());
    emit this->streamsChanged(this->streams());

    auto medias = this->medias();

    if (!medias.isEmpty())
        this->setMedia(medias.first());

    this->setState(state);
}

#include "moc_desktopcaptureelement.cpp"
