/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <qb.h>

class Capture: public QObject
{
    Q_PROPERTY(QStringList webcams READ webcams NOTIFY webcamsChanged)
    Q_PROPERTY(QString device READ device WRITE setDevice RESET resetDevice)
    Q_PROPERTY(QString ioMethod READ ioMethod WRITE setIoMethod RESET resetIoMethod)
    Q_PROPERTY(int nBuffers READ nBuffers WRITE setNBuffers RESET resetNBuffers)
    Q_PROPERTY(bool isCompressed READ isCompressed)
    Q_PROPERTY(QString caps READ caps)

    public:
        explicit Capture();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE bool isCompressed() const;
        Q_INVOKABLE QString caps() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList availableSizes(const QString &webcam) const;
        Q_INVOKABLE QSize size(const QString &webcam) const;
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size) const;
        Q_INVOKABLE bool resetSize(const QString &webcam) const;
        Q_INVOKABLE QVariantList controls(const QString &webcam) const;
        Q_INVOKABLE bool setControls(const QString &webcam, const QVariantMap &controls) const;
        Q_INVOKABLE bool resetControls(const QString &webcam) const;
        Q_INVOKABLE QbPacket readFrame() const;

    signals:
        void error(const QString &message);
        void webcamsChanged(const QStringList &webcams) const;
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void controlsChanged(const QString &webcam, const QVariantMap &controls) const;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetIoMethod();
        void resetNBuffers();
        void reset(const QString &webcam="") const;
};

#endif // CAPTURE_H
