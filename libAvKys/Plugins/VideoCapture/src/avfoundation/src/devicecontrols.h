/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#ifndef DEVICECONTROLS_H
#define DEVICECONTROLS_H

#include <QString>
#include <QVariantList>

class DeviceControlsPrivate;

class DeviceControls
{
    public:
        explicit DeviceControls();
        ~DeviceControls();

        bool open(const QString &deviceId);
        void close();
        QVariantList imageControls();
        QVariantList cameraControls();
        void setImageControls(const QVariantMap &imageControls);
        void setCameraControls(const QVariantMap &cameraControls);

    private:
        DeviceControlsPrivate *d;
};

#endif // DEVICECONTROLS_H
