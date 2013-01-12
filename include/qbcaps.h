/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef QBCAPS_H
#define QBCAPS_H

// http://gstreamer.freedesktop.org/data/doc/gstreamer/head/pwg/html/section-types-definitions.html

#include <QtCore>

class QbCaps: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType RESET resetMimeType)

    public:
        explicit QbCaps(QObject *parent=NULL);
        QbCaps(QString capsString);
        QbCaps(const QbCaps &other);
        QbCaps &operator =(const QbCaps &other);
        bool operator ==(const QbCaps &other) const;
        bool operator !=(const QbCaps &other) const;

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE QString mimeType() const;
        Q_INVOKABLE QString toString() const;

    private:
        bool m_isValid;
        QString m_mimeType;

    public slots:
        void setMimeType(QString mimeType);
        void resetMimeType();
};

#endif // QBCAPS_H
