/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef UVCEXTENDEDCONTROLS_H
#define UVCEXTENDEDCONTROLS_H

#include <QObject>

class UvcExtendedControlsPrivate;

class UvcExtendedControls: public QObject
{
    Q_OBJECT

    public:
        UvcExtendedControls(QObject *parent=nullptr);
        UvcExtendedControls(const QString &devicePath);
        UvcExtendedControls(int fd);
        ~UvcExtendedControls();
        void load(const QString &devicePath);
        void load(int fd);
        QVariantList controls(int fd) const;
        QVariantList controls(const QString &devicePath) const;
        bool setControls(int fd, const QVariantMap &controls) const;
        bool setControls(const QString &devicePath, const QVariantMap &controls) const;

    private:
        UvcExtendedControlsPrivate *d;

};

#endif // UVCEXTENDEDCONTROLS_H
