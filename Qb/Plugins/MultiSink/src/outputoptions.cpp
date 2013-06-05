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

#include "outputoptions.h"

OutputOptions::OutputOptions(QObject *parent): QObject(parent)
{
    this->resetCodecContext();
    this->resetCaps();
}

OutputOptions::OutputOptions(CodecContextPtr codecContext, QbCaps caps):
    QObject(NULL),
    m_codecContext(codecContext),
    m_caps(caps)
{
}

OutputOptions::OutputOptions(const OutputOptions &other):
    QObject(other.parent()),
    m_codecContext(other.m_codecContext),
    m_caps(other.m_caps)
{
}

OutputOptions &OutputOptions::operator =(const OutputOptions &other)
{
    if (this != &other)
    {
        this->m_codecContext = other.m_codecContext;
        this->m_caps = other.m_caps;
    }

    return *this;
}

CodecContextPtr OutputOptions::codecContext() const
{
    return this->m_codecContext;
}

QbCaps OutputOptions::caps() const
{
    return this->m_caps;
}

void OutputOptions::setCodecContext(CodecContextPtr codecContext)
{
    this->m_codecContext = codecContext;
}

void OutputOptions::setCaps(QbCaps caps)
{
    this->m_caps = caps;
}

void OutputOptions::resetCodecContext()
{
    this->m_codecContext.clear();
}

void OutputOptions::resetCaps()
{
    this->setCaps(QbCaps());
}
