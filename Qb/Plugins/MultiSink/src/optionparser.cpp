/* Webcamoid, webcam capture application.
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

#include "optionparser.h"

OptionParser::OptionParser(QObject *parent): QObject(parent)
{
}

QString OptionParser::error() const
{
    return this->m_error;
}

QList<ParsedOption> OptionParser::parse(QString cmd, bool *ok)
{
    QList<ParsedOption> options;

    if (ok)
        *ok = false;

    for (int i = 0; i < cmd.length(); )
    {
        int j = cmd.indexOf(QRegExp("\\s+"), i);

        if (i == j)
            j = cmd.indexOf(QRegExp("[^\\s]+"), i);
        else
        {
            QString param = cmd.mid(i, j - i);

            if (param.startsWith("--"))
            {
                QString paramName = param.mid(2);

                bool optionOk;

                Option option = this->findOption(paramName, true, &optionOk);

                if (optionOk)
                {
                    ParsedOption::OptionType type;
                    QVariant value;

                    if (option.flags() & Option::OptionFlagsHasValue)
                    {
                        i = cmd.indexOf(QRegExp("[^\\s]+"), j);
                        j = cmd.indexOf(QRegExp("\\s+"), i);

                        QString valueString = cmd.mid(i, j - i);

                        if (QRegExp(option.valregex()).exactMatch(valueString))
                            value = this->convertValue(param, valueString);
                        else
                        {
                            this->m_error = QString("Invalid value: %1 for %2").arg(valueString).arg(param);

                            return options;
                        }

                        type = ParsedOption::OptionTypePair;
                    }
                    else
                        type = ParsedOption::OptionTypeSingle;

                    options << ParsedOption(paramName, value, type);
                }
                else
                {
                    if (ok)
                        *ok = false;

                    this->m_error = QString("Invalid option: %1").arg(param);

                    return options;
                }
            }
            else if (param.startsWith("-"))
            {
                QString paramName = param.mid(1);

                bool optionOk;

                Option option = this->findOption(paramName, false, &optionOk);

                if (optionOk)
                {
                    ParsedOption::OptionType type;
                    QVariant value;

                    if (option.flags() & Option::OptionFlagsHasValue)
                    {
                        i = cmd.indexOf(QRegExp("[^\\s]+"), j);
                        j = cmd.indexOf(QRegExp("\\s+"), i);

                        QString valueString = cmd.mid(i, j - i);

                        if (QRegExp(option.valregex()).exactMatch(valueString))
                            value = this->convertValue(param, valueString);
                        else
                        {
                            this->m_error = QString("Invalid value: %1 for %2").arg(valueString).arg(param);

                            return options;
                        }

                        type = ParsedOption::OptionTypePair;
                    }
                    else
                        type = ParsedOption::OptionTypeSingle;

                    options << ParsedOption(paramName, value, type);
                }
                else
                {
                    if (ok)
                        *ok = false;

                    this->m_error = QString("Invalid option: %1").arg(param);

                    return options;
                }
            }
            else
                options << ParsedOption("", param, ParsedOption::OptionTypeData);
        }

        if (j < 0)
            break;
        else
            i = j;
    }

    if (ok)
        *ok = true;

    return options;
}

Option OptionParser::findOption(QString option, bool isLong, bool *ok)
{
    if (ok)
        *ok = true;

    foreach (Option opt, this->m_options)
        if (opt.name() == option &&
            (opt.flags() & Option::OptionFlagsIsLong) == isLong)
            return opt;

    if (ok)
        *ok = false;

    return Option();
}

QVariant OptionParser::convertValue(QString key, QString value)
{
    Q_UNUSED(key)

    return value;
}

void OptionParser::addOption(Option option)
{
    this->m_options << option;
}
