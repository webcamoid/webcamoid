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

#include "optionparser.h"

OptionParser::OptionParser(QObject *parent): QObject(parent)
{
}

void OptionParser::addOption(QString name, QString comment, Option::OptionFlags flags)
{
    this->m_options << Option(name, comment, flags);
}

QVariantMap OptionParser::parse(QString cmd)
{
    QStringList params = cmd.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    QVariantMap options;
    bool isOpt = true;
    QString curOpt;

    foreach (QString param, params)
        if (isOpt)
        {
            foreach (Option option, this->m_options)
            {
                QString optName = option.flags() & Option::OptionFlagsIsLong? "--": "-";
                optName.append(option.name());

                if (param == optName)
                {
                    if (option.flags() & Option::OptionFlagsHasValue)
                    {
                        curOpt = option.name();
                        isOpt = false;
                    }
                    else
                        options[option.name()] = QVariant();

                    break;
                }
            }
        }
        else
        {
            options[curOpt] = QVariant(param);
            isOpt = true;
        }

    return options;
}
