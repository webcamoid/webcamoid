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

class HaarStagePrivate
{
    public:
        HaarTreeVector m_trees;
        qreal m_threshold {0.0};
        int m_parentStage {-1};
        int m_nextStage {-1};
        int m_childStage {-1};
};

HaarStageHID::HaarStageHID(const HaarStage &stage,
                           int oWidth,
                           const quint32 *integral,
                           const quint32 *tiltedIntegral,
                           qreal invArea,
                           qreal scale)
{
    this->m_count = stage.d->m_trees.size();
    this->m_trees = new HaarTreeHID *[this->m_count];
    static const qreal thresholdBias = 0.0001;
    this->m_threshold = stage.d->m_threshold - thresholdBias;

    for (int i = 0; i < this->m_count; i++)
        this->m_trees[i] = new HaarTreeHID(stage.d->m_trees[i],
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

HaarStage::HaarStage(QObject *parent):
    QObject(parent)
{
    this->d = new HaarStagePrivate;
}

HaarStage::HaarStage(const HaarStage &other):
    QObject(nullptr)
{
    this->d = new HaarStagePrivate;
    this->d->m_trees = other.d->m_trees;
    this->d->m_threshold = other.d->m_threshold;
    this->d->m_parentStage = other.d->m_parentStage;
    this->d->m_nextStage = other.d->m_nextStage;
    this->d->m_childStage = other.d->m_childStage;
}

HaarStage::~HaarStage()
{
    delete this->d;
}

HaarTreeVector HaarStage::trees() const
{
    return this->d->m_trees;
}

HaarTreeVector &HaarStage::trees()
{
    return this->d->m_trees;
}

qreal HaarStage::threshold() const
{
    return this->d->m_threshold;
}

qreal &HaarStage::threshold()
{
    return this->d->m_threshold;
}

int HaarStage::parentStage() const
{
    return this->d->m_parentStage;
}

int &HaarStage::parentStage()
{
    return this->d->m_parentStage;
}

int HaarStage::nextStage() const
{
    return this->d->m_nextStage;
}

int &HaarStage::nextStage()
{
    return this->d->m_nextStage;
}

int HaarStage::childStage() const
{
    return this->d->m_childStage;
}

int &HaarStage::childStage()
{
    return this->d->m_childStage;
}

HaarStage &HaarStage::operator =(const HaarStage &other)
{
    if (this != &other) {
        this->d->m_trees = other.d->m_trees;
        this->d->m_threshold = other.d->m_threshold;
        this->d->m_parentStage = other.d->m_parentStage;
        this->d->m_nextStage = other.d->m_nextStage;
        this->d->m_childStage = other.d->m_childStage;
    }

    return *this;
}

bool HaarStage::operator ==(const HaarStage &other) const
{
    return this->d->m_trees == other.d->m_trees
           && qFuzzyCompare(this->d->m_threshold, other.d->m_threshold)
           && this->d->m_parentStage == other.d->m_parentStage
           && this->d->m_nextStage == other.d->m_nextStage
           && this->d->m_childStage == other.d->m_childStage;
}

bool HaarStage::operator !=(const HaarStage &other) const
{
    return !(*this == other);
}

void HaarStage::setTrees(const HaarTreeVector &trees)
{
    if (this->d->m_trees == trees)
        return;

    this->d->m_trees = trees;
    emit this->treesChanged(trees);
}

void HaarStage::setThreshold(qreal threshold)
{
    if (qFuzzyCompare(this->d->m_threshold, threshold))
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void HaarStage::setParentStage(int parentStage)
{
    if (this->d->m_parentStage == parentStage)
        return;

    this->d->m_parentStage = parentStage;
    emit this->parentStageChanged(parentStage);
}

void HaarStage::setNextStage(int nextStage)
{
    if (this->d->m_nextStage == nextStage)
        return;

    this->d->m_nextStage = nextStage;
    emit this->nextStageChanged(nextStage);
}

void HaarStage::setChildStage(int childStage)
{
    if (this->d->m_childStage == childStage)
        return;

    this->d->m_childStage = childStage;
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
