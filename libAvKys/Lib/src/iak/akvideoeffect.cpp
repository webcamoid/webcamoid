/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickItem>

#include "akvideoeffect.h"

AkVideoEffect::AkVideoEffect(QObject *parent):
    QObject(parent)
{
}

AkVideoEffect::~AkVideoEffect()
{
}

QObject *AkVideoEffect::controlInterface(QQmlEngine *engine,
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
    auto item = component.create(context);

    if (!item) {
        delete context;

        return nullptr;
    }

    QQmlEngine::setObjectOwnership(item, QQmlEngine::JavaScriptOwnership);
    context->setParent(item);

    return item;
}

QString AkVideoEffect::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return nullptr;
}

void AkVideoEffect::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(context)
    Q_UNUSED(controlId)
}

void AkVideoEffect::setGLFunctions(QOpenGLFunctions *gl)
{
    this->m_gl = gl;
}

#include "moc_akvideoeffect.cpp"
