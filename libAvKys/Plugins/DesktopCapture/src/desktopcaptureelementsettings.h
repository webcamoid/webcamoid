/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef DESKTOPCAPTUREELEMENTSETTINGS_H
#define DESKTOPCAPTUREELEMENTSETTINGS_H

#include <QObject>

class DesktopCaptureElementSettings: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString captureLib
               READ captureLib
               WRITE setCaptureLib
               RESET resetCaptureLib
               NOTIFY captureLibChanged)
    Q_PROPERTY(QStringList subModules
               READ subModules
               NOTIFY subModulesChanged)

    public:
        DesktopCaptureElementSettings(QObject *parent=nullptr);

        Q_INVOKABLE QString captureLib() const;
        Q_INVOKABLE QStringList subModules() const;

    signals:
        void captureLibChanged(const QString &captureLib);
        void subModulesChanged(const QStringList &subModules);

    public slots:
        void setCaptureLib(const QString &captueLib);
        void resetCaptureLib();
};

#endif // DESKTOPCAPTUREELEMENTSETTINGS_H
