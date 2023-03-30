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

#include <QDataStream>
#include <QDebug>
#include <QMetaMethod>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>

#include "akelement.h"
#include "akpacket.h"
#include "akaudiopacket.h"
#include "akvideopacket.h"

class AkElementPrivate
{
    public:
        AkElement::ElementState m_state {AkElement::ElementStateNull};

        AkElementPrivate();
        static QList<QMetaMethod> methodsByName(const QObject *object,
                                                const QString &methodName);
        static bool methodCompat(const QMetaMethod &method1,
                                 const QMetaMethod &method2);
};

AkElement::AkElement(QObject *parent):
    QObject(parent)
{
    this->d = new AkElementPrivate();
}

AkElement::~AkElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

AkElement::ElementState AkElement::state() const
{
    return this->d->m_state;
}

QObject *AkElement::controlInterface(QQmlEngine *engine,
                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return nullptr;

    auto qmlFile = this->controlInterfaceProvide(controlId);

    if (qmlFile.isEmpty())
        return nullptr;

    // Load the UI from the plugin.
    QQmlComponent component(engine, qmlFile);

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return nullptr;
    }

    // Create a context for the plugin.
    auto context = new QQmlContext(engine->rootContext());
    this->controlInterfaceConfigure(context, controlId);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return nullptr;
    }

    QQmlEngine::setObjectOwnership(item, QQmlEngine::JavaScriptOwnership);
    context->setParent(item);

    return item;
}

bool AkElement::link(const QObject *dstElement,
                     Qt::ConnectionType connectionType) const
{
    return AkElement::link(this, dstElement, connectionType);
}

bool AkElement::link(const AkElementPtr &dstElement,
                     Qt::ConnectionType connectionType) const
{
    return this->link(static_cast<QObject *>(dstElement.data()),
                      connectionType);
}

bool AkElement::unlink(const QObject *dstElement) const
{
    return AkElement::unlink(this, dstElement);
}

bool AkElement::unlink(const AkElementPtr &dstElement) const
{
    return this->unlink(static_cast<QObject *>(dstElement.data()));
}

bool AkElement::link(const AkElementPtr &srcElement,
                     const QObject *dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool AkElement::link(const AkElementPtr &srcElement,
                     const AkElementPtr &dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool AkElement::link(const QObject *srcElement,
                     const QObject *dstElement,
                     Qt::ConnectionType connectionType)
{
    if (!srcElement || !dstElement)
        return false;

    auto signalList = AkElementPrivate::methodsByName(srcElement, "oStream");
    auto slotList = AkElementPrivate::methodsByName(dstElement, "iStream");

    for (auto &signal: signalList)
        for (auto &slot: slotList)
            if (AkElementPrivate::methodCompat(signal, slot)
                && signal.methodType() == QMetaMethod::Signal
                && slot.methodType() == QMetaMethod::Slot)
                QObject::connect(srcElement,
                                 signal,
                                 dstElement,
                                 slot,
                                 connectionType);

    return true;
}

bool AkElement::unlink(const AkElementPtr &srcElement,
                       const QObject *dstElement)
{
    return srcElement->unlink(dstElement);
}

bool AkElement::unlink(const AkElementPtr &srcElement,
                       const AkElementPtr &dstElement)
{
    return srcElement->unlink(dstElement);
}

bool AkElement::unlink(const QObject *srcElement, const QObject *dstElement)
{
    if (!srcElement || !dstElement)
        return false;

    for (auto &signal: AkElementPrivate::methodsByName(srcElement, "oStream"))
        for (auto &slot: AkElementPrivate::methodsByName(dstElement, "iStream"))
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(srcElement, signal, dstElement, slot);

    return true;
}

QString AkElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString();
}

void AkElement::controlInterfaceConfigure(QQmlContext *context,
                                          const QString &controlId) const
{
    Q_UNUSED(context)
    Q_UNUSED(controlId)
}

AkPacket AkElement::iAudioStream(const AkAudioPacket &packet)
{
    Q_UNUSED(packet)

    return {};
}

AkPacket AkElement::iVideoStream(const AkVideoPacket &packet)
{
    Q_UNUSED(packet)

    return {};
}

AkPacket AkElement::iStream(const AkPacket &packet)
{
    switch (packet.type()) {
    case AkPacket::PacketAudio:
        return this->iAudioStream(packet);
    case AkPacket::PacketVideo:
        return this->iVideoStream(packet);
    default:
        break;
    }

    return {};
}

bool AkElement::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return false;

    this->d->m_state = state;
    emit this->stateChanged(state);

    return true;
}

void AkElement::resetState()
{
    this->setState(ElementStateNull);
}

void AkElement::registerTypes()
{
    qRegisterMetaType<AkElementPtr>("AkElementPtr");
    qRegisterMetaType<ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<ElementState>("ElementState");
    qmlRegisterSingletonType<AkElement>("Ak", 1, 0, "AkElement",
                                        [] (QQmlEngine *qmlEngine,
                                            QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkElement();
    });
}

AkElementPrivate::AkElementPrivate()
{
}

QList<QMetaMethod> AkElementPrivate::methodsByName(const QObject *object,
                                                   const QString &methodName)
{
    QList<QMetaMethod> methods;
    QStringList methodSignatures;

    for (int i = 0; i < object->metaObject()->methodCount(); i++) {
        QMetaMethod method = object->metaObject()->method(i);
        QString signature(method.methodSignature());

        if (QRegExp(QString(R"(\s*%1\s*\(.*)").arg(methodName))
            .exactMatch(signature))
            if (!methodSignatures.contains(signature)) {
                methods << method;
                methodSignatures << signature;
            }
    }

    return methods;
}

bool AkElementPrivate::methodCompat(const QMetaMethod &method1,
                                    const QMetaMethod &method2)
{
    return method1.parameterTypes() == method2.parameterTypes();
}

QDataStream &operator >>(QDataStream &istream, AkElement::ElementState &state)
{
    int stateInt;
    istream >> stateInt;
    state = static_cast<AkElement::ElementState>(stateInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, AkElement::ElementState state)
{
    ostream << static_cast<int>(state);

    return ostream;
}

#include "moc_akelement.cpp"
