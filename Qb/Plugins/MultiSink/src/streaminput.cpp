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

#include "streaminput.h"

StreamInput::StreamInput(QObject *parent): QObject(parent)
{
    this->resetType();
    this->resetFrom();
    this->resetTo();
}

StreamInput::StreamInput(QString type, int from, int to):
    QObject(NULL),
    m_type(type),
    m_from(from),
    m_to(to)
{
}

StreamInput::StreamInput(QString description)
{
    QStringList parts = description.split(":", QString::SkipEmptyParts);

    this->setType(parts[0]);
    this->setFrom(parts[1].toInt());

    if (parts.length() > 2)
        this->setTo(parts[2].toInt());
    else
        this->setTo(this->from());
}

StreamInput::StreamInput(const StreamInput &other):
    QObject(other.parent()),
    m_type(other.m_type),
    m_from(other.m_from),
    m_to(other.m_to)
{
}

StreamInput &StreamInput::operator =(const StreamInput &other)
{
    if (this != &other)
    {
        this->m_type = other.m_type;
        this->m_from = other.m_from;
        this->m_to = other.m_to;
    }

    return *this;
}

QString StreamInput::type() const
{
    return this->m_type;
}

int StreamInput::from() const
{
    return this->m_from;
}

int StreamInput::to() const
{
    return this->m_to;
}

QString StreamInput::toString() const
{
    return QString("%1:%2:%3").arg(this->type())
                              .arg(this->from())
            .arg(this->to());
}

bool StreamInput::operator <(const StreamInput &other) const
{
    return this->toString() < other.toString();
}

void StreamInput::setType(QString type)
{
    this->m_type = type;
}

void StreamInput::setFrom(int from)
{
    this->m_from = from;
}

void StreamInput::setTo(int to)
{
    this->m_to = to;
}

void StreamInput::resetType()
{
    this->setType("");
}

void StreamInput::resetFrom()
{
    this->setFrom(0);
}

void StreamInput::resetTo()
{
    this->setTo(0);
}

QDebug operator <<(QDebug debug, const StreamInput &input)
{
    debug.nospace() << QString("StreamInput(%1)").arg(input.toString());

    return debug.space();
}
