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

#include <akelement.h>

#include "multisinkglobals.h"

class MultiSinkGlobalsPrivate
{
    public:
        QString m_codecLib;
        QStringList m_preferredFramework;

        MultiSinkGlobalsPrivate();
};

MultiSinkGlobals::MultiSinkGlobals(QObject *parent):
    QObject(parent)
{
    this->d = new MultiSinkGlobalsPrivate;
    this->resetCodecLib();
}

MultiSinkGlobals::~MultiSinkGlobals()
{
    delete this->d;
}

QString MultiSinkGlobals::codecLib() const
{
    return this->d->m_codecLib;
}

QStringList MultiSinkGlobals::subModules() const
{
    return AkElement::listSubModules("MultiSink");
}

void MultiSinkGlobals::setCodecLib(const QString &codecLib)
{
    if (this->d->m_codecLib == codecLib)
        return;

    this->d->m_codecLib = codecLib;
    emit this->codecLibChanged(codecLib);
}

void MultiSinkGlobals::resetCodecLib()
{
    auto subModules = AkElement::listSubModules("MultiSink");

    for (auto &framework: this->d->m_preferredFramework)
        if (subModules.contains(framework)) {
            this->setCodecLib(framework);

            return;
        }

    if (this->d->m_codecLib.isEmpty() && !subModules.isEmpty())
        this->setCodecLib(subModules.first());
    else
        this->setCodecLib("");
}

MultiSinkGlobalsPrivate::MultiSinkGlobalsPrivate()
{
    this->m_preferredFramework = QStringList {
        "ffmpeg",
        "gstreamer"
    };
}

#include "moc_multisinkglobals.cpp"
