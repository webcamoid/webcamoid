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

#ifndef VIRTUALCAMERAELEMENTSETTINGS_H
#define VIRTUALCAMERAELEMENTSETTINGS_H

#include <QObject>

class VirtualCameraElementSettings: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString outputLib
               READ outputLib
               WRITE setOutputLib
               RESET resetOutputLib
               NOTIFY outputLibChanged)
    Q_PROPERTY(QStringList outputSubModules
               READ outputSubModules
               NOTIFY outputSubModulesChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableRootMethods
               READ availableRootMethods
               NOTIFY availableRootMethodsChanged)

    public:
        VirtualCameraElementSettings(QObject *parent=nullptr);

        Q_INVOKABLE QString outputLib() const;
        Q_INVOKABLE QStringList outputSubModules() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableRootMethods() const;

    signals:
        void outputLibChanged(const QString &outputLib);
        void outputSubModulesChanged(const QStringList &outputSubModules);
        void rootMethodChanged(const QString &rootMethod);
        void availableRootMethodsChanged(const QStringList &availableRoot);

    public slots:
        void setOutputLib(const QString &outputLib);
        void setRootMethod(const QString &rootMethod);
        void resetOutputLib();
        void resetRootMethod();
};

#endif // VIRTUALCAMERAELEMENTSETTINGS_H
