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

#include "multisrcglobals.h"

class MultiSrcGlobalsPrivate
{
    public:
        QString m_codecLib;
        QStringList m_preferredFramework;

        MultiSrcGlobalsPrivate();
};

MultiSrcGlobals::MultiSrcGlobals(QObject *parent):
    QObject(parent)
{
    this->d = new MultiSrcGlobalsPrivate;
    this->resetCodecLib();
}

MultiSrcGlobals::~MultiSrcGlobals()
{
    delete this->d;
}

QString MultiSrcGlobals::codecLib() const
{
    return this->d->m_codecLib;
}

QStringList MultiSrcGlobals::subModules() const
{
    return AkElement::listSubModules("MultiSrc");
}

void MultiSrcGlobals::setCodecLib(const QString &codecLib)
{
    if (this->d->m_codecLib == codecLib)
        return;

    this->d->m_codecLib = codecLib;
    emit this->codecLibChanged(codecLib);
}

void MultiSrcGlobals::resetCodecLib()
{
    auto subModules = AkElement::listSubModules("MultiSrc");

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

MultiSrcGlobalsPrivate::MultiSrcGlobalsPrivate()
{
    this->m_preferredFramework = QStringList {
        "ffmpeg",
        "gstreamer",
        "ndkmedia",
    };
}

#include "moc_multisrcglobals.cpp"
