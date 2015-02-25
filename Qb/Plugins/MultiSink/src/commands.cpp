/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <qb.h>

#include "commands.h"

Commands::Commands(QObject *parent): OptionParser(parent)
{

    // Stream specifiers:
    this->addOption(Option("i",
                          "Input stream.",
                          "\\d+",
                          Option::OptionFlagsHasValue));

    this->addOption(Option("o",
                           "Output stream."));

    // Output options:
    this->addOption(Option("f",
                           "Output format.",
                           "[0-9a-z_]+",
                           Option::OptionFlagsHasValue));

    // Video options:
    this->addOption(Option("r",
                           "Video frame rate.",
                           "\\d+(/\\d+)?",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("s",
                           "Video size.",
                           "\\d+x\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("g",
                           "Distance of keyframes to seek.",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("c:v",
                           "Video codec.",
                           "[0-9a-z_]+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("b:v",
                           "Video bitrate.",
                           "\\d+(\\.\\d+)?(k|M|G|T)?",
                           Option::OptionFlagsHasValue));

    // Audio options:
    this->addOption(Option("ar",
                           "Audio sampling rate",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("ac",
                           "Number of audio channels.",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("c:a",
                           "Audio codec.",
                           "[0-9a-z_]+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("b:a",
                           "Audio bitrate.",
                           "\\d+(\\.\\d+)?(k|M|G|T)?",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("channel_layout",
                           "Audio channel layout.",
                           "([a-z]+|[2-7]\\.[0-1])(\\([a-z]+(-[a-z]+)?\\))?",
                           Option::OptionFlagsHasValue));

    // Common input options:
    this->addOption(Option("oi",
                           "Output index.",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("opt",
                           "Codec options.",
                           "[^=,\\s]+=[^=,\\s]+(,[^=,\\s]+=[^=,\\s]+)*",
                           Option::OptionFlagsHasValue));
}

QVariantMap Commands::inputs() const
{
    return this->m_inputs;
}

QVariantMap Commands::outputOptions() const
{
    return this->m_outputOptions;
}

bool Commands::parseCmd(QString cmd)
{
    bool ok;

    QList<ParsedOption> options = this->parse(cmd, &ok);

    if (!ok)
        return false;

    this->clear();

    QString curStream;
    QString curInput;
    QVariantMap curOptions;
    bool hasOutputOptions = false;

    foreach (ParsedOption option, options)
        if (option.key() == "i")
        {
            QString input = option.value().toString();

            if (curStream.isEmpty())
            {
                curStream = "i";
                curInput = input;
            }
            else if (curStream == "i")
            {
                this->m_inputs[curInput] = curOptions;
                curInput = input;
                curOptions.clear();
            }
            else if (curStream == "o")
            {
                curStream = "i";
                curInput = input;
                curOptions.clear();
            }
        }
        else if (option.key() == "o")
        {
            if (hasOutputOptions)
            {
                this->m_error = QString("Only one output is allowed.");

                return false;
            }
            else if (curStream.isEmpty())
            {
                curStream = "o";
                hasOutputOptions = true;
            }
            else if (curStream == "i")
            {
                this->m_inputs[curInput] = curOptions;
                curOptions.clear();

                curStream = "o";
                hasOutputOptions = true;
            }
        }
        else
        {
            if (option.type() == ParsedOption::OptionTypeNone ||
                option.type() == ParsedOption::OptionTypeData)
            {
                this->m_error = QString("Invalid parameter: %1.").arg(option.value().toString());

                return false;
            }
            else if (curStream.isEmpty())
            {
                this->m_error = QString("Option without a stream specifier: -%1.").arg(option.key());

                return false;
            }
            else if (curStream == "i")
            {
                QStringList validOptions;

                validOptions << "ar"
                             << "ac"
                             << "c:a"
                             << "b:a"
                             << "channel_layout"
                             << "r"
                             << "s"
                             << "g"
                             << "c:v"
                             << "b:v"
                             << "oi"
                             << "opt";

                if (!validOptions.contains(option.key()))
                {
                    this->m_error = QString("Invalid input option in %1: %2.").arg(curInput).arg(option.key());

                    return false;
                }

                curOptions[option.key()] = option.value();
            }
            else if (curStream == "o")
            {
                QStringList validOptions;

                validOptions << "f";

                if (!validOptions.contains(option.key()))
                {
                    this->m_error = QString("Invalid output option: %1.").arg(option.key());

                    return false;
                }

                this->m_outputOptions[option.key()] = option.value();
            }
        }

    if (!this->m_outputOptions.contains("f"))
    {
        this->m_error = QString("Output stream without format specifier (-f).");

        return false;
    }

    return true;
}

QVariant Commands::convertValue(QString key, QString value)
{
    if (key == "-i"
        || key == "-g"
        || key == "-ar"
        || key == "-ac"
        || key == "-oi")
        return value.toInt();
    else if (key == "-r") {
        QbFrac frac = value.contains("/")?
                          QbFrac(value):
                          QbFrac(value.toInt(), 1);

        return QVariant::fromValue(frac);
    }
    else if (key == "-s") {
        QStringList size = value.split("x", QString::SkipEmptyParts);

        return QSize(size[0].toInt(), size[1].toInt());
    }
    else if (key == "-b:v"
             || key == "-b:a") {
        quint64 multiplier;

        if (value.contains("k"))
            multiplier = 1e3;
        else if (value.contains("M"))
            multiplier = 1e6;
        else if (value.contains("G"))
            multiplier = 1e9;
        else if (value.contains("T"))
            multiplier = 1e12;
        else
            multiplier = 1;

        quint64 val = value.remove(QRegExp("\\D")).toInt();

        return val * multiplier;
    }
    else if (key == "-opt") {
        QVariantMap options;

        foreach (QString pair, value.split(",")) {
            QStringList p = pair.split("=");
            options[p[0]] = p[1];
        }

        return options;
    }

    return value;
}

void Commands::clear()
{
    this->m_inputs.clear();
    this->m_outputOptions.clear();
}
