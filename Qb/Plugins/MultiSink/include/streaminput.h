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

#ifndef STREAMINPUT_H
#define STREAMINPUT_H

#include <QtCore>

class StreamInput: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type WRITE setType RESET resetType)
    Q_PROPERTY(int from READ from WRITE setFrom RESET resetFrom)
    Q_PROPERTY(int to READ to WRITE setTo RESET resetTo)

    public:
        explicit StreamInput(QObject *parent=NULL);
        StreamInput(QString type, int from, int to);
        StreamInput(QString description);
        StreamInput(const StreamInput &other);

        StreamInput &operator =(const StreamInput &other);

        Q_INVOKABLE QString type() const;
        Q_INVOKABLE int from() const;
        Q_INVOKABLE int to() const;

        Q_INVOKABLE QString toString() const;

        bool operator <(const StreamInput& other) const;

    private:
        QString m_type;
        int m_from;
        int m_to;

        friend QDebug operator <<(QDebug debug, const StreamInput &input);

    public slots:
        void setType(QString type);
        void setFrom(int from);
        void setTo(int to);
        void resetType();
        void resetFrom();
        void resetTo();
};

QDebug operator <<(QDebug debug, const StreamInput &input);

Q_DECLARE_METATYPE(StreamInput)

#endif // STREAMINPUT_H
