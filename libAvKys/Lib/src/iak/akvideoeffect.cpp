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

#include "akvideoeffect.h"

#include "moc_akvideoeffect.cpp"

AkVideoEffect::AkVideoEffect(QObject *parent):
    QObject(parent)
{
}

AkVideoEffect::~AkVideoEffect()
{
}

QObject *AkVideoEffect::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(engine)
    Q_UNUSED(controlId)

    return nullptr;
}

QString AkVideoEffect::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return nullptr;
}

void AkVideoEffect::controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const
{
    Q_UNUSED(context)
    Q_UNUSED(controlId)
}

void AkVideoEffect::setGLFunctions(QOpenGLFunctions *gl)
{
    this->m_gl = gl;
}
