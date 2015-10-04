/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "parsedoption.h"

ParsedOption::ParsedOption(QObject *parent):
    QObject(parent)
{
    this->m_type = OptionTypeNone;
}

ParsedOption::ParsedOption(const QString &key,
                           const QVariant &value,
                           OptionType type):
    QObject(NULL),
    m_key(key),
    m_value(value),
    m_type(type)
{
}

ParsedOption::ParsedOption(const ParsedOption &other):
    QObject(NULL),
    m_key(other.m_key),
    m_value(other.m_value),
    m_type(other.m_type)
{
}

ParsedOption &ParsedOption::operator =(const ParsedOption &other)
{
    if (this != &other) {
        this->m_key = other.m_key;
        this->m_value = other.m_value;
        this->m_type = other.m_type;
    }

    return *this;
}

QString ParsedOption::key() const
{
    return this->m_key;
}

QVariant ParsedOption::value() const
{
    return this->m_value;
}

ParsedOption::OptionType ParsedOption::type() const
{
    return this->m_type;
}

void ParsedOption::setKey(const QString &key)
{
    if (this->m_key == key)
        return;

    this->m_key = key;
    emit this->keyChanged(key);
}

void ParsedOption::setValue(const QVariant &value)
{
    if (this->m_value == value)
        return;

    this->m_value = value;
    emit this->valueChanged(value);
}

void ParsedOption::setType(ParsedOption::OptionType type)
{
    if (this->m_type == type)
        return;

    this->m_type = type;
    emit this->typeChanged(type);
}

void ParsedOption::resetKey()
{
    this->setKey("");
}

void ParsedOption::resetValue()
{
    this->setValue(QVariant());
}

void ParsedOption::resetType()
{
    this->setType(OptionTypeNone);
}

QDebug operator <<(QDebug debug, const ParsedOption &option)
{
    debug.nospace() << "ParsedOption("
                    << option.key()
                    << ", "
                    << option.value()
                    << ", "
                    << option.type()
                    << ")";

    return debug.space();
}
