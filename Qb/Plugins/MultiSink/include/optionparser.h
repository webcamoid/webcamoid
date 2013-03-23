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

#ifndef OPTIONPARSER_H
#define OPTIONPARSER_H

#include "option.h"

class OptionParser: public QObject
{
    Q_OBJECT

    public:
        explicit OptionParser(QObject *parent=NULL);

        Q_INVOKABLE void addOption(QString name, Option::OptionFlags flags=Option::OptionFlagsNoFlags);
        Q_INVOKABLE QVariantMap parse(QString cmd);

    private:
        QList<Option> m_options;
};

#endif // OPTIONPARSER_H
