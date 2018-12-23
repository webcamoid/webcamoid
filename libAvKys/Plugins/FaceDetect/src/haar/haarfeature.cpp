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

#include "haarfeature.h"

HaarFeatureHID::HaarFeatureHID(const HaarFeature &feature,
                               int oWidth,
                               const quint32 *integral,
                               const quint32 *tiltedIntegral,
                               qreal invArea,
                               qreal scale)
{
    this->m_count = feature.m_count;
    this->m_tilted = feature.m_tilted;
    this->m_threshold = feature.m_threshold;
    this->m_leftNode = feature.m_leftNode;
    this->m_leftVal = feature.m_leftVal;
    this->m_rightNode = feature.m_rightNode;
    this->m_rightVal = feature.m_rightVal;

    qreal area0 = 0;
    qreal sum0 = 0;

    for (int i = 0; i < this->m_count; i++) {
        int rectX = qRound(scale * feature.m_rects[i].x());
        int rectY = qRound(scale * feature.m_rects[i].y());
        int rectWidth = qRound(scale * feature.m_rects[i].width());
        int rectHeight = qRound(scale * feature.m_rects[i].height());

        if (this->m_tilted) {
            this->m_p0[i] = const_cast<quint32 *>(tiltedIntegral)
                            +  rectX
                            +  rectY * oWidth;
            this->m_p1[i] = const_cast<quint32 *>(tiltedIntegral)
                            +  rectX - rectHeight
                            + (rectY + rectHeight) * oWidth;
            this->m_p2[i] = const_cast<quint32 *>(tiltedIntegral)
                            +  rectX + rectWidth
                            + (rectY + rectWidth) * oWidth;
            this->m_p3[i] = const_cast<quint32 *>(tiltedIntegral)
                            +  rectX + rectWidth - rectHeight
                            + (rectY + rectWidth + rectHeight) * oWidth;
        } else {
            this->m_p0[i] = const_cast<quint32 *>(integral)
                            + rectX
                            + rectY * oWidth;
            this->m_p1[i] = const_cast<quint32 *>(integral)
                            + rectX + rectWidth
                            + rectY * oWidth;
            this->m_p2[i] = const_cast<quint32 *>(integral)
                            +  rectX
                            + (rectY + rectHeight) * oWidth;
            this->m_p3[i] = const_cast<quint32 *>(integral)
                            +  rectX + rectWidth
                            + (rectY + rectHeight) * oWidth;
        }

        this->m_weight[i] = (this->m_tilted? 0.5: 1)
                               * feature.m_weight[i] * invArea;

        int rectArea = rectWidth * rectHeight;

        if (i == 0)
            area0 = rectArea;
        else
            sum0 += this->m_weight[i] * rectArea;
    }

    this->m_weight[0] = -sum0 / area0;
}

HaarFeature::HaarFeature(QObject *parent):
    QObject(parent)
{
    this->m_count = 0;
    this->m_tilted = false;
    this->m_threshold = 0;
    this->m_leftNode = -1;
    this->m_leftVal = Q_QNAN;
    this->m_rightNode = -1;
    this->m_rightVal = Q_QNAN;
}

HaarFeature::HaarFeature(const HaarFeature &other):
    QObject(nullptr)
{
    this->m_count = other.m_count;
    this->m_tilted = other.m_tilted;
    this->m_threshold = other.m_threshold;
    this->m_leftNode = other.m_leftNode;
    this->m_leftVal = other.m_leftVal;
    this->m_rightNode = other.m_rightNode;
    this->m_rightVal = other.m_rightVal;

    for (int i = 0; i < other.m_count; i++) {
        this->m_rects[i] = other.m_rects[i];
        this->m_weight[i] = other.m_weight[i];
    }
}

RectVector HaarFeature::rects() const
{
    RectVector rects(this->m_count);

    for (int i = 0; i < this->m_count; i++)
        rects[i] = this->m_rects[i];

    return rects;
}

RealVector HaarFeature::weight() const
{
    RealVector weight(this->m_count);

    for (int i = 0; i < this->m_count; i++)
        weight[i] = this->m_weight[i];

    return weight;
}

bool HaarFeature::tilted() const
{
    return this->m_tilted;
}

bool &HaarFeature::tilted()
{
    return this->m_tilted;
}

qreal HaarFeature::threshold() const
{
    return this->m_threshold;
}

qreal &HaarFeature::threshold()
{
    return this->m_threshold;
}

int HaarFeature::leftNode() const
{
    return this->m_leftNode;
}

int &HaarFeature::leftNode()
{
    return this->m_leftNode;
}

qreal HaarFeature::leftVal() const
{
    return this->m_leftVal;
}

qreal &HaarFeature::leftVal()
{
    return this->m_leftVal;
}

