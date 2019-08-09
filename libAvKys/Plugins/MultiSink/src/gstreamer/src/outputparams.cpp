/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

class OutputParamsPrivate
{
    public:
        int m_inputIndex {-1};
        quint64 m_nFrame {0};
        qint64 m_id {-1};
        qint64 m_pts {0};
        qint64 m_ptsDiff {0};
        qint64 m_ptsDrift {0};
};

OutputParams::OutputParams(int inputIndex, QObject *parent):
    QObject(parent)
{
    this->d = new OutputParamsPrivate;
    this->d->m_inputIndex = inputIndex;
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent())
{
    this->d = new OutputParamsPrivate;
    this->d->m_inputIndex = other.d->m_inputIndex;
    this->d->m_nFrame = other.d->m_nFrame;
    this->d->m_id = other.d->m_id;
    this->d->m_pts = other.d->m_pts;
    this->d->m_ptsDiff = other.d->m_ptsDiff;
    this->d->m_ptsDrift = other.d->m_ptsDrift;
}

OutputParams::~OutputParams()
{
    delete this->d;
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->d->m_inputIndex = other.d->m_inputIndex;
        this->d->m_nFrame = other.d->m_nFrame;
        this->d->m_id = other.d->m_id;
        this->d->m_pts = other.d->m_pts;
        this->d->m_ptsDiff = other.d->m_ptsDiff;
        this->d->m_ptsDrift = other.d->m_ptsDrift;
    }

    return *this;
}

int OutputParams::inputIndex() const
{
    return this->d->m_inputIndex;
}

int &OutputParams::inputIndex()
{
    return this->d->m_inputIndex;
}

quint64 OutputParams::nFrame() const
{
    return this->d->m_nFrame;
}

quint64 &OutputParams::nFrame()
{
    return this->d->m_nFrame;
}

qint64 OutputParams::nextPts(qint64 pts, qint64 id)
{
    if (this->d->m_pts < 0 || this->d->m_id < 0) {
        this->d->m_ptsDrift = -pts;
        this->d->m_pts = pts;
        this->d->m_id = id;

        return 0;
    }

    if (pts <= this->d->m_pts || id != this->d->m_id) {
        this->d->m_ptsDrift += this->d->m_pts - pts + this->d->m_ptsDiff;
        this->d->m_pts = pts;
        this->d->m_id = id;

        return pts + this->d->m_ptsDrift;
    }

    this->d->m_ptsDiff = pts - this->d->m_pts;
    this->d->m_pts = pts;

    return pts + this->d->m_ptsDrift;
}

void OutputParams::setInputIndex(int inputIndex)
{
    if (this->d->m_inputIndex == inputIndex)
        return;

    this->d->m_inputIndex = inputIndex;
    emit this->inputIndexChanged(inputIndex);
}

void OutputParams::setNFrame(quint64 nFrame)
{
    if (this->d->m_nFrame == nFrame)
        return;

    this->d->m_nFrame = nFrame;
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

#include "moc_outputparams.cpp"
