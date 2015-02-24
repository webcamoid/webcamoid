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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "option.h"

Option::Option(QObject *parent): QObject(parent)
{
    this->resetName();
    this->resetComment();
    this->resetValregex();
    this->resetFlags();
}

Option::Option(QString name,
               QString comment,
               QString valregex,
               OptionFlags flags):
    QObject(NULL),
    m_name(name),
    m_comment(comment),
    m_valregex(valregex),
    m_flags(flags)
{
}

Option::Option(const Option &other):
    QObject(NULL),
    m_name(other.m_name),
    m_comment(other.m_comment),
    m_valregex(other.m_valregex),
    m_flags(other.m_flags)
{
}

Option &Option::operator =(const Option &other)
{
    if (this != &other)
    {
        this->m_name = other.m_name;
        this->m_comment = other.m_comment;
        this->m_valregex = other.m_valregex;
        this->m_flags = other.m_flags;
    }

    return *this;
}

QString Option::name() const
{
    return this->m_name;
}

QString Option::comment() const
{
    return this->m_comment;
}

QString Option::valregex() const
{
    return this->m_valregex;
}

Option::OptionFlags Option::flags() const
{
    return this->m_flags;
}

void Option::setName(QString name)
{
    this->m_name = name;
}

void Option::setComment(QString comment)
{
    this->m_comment = comment;
}

void Option::setValregex(QString valregex)
{
    this->m_valregex = valregex;
}

void Option::setFlags(OptionFlags flags)
{
    this->m_flags = flags;
}

void Option::resetName()
{
    this->setName("");
}

void Option::resetComment()
{
    this->setComment("");
}

void Option::resetValregex()
{
    this->setValregex(".*");
}

void Option::resetFlags()
{
    this->setFlags(OptionFlagsNoFlags);
}

QDebug operator <<(QDebug debug, const Option &option)
{
    debug.nospace() << QString("Option(%1, %2, %3, %4)").arg(option.name())
                                                        .arg(option.comment())
                                                        .arg(option.valregex())
                                                        .arg(option.flags());

    return debug.space();
}