int HaarFeature::rightNode() const
{
    return this->m_rightNode;
}

int &HaarFeature::rightNode()
{
    return this->m_rightNode;
}

qreal HaarFeature::rightVal() const
{
    return this->m_rightVal;
}

qreal &HaarFeature::rightVal()
{
    return this->m_rightVal;
}

HaarFeature &HaarFeature::operator =(const HaarFeature &other)
{
    if (this != &other) {
        this->m_count = other.m_count;
        this->m_tilted = other.m_tilted;
        this->m_threshold = other.m_threshold;
        this->m_leftNode = other.m_leftNode;
        this->m_leftVal = other.m_leftVal;
        this->m_rightNode = other.m_rightNode;
        this->m_rightVal = other.m_rightVal;

        for (int i = 0; i < other.m_count; i++) {
            this->m_rects[i] = other.m_rects[i];
            this->m_weight[i] = other.m_weight[i];
        }
    }

    return *this;
}

bool HaarFeature::operator ==(const HaarFeature &other) const
{
    if (this->m_count == other.m_count
        && this->m_tilted == other.m_tilted
        && qFuzzyCompare(this->m_threshold, other.m_threshold)
        && this->m_leftNode == other.m_leftNode
        && qFuzzyCompare(this->m_leftVal, other.m_leftVal)
        && this->m_rightNode == other.m_rightNode
        && qFuzzyCompare(this->m_rightVal, other.m_rightVal)) {
        for (int i = 0; i < this->m_count; i++)
            if (this->m_rects[i] != other.m_rects[i]
                || !qFuzzyCompare(this->m_weight[i], other.m_weight[i])) {
                return false;
            }

        return true;
    }

    return true;
}

bool HaarFeature::operator !=(const HaarFeature &other) const
{
    return !(*this == other);
}

void HaarFeature::setRects(const RectVector &rects)
{
    if (this->m_count == rects.size()) {
        bool eq = true;

        for (int i = 0; i < rects.size(); i++)
            if (this->m_rects[i] != rects[i]) {
                eq = false;

                break;
            }

        if (eq)
            return;
    }

    this->m_count = rects.size();

    for (int i = 0; i < rects.size(); i++)
        this->m_rects[i] = rects[i];

    emit this->rectsChanged(rects);
}

void HaarFeature::setWeight(const RealVector &weight)
{
    if (this->m_count == weight.size()) {
        bool eq = true;

        for (int i = 0; i < weight.size(); i++)
            if (!qFuzzyCompare(this->m_weight[i], weight[i])) {
                eq = false;

                break;
            }

        if (eq)
            return;
    }

    this->m_count = weight.size();

    for (int i = 0; i < weight.size(); i++)
        this->m_weight[i] = weight[i];

    emit this->weightChanged(weight);
}

void HaarFeature::setTilted(bool tilted)
{
    if (this->m_tilted == tilted)
        return;

    this->m_tilted = tilted;
    emit this->tiltedChanged(tilted);
}

void HaarFeature::setThreshold(qreal threshold)
{
    if (qFuzzyCompare(this->m_threshold, threshold))
        return;

    this->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void HaarFeature::setLeftNode(int leftNode)
{
    if (this->m_leftNode == leftNode)
        return;

    this->m_leftNode = leftNode;
    emit this->leftNodeChanged(leftNode);
}

void HaarFeature::setLeftVal(qreal leftVal)
{
    if (qFuzzyCompare(this->m_leftVal, leftVal))
        return;

    this->m_leftVal = leftVal;
    emit this->leftValChanged(leftVal);
}

void HaarFeature::setRightNode(int rightNode)
{
    if (this->m_rightNode == rightNode)
        return;

    this->m_rightNode = rightNode;
    emit this->rightNodeChanged(rightNode);
}

void HaarFeature::setRightVal(qreal rightVal)
{
    if (qFuzzyCompare(this->m_rightVal, rightVal))
        return;

    this->m_rightVal = rightVal;
    emit this->rightValChanged(rightVal);
}

void HaarFeature::resetRects()
{
    this->setRects(RectVector());
}

void HaarFeature::resetWeight()
{
    this->setWeight(RealVector());
}

void HaarFeature::resetTilted()
{
    this->setTilted(false);
}

void HaarFeature::resetThreshold()
{
    this->setThreshold(0);
}

void HaarFeature::resetLeftNode()
{
    this->setLeftNode(-1);
}

void HaarFeature::resetLeftVal()
{
    this->setLeftVal(Q_QNAN);
}

void HaarFeature::resetRightNode()
{
    this->setRightNode(-1);
}

void HaarFeature::resetRightVal()
{
    this->setRightVal(Q_QNAN);
}
