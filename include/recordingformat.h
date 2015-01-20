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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef RECORDINGFORMAT_H
#define RECORDINGFORMAT_H

#include <QObject>
#include <QStringList>

class RecordingFormat: public QObject
{
    Q_OBJECT

    public:
        explicit RecordingFormat(const QString &description,
                                 const QStringList &suffix,
                                 const QString &params,
                                 QObject *parent=NULL);
        RecordingFormat(const RecordingFormat &other);
        RecordingFormat &operator =(const RecordingFormat &other);
        bool operator ==(const RecordingFormat &other) const;

        Q_INVOKABLE QString description() const;
        Q_INVOKABLE QStringList suffix() const;
        Q_INVOKABLE QString params() const;

    private:
        QString m_description;
        QStringList m_suffix;
        QString m_params;

    public slots:
        void setDescription(const QString &description);
        void setSuffix(const QStringList &suffix);
        void setParams(const QString &params);
};

#endif // RECORDINGFORMAT_H
