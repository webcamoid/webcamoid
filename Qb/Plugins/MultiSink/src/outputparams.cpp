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

#include "outputparams.h"

OutputParams::OutputParams(QObject *parent): QObject(parent)
{
    this->resetCodecContext();
    this->resetFilter();
    this->resetOutputIndex();
    this->resetPts();
    this->m_prevPts = -1;
    this->m_lastPts = -1;
    this->m_ptsDrift = 0;
}

OutputParams::OutputParams(CodecContextPtr codecContext,
                           QbElementPtr filter,
                           int outputIndex,
                           qint64 pts):
    QObject(NULL),
    m_codecContext(codecContext),
    m_filter(filter),
    m_outputIndex(outputIndex),
    m_pts(pts),
    m_prevPts(-1),
    m_lastPts(-1),
    m_ptsDrift(0)
{
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent()),
    m_codecContext(other.m_codecContext),
    m_filter(other.m_filter),
    m_outputIndex(other.m_outputIndex),
    m_pts(other.m_pts),
    m_prevPts(other.m_prevPts),
    m_lastPts(other.m_lastPts),
    m_ptsDrift(other.m_ptsDrift)
{
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->m_codecContext = other.m_codecContext;
        this->m_filter = other.m_filter;
        this->m_outputIndex = other.m_outputIndex;
        this->m_pts = other.m_pts;
        this->m_prevPts = other.m_prevPts;
        this->m_lastPts = other.m_lastPts;
        this->m_ptsDrift = other.m_ptsDrift;
    }

    return *this;
}

CodecContextPtr OutputParams::codecContext() const
{
    return this->m_codecContext;
}

QbElementPtr OutputParams::filter() const
{
    return this->m_filter;
}

int OutputParams::outputIndex() const
{
    return this->m_outputIndex;
}

qint64 OutputParams::pts() const
{
    return this->m_pts;
}

void OutputParams::setCodecContext(CodecContextPtr codecContext)
{
    this->m_codecContext = codecContext;
}

void OutputParams::setFilter(QbElementPtr filter)
{
    this->m_filter = filter;
}

void OutputParams::setOutputIndex(int outputIndex)
{
    this->m_outputIndex = outputIndex;
}

bool OutputParams::setPts(qint64 pts)
{
    if (this->m_prevPts >= 0 && pts == this->m_prevPts)
        return false;

    if (this->m_lastPts < 0 && pts != 0)
        this->m_ptsDrift = -pts;

    this->m_pts = pts + this->m_ptsDrift;

    if (this->m_prevPts >= 0 && this->m_pts <= this->m_lastPts) {
        this->m_pts = this->m_lastPts + 1;
        this->m_ptsDrift = this->m_pts - pts;
    }

    this->m_prevPts = pts;
    this->m_lastPts = this->m_pts;

    return true;
}

void OutputParams::resetCodecContext()
{
    this->m_codecContext.clear();
}

void OutputParams::resetFilter()
{
    this->m_filter.clear();
}

void OutputParams::resetOutputIndex()
{
    this->setOutputIndex(0);
}

void OutputParams::resetPts()
{
    this->setPts(0);
}
