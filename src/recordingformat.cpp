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

#include "recordingformat.h"

RecordingFormat::RecordingFormat(const QString &description,
                                 const QStringList &suffix,
                                 const QString &params,
                                 QObject *parent):
    QObject(parent),
    m_description(description),
    m_suffix(suffix),
    m_params(params)
{
}

RecordingFormat::RecordingFormat(const RecordingFormat &other):
    QObject(other.parent()),
    m_description(other.m_description),
    m_suffix(other.m_suffix),
    m_params(other.m_params)
{

}

RecordingFormat &RecordingFormat::operator =(const RecordingFormat &other)
{
    if (this != &other) {
        this->m_description = other.m_description;
        this->m_suffix = other.m_suffix;
        this->m_params = other.m_params;
    }

    return *this;
}

bool RecordingFormat::operator ==(const RecordingFormat &other) const
{
    if (this->m_description == other.m_description
        && this->m_suffix == other.m_suffix
        && this->m_params == other.m_params)
        return true;

    return false;
}
QString RecordingFormat::description() const
{
    return this->m_description;
}

QStringList RecordingFormat::suffix() const
{
    return this->m_suffix;
}

QString RecordingFormat::params() const
{
    return this->m_params;
}

void RecordingFormat::setDescription(const QString &description)
{
    this->m_description = description;
}

void RecordingFormat::setSuffix(const QStringList &suffix)
{
    this->m_suffix = suffix;
}

void RecordingFormat::setParams(const QString &params)
{
    this->m_params = params;
}
