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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "outputparams.h"

OutputParams::OutputParams(int inputIndex, QObject *parent):
    QObject(parent),
    m_inputIndex(inputIndex)
{
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent()),
    m_inputIndex(other.m_inputIndex)
{
}

OutputParams::~OutputParams()
{
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->m_inputIndex = other.m_inputIndex;
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

void OutputParams::setInputIndex(int inputIndex)
{
    if (this->m_inputIndex == inputIndex)
        return;

    this->m_inputIndex = inputIndex;
    emit this->inputIndexChanged(inputIndex);
}

void OutputParams::resetInputIndex()
{
    this->setInputIndex(0);
}
