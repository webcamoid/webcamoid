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

#include "desktopcaptureelement.h"
#include "desktopcaptureglobals.h"

Q_GLOBAL_STATIC(DesktopCaptureGlobals, globalDesktopCapture)

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(static_cast<T *>(obj));
}

DesktopCaptureElement::DesktopCaptureElement():
    AkMultimediaSourceElement(),
    m_screenCapture(ptr_init<ScreenDev>())
{
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
    return this->m_screenCapture->fps();
}

QStringList DesktopCaptureElement::medias()
{
    return this->m_screenCapture->medias();
}

QString DesktopCaptureElement::media() const
{
    return this->m_screenCapture->media();
}

QList<int> DesktopCaptureElement::streams() const
{
    return this->m_screenCapture->streams();
}

int DesktopCaptureElement::defaultStream(const QString &mimeType)
{
    return this->m_screenCapture->defaultStream(mimeType);
}

QString DesktopCaptureElement::description(const QString &media)
{
    return this->m_screenCapture->description(media);
}

AkCaps DesktopCaptureElement::caps(int stream)
{
    return this->m_screenCapture->caps(stream);
}

QString DesktopCaptureElement::captureLib() const
{
    return globalDesktopCapture->captureLib();
}

void DesktopCaptureElement::setFps(const AkFrac &fps)
{
    this->m_screenCapture->setFps(fps);
}

void DesktopCaptureElement::resetFps()
{
    this->m_screenCapture->resetFps();
}

void DesktopCaptureElement::setMedia(const QString &media)
{
    this->m_screenCapture->setMedia(media);
}

void DesktopCaptureElement::resetMedia()
{
    this->m_screenCapture->resetMedia();
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
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!this->m_screenCapture->init())
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
            if (!this->m_screenCapture->init())
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
            this->m_screenCapture->uninit();

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

    this->m_screenCapture =
            ptr_init<ScreenDev>(this->loadSubModule("DesktopCapture", captureLib));

    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::mediasChanged,
                     this,
                     &DesktopCaptureElement::mediasChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::mediaChanged,
                     this,
                     &DesktopCaptureElement::mediaChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::streamsChanged,
                     this,
                     &DesktopCaptureElement::streamsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::streamsChanged,
                     this,
                     &DesktopCaptureElement::streamsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::fpsChanged,
                     this,
                     &DesktopCaptureElement::fpsChanged);
    QObject::connect(this->m_screenCapture.data(),
                     &ScreenDev::sizeChanged,
                     this,
                     &DesktopCaptureElement::sizeChanged);
    QObject::connect(this->m_screenCapture.data(),
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
