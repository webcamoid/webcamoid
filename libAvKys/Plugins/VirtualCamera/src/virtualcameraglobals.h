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

#ifndef VIRTUALCAMERAGLOBALS_H
#define VIRTUALCAMERAGLOBALS_H

#include <QObject>

class VirtualCameraGlobals: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString convertLib
               READ convertLib
               WRITE setConvertLib
               RESET resetConvertLib
               NOTIFY convertLibChanged)
    Q_PROPERTY(QString outputLib
               READ outputLib
               WRITE setOutputLib
               RESET resetOutputLib
               NOTIFY outputLibChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableMethods
               READ availableMethods)

    public:
        explicit VirtualCameraGlobals(QObject *parent=nullptr);

        Q_INVOKABLE QString convertLib() const;
        Q_INVOKABLE QString outputLib() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableMethods() const;

    private:
        QString m_convertLib;
        QString m_outputLib;
        QString m_rootMethod;
        QStringList m_preferredFramework;
        QStringList m_preferredLibrary;
        QStringList m_preferredRootMethod;

    signals:
        void convertLibChanged(const QString &convertLib);
        void outputLibChanged(const QString &outputLib);
        void rootMethodChanged(const QString &rootMethod);

    public slots:
        void setConvertLib(const QString &convertLib);
        void setOutputLib(const QString &outputLib);
        void setRootMethod(const QString &rootMethod);
        void resetConvertLib();
        void resetOutputLib();
        void resetRootMethod();
};

#endif // VIRTUALCAMERAGLOBALS_H
