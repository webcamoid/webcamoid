/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "qb.h"

QbElement::QbElement(QObject *parent): QObject(parent)
{
    this->m_application = NULL;
    this->m_state = ElementStateNull;

    this->resetSrcs();
    this->resetSinks();
}

QbElement::~QbElement()
{
    this->setState(QbElement::ElementStateNull);

    if (this->m_application)
        QMetaObject::invokeMethod(this->m_application,
                                  "deleteInstance",
                                  Q_ARG(QString, this->m_pluginId));
}

QbElement::ElementState QbElement::state()
{
    return this->m_state;
}

QList<QbElement *> QbElement::srcs()
{
    return this->m_srcs;
}

QList<QbElement *> QbElement::sinks()
{
    return this->m_sinks;
}
bool QbElement::link(QObject *dstElement, Qt::ConnectionType connectionType)
{
    if (!dstElement)
        return false;

    foreach (QMetaMethod signal, this->methodsByName(this, "oStream"))
        foreach (QMetaMethod slot, this->methodsByName(dstElement, "iStream"))
            if (this->methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::connect(this, signal, dstElement, slot, connectionType);

    return true;
}

bool QbElement::link(QbElementPtr dstElement, Qt::ConnectionType connectionType)
{
    if (!this->link(static_cast<QObject *>(dstElement.data()), connectionType))
        return false;

    this->setSinks(this->sinks() << dstElement.data());
    dstElement->setSrcs(dstElement->srcs() << this);

    return true;
}

bool QbElement::unlink(QObject *dstElement)
{
    if (!dstElement)
        return false;

    foreach (QMetaMethod signal, this->methodsByName(this, "oStream"))
        foreach (QMetaMethod slot, this->methodsByName(dstElement, "iStream"))
            if (this->methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(this, signal, dstElement, slot);

    return true;
}

bool QbElement::unlink(QbElementPtr dstElement)
{
    if (!this->unlink(static_cast<QObject *>(dstElement.data())))
        return false;

    QList<QbElement *> sinks = this->m_sinks;
    sinks.removeOne(dstElement.data());
    this->setSinks(sinks);

    QList<QbElement *> srcs = dstElement->m_srcs;
    srcs.removeOne(this);
    dstElement->setSrcs(srcs);

    return true;
}

void QbElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
}

QList<QMetaMethod> QbElement::methodsByName(QObject *object,QString methodName)
{
    QList<QMetaMethod> methods;
    QStringList methodSignatures;

    for (int i = 0; i < object->metaObject()->methodCount(); i++) {
        QMetaMethod method = object->metaObject()->method(i);

#if QT_VERSION >= 0x050000
        const char *signature = method.methodSignature().constData();
#else
        const char *signature = method.signature();
#endif // QT_VERSION >= 0x050000

        if (QRegExp(QString("\\s*%1\\s*\\(.*").arg(methodName)).exactMatch(signature))
            if (!methodSignatures.contains(signature)) {
                methods << method;
                methodSignatures << signature;
            }
    }

    return methods;
}

bool QbElement::methodCompat(QMetaMethod method1, QMetaMethod method2)
{
    if (method1.parameterTypes() == method2.parameterTypes())
        return true;

    return false;
}

void QbElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)
}

void QbElement::setState(QbElement::ElementState state)
{
    QbElement::ElementState preState = this->m_state;

    if (state == ElementStateNull
        && preState != state) {
        if (preState == ElementStatePlaying) {
            this->setState(ElementStatePaused);
            preState = this->m_state;
        }

        this->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
    else if (state == ElementStatePaused
             && preState != state) {
        this->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
    else if (state == ElementStatePlaying
             && preState != state) {
        if (preState == ElementStateNull) {
            this->setState(ElementStatePaused);
            preState = this->m_state;
        }

        this->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
}

void QbElement::setSrcs(QList<QbElement *> srcs)
{
    this->m_srcs = srcs;
}

void QbElement::setSinks(QList<QbElement *> sinks)
{
    this->m_sinks = sinks;
}

void QbElement::resetState()
{
    this->setState(ElementStateNull);
}

void QbElement::resetSrcs()
{
    this->setSrcs(QList<QbElement *>());
}

void QbElement::resetSinks()
{
    this->setSinks(QList<QbElement *>());
}
