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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *      By downloading, copying, installing or using the software you agree to this license.
 *      If you do not agree to this license, do not download, install,
 *      copy or use the software.
 *
 *
 *                            Intel License Agreement
 *                    For Open Source Computer Vision Library
 *
 *     Copyright (C) 2000, Intel Corporation, all rights reserved.
 *     Third party copyrights are property of their respective owners.
 *
 *     Redistribution and use in source and binary forms, with or without modification,
 *     are permitted provided that the following conditions are met:
 *
 *       * Redistribution's of source code must retain the above copyright notice,
 *         this list of conditions and the following disclaimer.
 *
 *       * Redistribution's in binary form must reproduce the above copyright notice,
 *         this list of conditions and the following disclaimer in the documentation
 *         and/or other materials provided with the distribution.
 *
 *       * The name of Intel Corporation may not be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *     This software is provided by the copyright holders and contributors "as is" and
 *     any express or implied warranties, including, but not limited to, the implied
 *     warranties of merchantability and fitness for a particular purpose are disclaimed.
 *     In no event shall the Intel Corporation or contributors be liable for any direct,
 *     indirect, incidental, special, exemplary, or consequential damages
 *     (including, but not limited to, procurement of substitute goods or services;
 *     loss of use, data, or profits; or business interruption) however caused
 *     and on any theory of liability, whether in contract, strict liability,
 *     or tort (including negligence or otherwise) arising in any way out of
 *     the use of this software, even if advised of the possibility of such damage.
 */

#include "haarstage.h"

HaarStageHID::HaarStageHID(const HaarStage &stage,
                           int oWidth,
                           const quint32 *integral,
                           const quint32 *tiltedIntegral,
                           qreal invArea,
                           qreal scale)
{
    this->m_count = stage.m_trees.size();
    this->m_trees = new HaarTreeHID *[this->m_count];
    static const qreal thresholdBias = 0.0001;
    this->m_threshold = stage.m_threshold - thresholdBias;

    this->m_parentStagePtr = NULL;
    this->m_nextStagePtr = NULL;
    this->m_childStagePtr = NULL;

    for (int i = 0; i < this->m_count; i++)
        this->m_trees[i] = new HaarTreeHID(stage.m_trees[i],
                                              oWidth,
                                              integral,
                                              tiltedIntegral,
                                              invArea,
                                              scale);
}

HaarStageHID::~HaarStageHID()
{
    for (int i = 0; i < this->m_count; i++)
        delete this->m_trees[i];

    delete [] this->m_trees;
}

HaarStage::HaarStage(QObject *parent): QObject(parent)
{
    this->m_threshold = 0;
    this->m_parentStage = -1;
    this->m_nextStage = -1;
    this->m_childStage = -1;
}

HaarStage::HaarStage(const HaarStage &other):
    QObject(NULL)
{
    this->m_trees = other.m_trees;
    this->m_threshold = other.m_threshold;
    this->m_parentStage = other.m_parentStage;
    this->m_nextStage = other.m_nextStage;
    this->m_childStage = other.m_childStage;
}

HaarStage::~HaarStage()
{
}

HaarTreeVector HaarStage::trees() const
{
    return this->m_trees;
}

HaarTreeVector &HaarStage::trees()
{
    return this->m_trees;
}

qreal HaarStage::threshold() const
{
    return this->m_threshold;
}

qreal &HaarStage::threshold()
{
    return this->m_threshold;
}

int HaarStage::parentStage() const
{
    return this->m_parentStage;
}

int &HaarStage::parentStage()
{
    return this->m_parentStage;
}

int HaarStage::nextStage() const
{
    return this->m_nextStage;
}

int &HaarStage::nextStage()
{
    return this->m_nextStage;
}

int HaarStage::childStage() const
{
    return this->m_childStage;
}

int &HaarStage::childStage()
{
    return this->m_childStage;
}

HaarStage &HaarStage::operator =(const HaarStage &other)
{
    if (this != &other) {
        this->m_trees = other.m_trees;
        this->m_threshold = other.m_threshold;
        this->m_parentStage = other.m_parentStage;
        this->m_nextStage = other.m_nextStage;
        this->m_childStage = other.m_childStage;
    }

    return *this;
}

bool HaarStage::operator ==(const HaarStage &other) const
{
    if (this->m_trees == other.m_trees
        && qFuzzyCompare(this->m_threshold, other.m_threshold)
        && this->m_parentStage == other.m_parentStage
        && this->m_nextStage == other.m_nextStage
        && this->m_childStage == other.m_childStage)
        return true;

    return false;
}

bool HaarStage::operator !=(const HaarStage &other) const
{
    return !(*this == other);
}

void HaarStage::setTrees(const HaarTreeVector &trees)
{
    if (this->m_trees == trees)
        return;

    this->m_trees = trees;
    emit this->treesChanged(trees);
}

void HaarStage::setThreshold(qreal threshold)
{
    if (qFuzzyCompare(this->m_threshold, threshold))
        return;

    this->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void HaarStage::setParentStage(int parentStage)
{
    if (this->m_parentStage == parentStage)
        return;

    this->m_parentStage = parentStage;
    emit this->parentStageChanged(parentStage);
}

void HaarStage::setNextStage(int nextStage)
{
    if (this->m_nextStage == nextStage)
        return;

    this->m_nextStage = nextStage;
    emit this->nextStageChanged(nextStage);
}

void HaarStage::setChildStage(int childStage)
{
    if (this->m_childStage == childStage)
        return;

    this->m_childStage = childStage;
    emit this->childStageChanged(childStage);
}

void HaarStage::resetTrees()
{
    this->setTrees(HaarTreeVector());
}

void HaarStage::resetThreshold()
{
    this->setThreshold(0);
}

void HaarStage::resetParentStage()
{
    this->setParentStage(-1);
}

void HaarStage::resetNextStage()
{
    this->setNextStage(-1);
}

void HaarStage::resetChildStage()
{
    this->setChildStage(-1);
}
