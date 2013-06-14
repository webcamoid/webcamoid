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

#include <qbelement.h>

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
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size);
        Q_INVOKABLE bool resetSize(const QString &webcam);
        Q_INVOKABLE QVariantList controls(const QString &webcam) const;

    private:

    signals:
        void webcamsChanged(const QStringList &webcams);
        void sizeChanged(const QString &webcam, const QSize &size);
        void controlsChanged(const QString &webcam, const QVariantMap &controls);

    public slots:
        void setControls(const QString &webcam, const QVariantMap &controls);
        void resetControls(const QString &webcam);
        void reset(const QString &webcam);
        void reset();
};

#endif // WEBCAMCONFIGELEMENT_H
