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

#include "haartree.h"

HaarTreeHID::HaarTreeHID(const HaarTree &tree,
                         int oWidth,
                         const quint32 *integral,
                         const quint32 *tiltedIntegral,
                         qreal invArea,
                         qreal scale)
{
    this->m_count = tree.m_features.size();
    this->m_features = new HaarFeatureHID *[this->m_count];

    for (int i = 0; i < this->m_count; i++)
        this->m_features[i] = new HaarFeatureHID(tree.m_features[i],
                                                    oWidth,
                                                    integral,
                                                    tiltedIntegral,
                                                    invArea,
                                                    scale);
}

HaarTreeHID::~HaarTreeHID()
{
    for (int i = 0; i < this->m_count; i++)
        delete this->m_features[i];

    delete [] this->m_features;
}

HaarTree::HaarTree(QObject *parent): QObject(parent)
{
}

HaarTree::HaarTree(const HaarTree &other):
    QObject(nullptr)
{
    this->m_features = other.m_features;
}

HaarFeatureVector HaarTree::features() const
{
    return this->m_features;
}

HaarFeatureVector &HaarTree::features()
{
    return this->m_features;
}

HaarTree &HaarTree::operator =(const HaarTree &other)
{
    if (this != &other) {
        this->m_features = other.m_features;
    }

    return *this;
}

bool HaarTree::operator ==(const HaarTree &other) const
{
    return this->m_features == other.m_features;
}

bool HaarTree::operator !=(const HaarTree &other) const
{
    return !(*this == other);
}

void HaarTree::setFeatures(const HaarFeatureVector &features)
{
    if (this->m_features == features)
        return;

    this->m_features = features;
    emit this->featuresChanged(features);
}

void HaarTree::resetFeatures()
{
    this->setFeatures(HaarFeatureVector());
}
