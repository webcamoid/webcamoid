/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#ifndef WEBCAMCONFIGELEMENT_H
#define WEBCAMCONFIGELEMENT_H

#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <qb.h>

class WebcamConfigElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList webcams READ webcams NOTIFY webcamsChanged)

    public:
        explicit WebcamConfigElement();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList availableSizes(const QString &webcam) const;
        Q_INVOKABLE QSize size(const QString &webcam) const;
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size) const;
        Q_INVOKABLE bool resetSize(const QString &webcam) const;
        Q_INVOKABLE QVariantList controls(const QString &webcam) const;
        Q_INVOKABLE bool setControls(const QString &webcam, const QVariantMap &controls) const;
        Q_INVOKABLE bool resetControls(const QString &webcam) const;

    private:
        QStringList m_webcams;
        QMap<v4l2_ctrl_type, QString> m_ctrlTypeToString;
        QFileSystemWatcher *m_fsWatcher;

        __u32 format(const QString &webcam, const QSize &size) const;
        QVariantList queryControl(int handle, v4l2_queryctrl *queryctrl) const;
        QMap<QString, uint> findControls(int handle) const;

    signals:
        void webcamsChanged(const QStringList &webcams) const;
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void controlsChanged(const QString &webcam, const QVariantMap &controls) const;

    public slots:
        void reset(const QString &webcam) const;
        void reset() const;

    private slots:
        void onDirectoryChanged(const QString &path);
};

#endif // WEBCAMCONFIGELEMENT_H
