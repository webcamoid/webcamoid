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

#ifndef VIDEOCAPTUREELEMENTSETTINGS_H
#define VIDEOCAPTUREELEMENTSETTINGS_H

#include <QObject>

class VideoCaptureElementSettings: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString codecLib
               READ codecLib
               WRITE setCodecLib
               RESET resetCodecLib
               NOTIFY codecLibChanged)
    Q_PROPERTY(QString captureLib
               READ captureLib
               WRITE setCaptureLib
               RESET resetCaptureLib
               NOTIFY captureLibChanged)
    Q_PROPERTY(QStringList codecSubModules
               READ codecSubModules
               NOTIFY codecSubModulesChanged)
    Q_PROPERTY(QStringList captureSubModules
               READ captureSubModules
               NOTIFY captureSubModulesChanged)

    public:
        VideoCaptureElementSettings(QObject *parent=nullptr);

        Q_INVOKABLE QString codecLib() const;
        Q_INVOKABLE QString captureLib() const;
        Q_INVOKABLE QStringList codecSubModules() const;
        Q_INVOKABLE QStringList captureSubModules() const;

    signals:
        void codecLibChanged(const QString &codecLib);
        void captureLibChanged(const QString &captureLib);
        void codecSubModulesChanged(const QStringList &codecSubModules);
        void captureSubModulesChanged(const QStringList &captureSubModules);

    public slots:
        void setCodecLib(const QString &codecLib);
        void setCaptureLib(const QString &captureLib);
        void resetCodecLib();
        void resetCaptureLib();
};

#endif // VIDEOCAPTUREELEMENTSETTINGS_H
