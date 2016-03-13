/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Web-Site: http://webcamoid.github.io/
 */

#include "outputparams.h"

OutputParams::OutputParams(int inputIndex, QObject *parent):
    QObject(parent),
    m_inputIndex(inputIndex),
    m_nFrame(0),
    m_id(-1),
    m_pts(0),
    m_ptsDiff(0),
    m_ptsDrift(0)
{
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent()),
    m_inputIndex(other.m_inputIndex),
    m_nFrame(other.m_nFrame),
    m_id(other.m_id),
    m_pts(other.m_pts),
    m_ptsDiff(other.m_ptsDiff),
    m_ptsDrift(other.m_ptsDrift)
{
}

OutputParams::~OutputParams()
{
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->m_inputIndex = other.m_inputIndex;
        this->m_nFrame = other.m_nFrame;
        this->m_id = other.m_id;
        this->m_pts = other.m_pts;
        this->m_ptsDiff = other.m_ptsDiff;
        this->m_ptsDrift = other.m_ptsDrift;
    }

    return *this;
}

int OutputParams::inputIndex() const
{
    return this->m_inputIndex;
}

int &OutputParams::inputIndex()
{
    return this->m_inputIndex;
}

quint64 OutputParams::nFrame() const
{
    return this->m_nFrame;
}

quint64 &OutputParams::nFrame()
{
    return this->m_nFrame;
}

qint64 OutputParams::nextPts(qint64 pts, qint64 id)
{
    if (this->m_pts < 0 || this->m_id < 0) {
        this->m_ptsDrift = -pts;
        this->m_pts = pts;
        this->m_id = id;

        return 0;
    }

    if (pts <= this->m_pts || id != this->m_id) {
        this->m_ptsDrift += this->m_pts - pts + this->m_ptsDiff;
        this->m_pts = pts;
        this->m_id = id;

        return pts + this->m_ptsDrift;
    }

    this->m_ptsDiff = pts - this->m_pts;
    this->m_pts = pts;

    return pts + this->m_ptsDrift;
}

void OutputParams::setInputIndex(int inputIndex)
{
    if (this->m_inputIndex == inputIndex)
        return;

    this->m_inputIndex = inputIndex;
    emit this->inputIndexChanged(inputIndex);
}

void OutputParams::setNFrame(quint64 nFrame)
{
    if (this->m_nFrame == nFrame)
        return;

    this->m_nFrame = nFrame;
    emit this->nFrameChanged(nFrame);
}

void OutputParams::resetInputIndex()
{
    this->setInputIndex(0);
}

void OutputParams::resetNFrame()
{
    this->setNFrame(0);
}
