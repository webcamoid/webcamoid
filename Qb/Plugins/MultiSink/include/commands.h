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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "optionparser.h"
#include "streaminput.h"

typedef QMap<StreamInput, QVariantMap> StreamInputMap;

class Commands: public OptionParser
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap generalOptions READ generalOptions)
    Q_PROPERTY(StreamInputMap inputs READ inputs)
    Q_PROPERTY(QVariantMap outputOptions READ outputOptions)

    public:
        explicit Commands(QObject *parent=NULL);

        Q_INVOKABLE QVariantMap generalOptions() const;
        Q_INVOKABLE StreamInputMap inputs() const;
        Q_INVOKABLE QVariantMap outputOptions() const;

        Q_INVOKABLE bool parseCmd(QString cmd);

    private:
        QVariantMap m_generalOptions;
        QMap<StreamInput, QVariantMap> m_inputs;
        QVariantMap m_outputOptions;

        virtual QVariant convertValue(QString key, QString value);

    public slots:
        void clear();
};

#endif // COMMANDS_H
