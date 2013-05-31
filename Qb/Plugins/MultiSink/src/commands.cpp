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

#include <qb.h>

#include "commands.h"

Commands::Commands(QObject *parent): OptionParser(parent)
{

    // Stream specifiers:
    this->addOption(Option("i",
                          "Input stream.",
                          "[a-z]:\\d+(:\\d+)?",
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

    this->addOption(Option("vn",
                           "Disable video record."));

    this->addOption(Option("vcodec",
                           "Video codec.",
                           "[0-9a-z_]+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("b:v",
                           "Video bitrate.",
                           "\\d+(k|M|G|T)?",
                           Option::OptionFlagsHasValue));

    // Audio options:
    this->addOption(Option("ar",
                           "audio sampling rate",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("ac",
                           "Number of audio channels.",
                           "\\d+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("an",
                           "Disable audio record."));

    this->addOption(Option("acodec",
                           "Audio codec.",
                           "[0-9a-z_]+",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("b:a",
                           "Audio bitrate.",
                           "\\d+(k|M|G|T)?",
                           Option::OptionFlagsHasValue));

    this->addOption(Option("channel_layout",
                           "Audio channel layout.",
                           "([a-z]+|[2-7]\\.[0-1])(\\([a-z]+(-[a-z]+)?\\))?",
                           Option::OptionFlagsHasValue));
}

QVariantMap Commands::generalOptions() const
{
    return this->m_generalOptions;
}

StreamInputMap Commands::inputs() const
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
    StreamInput curInput;
    QVariantMap curOptions;
    bool hasOutputOptions = false;

    foreach (ParsedOption option, options)
        if (option.key() == "i")
        {
            StreamInput input(option.value().toString());
            QStringList validInputTypes;

            validInputTypes << "a" << "v";

            if (!validInputTypes.contains(input.type()))
            {
                this->m_error = QString("Invalid input type: %1.").arg(input.type());

                return false;
            }

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
        else if (option.key() == "an" || option.key() == "vn")
        {
            if (curStream == "i")
            {
                this->m_inputs[curInput] = curOptions;
                curOptions.clear();
            }

            curStream = "";
            this->m_generalOptions[option.key()] = QVariant();
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
                QString type;

                if (curInput.type() == "a")
                {
                    validOptions << "ar"
                                 << "ac"
                                 << "acodec"
                                 << "b:a"
                                 << "channel_layout";

                    type = "audio";
                }
                else if (curInput.type() == "v")
                {
                    validOptions << "r"
                                 << "s"
                                 << "vcodec"
                                 << "b:v";

                    type = "video";
                }

                if (!validOptions.contains(option.key()))
                {
                    this->m_error = QString("Invalid %1 input option: %2.").arg(type).arg(option.key());

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
    if (key == "i")
        return QVariant::fromValue(StreamInput(value));
    else if (key == "r")
    {
        QbFrac frac = value.contains("/")?
                          QbFrac(value):
                          QbFrac(value.toInt(), 1);

        return QVariant::fromValue(frac);
    }
    else if (key == "s")
    {
        QStringList size = value.split("x", QString::SkipEmptyParts);

        return QSize(size[0].toInt(), size[1].toInt());
    }
    else if (key == "b:v" || key == "b:a")
    {
        bool ok;

        int val = value.toInt(&ok);

        if (ok)
            return val;
        else
            return value.replace("k", QString("%1").arg(1e3))
                        .replace("M", QString("%1").arg(1e6))
                        .replace("G", QString("%1").arg(1e9))
                        .replace("T", QString("%1").arg(1e12)).toInt();
    }
    else if (key == "ar" ||  key == "ac")
        return value.toInt();

    return value;
}

void Commands::clear()
{
    this->m_generalOptions.clear();
    this->m_inputs.clear();
    this->m_outputOptions.clear();
}
