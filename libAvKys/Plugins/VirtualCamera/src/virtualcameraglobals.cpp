/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include <QDir>
#include <QProcessEnvironment>
#include <akelement.h>

#include "virtualcameraglobals.h"

class VirtualCameraGlobalsPrivate
{
    public:
        QString m_outputLib;
        QString m_rootMethod;
        QStringList m_preferredLibrary;
        QStringList m_preferredRootMethod;

        VirtualCameraGlobalsPrivate();
};

VirtualCameraGlobals::VirtualCameraGlobals(QObject *parent):
    QObject(parent)
{
    this->d = new VirtualCameraGlobalsPrivate;
    this->resetOutputLib();
    this->resetRootMethod();
}

VirtualCameraGlobals::~VirtualCameraGlobals()
{
    delete this->d;
}

QString VirtualCameraGlobals::outputLib() const
{
    return this->d->m_outputLib;
}

QStringList VirtualCameraGlobals::outputSubModules() const
{
    return AkElement::listSubModules("VirtualCamera", "output");
}

QString VirtualCameraGlobals::rootMethod() const
{
    return this->d->m_rootMethod;
}

QStringList VirtualCameraGlobals::availableRootMethods() const
{
    QStringList methods;
    QString ext;

#ifdef Q_OS_WIN32
    ext = ".exe";
    auto paths =
            QProcessEnvironment::systemEnvironment().value("Path").split(';');

    static const QStringList sus {
        "runas"
    };
#elif defined(Q_OS_OSX)
    auto paths =
            QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    static const QStringList sus {
        "osascript"
    };
#else
    auto paths =
            QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    static const QStringList sus {
        "beesu",
        "gksu",
        "gksudo",
        "gtksu",
        "kdesu",
        "kdesudo",
        "ktsuss",
        "pkexec",
    };
#endif

    for (auto &su: sus)
        for (auto &path: paths)
            if (QDir(path).exists(su + ext)) {
                methods << su;

                break;
            }

    return methods;
}

void VirtualCameraGlobals::setOutputLib(const QString &outputLib)
{
    if (this->d->m_outputLib == outputLib)
        return;

    this->d->m_outputLib = outputLib;
    emit this->outputLibChanged(outputLib);
}

void VirtualCameraGlobals::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_rootMethod == rootMethod)
        return;

    this->d->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(rootMethod);
}

void VirtualCameraGlobals::resetOutputLib()
{
    auto subModules = AkElement::listSubModules("VirtualCamera", "output");

    for (auto &library: this->d->m_preferredLibrary)
        if (subModules.contains(library)) {
            this->setOutputLib(library);

            return;
        }

    if (this->d->m_outputLib.isEmpty() && !subModules.isEmpty())
        this->setOutputLib(subModules.first());
    else
        this->setOutputLib("");
}

void VirtualCameraGlobals::resetRootMethod()
{
    auto methods = this->availableRootMethods();

    for (auto &method: this->d->m_preferredLibrary)
        if (methods.contains(method)) {
            this->setRootMethod(method);

            return;
        }

    if (this->d->m_rootMethod.isEmpty() && !methods.isEmpty())
        this->setRootMethod(methods.first());
    else
        this->setRootMethod("");
}

VirtualCameraGlobalsPrivate::VirtualCameraGlobalsPrivate()
{
    this->m_preferredLibrary = QStringList {
#ifdef Q_OS_WIN32
        "dshow",
#elif defined(Q_OS_OSX)
        "cmio",
#else
        "akvcam",
        "v4l2lb",
#endif
    };

    this->m_preferredRootMethod = QStringList {
#ifdef Q_OS_WIN32
        "runas",
#elif defined(Q_OS_OSX)
        "osascript",
#else
        // List of possible graphical sudo methods that can be supported:
        //
        // https://en.wikipedia.org/wiki/Comparison_of_privilege_authorization_features#Introduction_to_implementations
        "pkexec",
        "kdesu",
        "kdesudo",
        "gksu",
        "gksudo",
        "gtksu",
        "ktsuss",
        "beesu",
#endif
    };
}

#include "moc_virtualcameraglobals.cpp"
