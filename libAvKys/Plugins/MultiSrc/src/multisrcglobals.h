/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef MULTISRCGLOBALS_H
#define MULTISRCGLOBALS_H

#include <QObject>

class MultiSrcGlobals: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString codecLib
               READ codecLib
               WRITE setCodecLib
               RESET resetCodecLib
               NOTIFY codecLibChanged)

    public:
        explicit MultiSrcGlobals(QObject *parent=NULL);

        Q_INVOKABLE QString codecLib() const;

    private:
        QString m_codecLib;
        QStringList m_preferredFramework;

    signals:
        void codecLibChanged(const QString &codecLib);

    public slots:
        void setCodecLib(const QString &codecLib);
        void resetCodecLib();
};

#endif // MULTISRCGLOBALS_H
