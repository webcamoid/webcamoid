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

#include <QtMath>

#include <QFile>
#include <QXmlStreamReader>
#include <QStringList>

#include "haarcascade.h"

HaarCascadeHID::HaarCascadeHID(const HaarCascade &cascade,
                               int startX,
                               int endX,
                               int startY,
                               int endY,
                               int windowWidth,
                               int windowHeight,
                               int oWidth,
                               const quint32 *integral,
                               const quint32 *tiltedIntegral,
                               qreal step,
                               qreal invArea,
                               qreal scale,
                               bool cannyPruning,
                               const quint32 **p,
                               const quint64 **pq,
                               const quint32 **ip,
                               const quint32 **icp,
                               QList<QRect> *roi,
                               QMutex *mutex)
{
    this->m_count = cascade.m_stages.size();
    this->m_stages = new HaarStageHID *[this->m_count];

    this->m_startX = startX;
    this->m_endX = endX;
    this->m_startY = startY;
    this->m_endY = endY;
    this->m_windowWidth = windowWidth;
    this->m_windowHeight = windowHeight;
    this->m_oWidth = oWidth;
    this->m_step = step;
    this->m_invArea = invArea;
    this->m_isTree = cascade.m_isTree;
    this->m_cannyPruning = cannyPruning;
    this->m_roi = roi;
    this->m_mutex = mutex;

    for (int i = 0; i < 4; i++) {
        this->m_p[i] = p[i];
        this->m_pq[i] = pq[i];
        this->m_ip[i] = ip[i];
        this->m_icp[i] = icp[i];
    }

    for (int i = 0; i < this->m_count; i++)
        this->m_stages[i] = new HaarStageHID(cascade.m_stages[i],
                                             oWidth,
                                             integral,
                                             tiltedIntegral,
                                             invArea,
                                             scale);

    for (int i = 0; i < this->m_count; i++) {
        int parent = cascade.m_stages[i].parentStage();
        this->m_stages[i]->m_parentStagePtr = parent < 0? nullptr: this->m_stages[parent];
        int next = cascade.m_stages[i].nextStage();
        this->m_stages[i]->m_nextStagePtr = next < 0? nullptr: this->m_stages[next];
        int child = cascade.m_stages[i].childStage();
        this->m_stages[i]->m_childStagePtr = child < 0? nullptr: this->m_stages[child];
    }
}

HaarCascadeHID::~HaarCascadeHID()
{
    for (int i = 0; i < this->m_count; i++)
        delete this->m_stages[i];

    delete [] this->m_stages;
}

void HaarCascadeHID::run(HaarCascadeHID *cascade)
{
    for (int j = cascade->m_startY; j < cascade->m_endY; j++) {
        int y = qRound(j * cascade->m_step);
        int iStep = 1;

        for (int i = cascade->m_startX; i < cascade->m_endX; i += iStep) {
            int x = qRound(i * cascade->m_step);
            auto offset = size_t(x + y * cascade->m_oWidth);

            if (cascade->m_cannyPruning) {
                quint32 sum = cascade->m_ip[0][offset]
                            - cascade->m_ip[1][offset]
                            - cascade->m_ip[2][offset]
                            + cascade->m_ip[3][offset];

                quint32 sumCanny = cascade->m_icp[0][offset]
                                 - cascade->m_icp[1][offset]
                                 - cascade->m_icp[2][offset]
                                 + cascade->m_icp[3][offset];

                if (sum < 20 || sumCanny < 100) {
                    iStep = 2;

                    continue;
                }
            }

            quint32 sum = cascade->m_p[0][offset]
                        - cascade->m_p[1][offset]
                        - cascade->m_p[2][offset]
                        + cascade->m_p[3][offset];

            quint64 sum2 = cascade->m_pq[0][offset]
                         - cascade->m_pq[1][offset]
                         - cascade->m_pq[2][offset]
                         + cascade->m_pq[3][offset];

            qreal mean = sum * cascade->m_invArea;
            qreal varianceNormFactor = sum2 * cascade->m_invArea - mean * mean;
            varianceNormFactor = (varianceNormFactor >= 0.0)? sqrt(varianceNormFactor): 1.0;
            int stageResult = 1;

            if (cascade->m_isTree) {
                HaarStageHID *haarStage = cascade->m_stages[0];

                while (haarStage) {
                    if (haarStage->pass(offset, varianceNormFactor))
                        haarStage = haarStage->m_childStagePtr;
                    else {
                        while (haarStage && haarStage->m_nextStagePtr == nullptr)
                            haarStage = haarStage->m_parentStagePtr;

                        if (haarStage == nullptr) {
                            stageResult = 0;

                            break;
                        }

                        haarStage = haarStage->m_nextStagePtr;
                    }
                }
            } else for (int stage = 0; stage < cascade->m_count; stage++)
                if (!cascade->m_stages[stage]->pass(offset, varianceNormFactor)) {
                    stageResult = -stage;

                    break;
                }

            if (stageResult > 0) {
                cascade->m_mutex->lock();
                cascade->m_roi->append(QRect(x,
                                             y,
                                             cascade->m_windowWidth,
                                             cascade->m_windowHeight));
                cascade->m_mutex->unlock();
            }

            iStep = stageResult != 0? 1: 2;
        }
    }

    delete cascade;
}

HaarCascade::HaarCascade(QObject *parent):
    QObject(parent)
{
    this->m_isTree = false;
}

HaarCascade::HaarCascade(const HaarCascade &other):
    QObject(nullptr)
{
    this->m_name = other.m_name;
    this->m_windowSize = other.m_windowSize;
    this->m_stages = other.m_stages;
    this->m_errorString = other.m_errorString;
    this->m_isTree = other.m_isTree;
}

QString HaarCascade::name() const
{
    return this->m_name;
}

QString &HaarCascade::name()
{
    return this->m_name;
}

QSize HaarCascade::windowSize() const
{
    return this->m_windowSize;
}

QSize &HaarCascade::windowSize()
{
    return this->m_windowSize;
}

HaarStageVector HaarCascade::stages() const
{
    return this->m_stages;
}

HaarStageVector &HaarCascade::stages()
{
    return this->m_stages;
}

QString HaarCascade::errorString() const
{
    return this->m_errorString;
}

bool HaarCascade::load(const QString &fileName)
{
    this->resetName();
    this->resetWindowSize();
    this->resetStages();

    QFile haarFile(fileName);

    if (!haarFile.open(QIODevice::ReadOnly))
        return false;

    QXmlStreamReader haarReader(&haarFile);
    QStringList pathList;
    QString path;

    QList<QRect> featureRectList;
    QList<qreal> featureWeightList;
    int stage = 0;
    this->m_isTree = false;

    while (!haarReader.atEnd()) {
        auto token = haarReader.readNext();

        switch (token) {
        case QXmlStreamReader::Invalid: {
            if (this->m_errorString != haarReader.errorString()) {
                this->m_errorString = haarReader.errorString();
                emit this->errorStringChanged(haarReader.errorString());
            }

            return false;
        }

        case QXmlStreamReader::StartElement: {
            pathList << haarReader.name().toString();

            if (path.isEmpty() && haarReader.name() != QStringLiteral("opencv_storage"))
                return false;

            if (path == "opencv_storage")
                this->m_name = haarReader.name().toString();
            else if (path == QString("opencv_storage/%1/stages").arg(this->m_name)
                     && haarReader.name() == QStringLiteral("_")) {
                this->m_stages << HaarStage();
                this->m_stages.last().setParentStage(stage - 1);
            } else if (path == QString("opencv_storage/%1/stages/_/trees").arg(this->m_name)
                && haarReader.name() == QStringLiteral("_")) {
                this->m_stages.last().trees() << HaarTree();
            } else if (path == QString("opencv_storage/%1/stages/_/trees/_").arg(this->m_name)
                && haarReader.name() == QStringLiteral("_")) {
                this->m_stages.last().trees().last().features() << HaarFeature();
            }

            break;
        }

        case QXmlStreamReader::EndElement: {
            if (path == QString("opencv_storage/%1/stages/_").arg(this->m_name)) {
                int parent = this->m_stages.last().parentStage();

                if (parent != -1
                    && this->m_stages[parent].childStage() == -1)
                    this->m_stages[parent].setChildStage(stage);

                this->m_isTree |= this->m_stages.last().nextStage() != -1;
                stage++;
            } else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/feature/rects").arg(this->m_name)) {
                this->m_stages.last().trees().last().features().last().setRects(featureRectList.toVector());
                this->m_stages.last().trees().last().features().last().setWeight(featureWeightList.toVector());

                featureRectList.clear();
                featureWeightList.clear();
            }

            pathList.removeLast();

            break;
        }

        case QXmlStreamReader::Characters: {
            if (path == QString("opencv_storage/%1/size").arg(this->m_name)) {
                QStringList sizeStr = haarReader.text().toString().simplified().split(" ");
                QSize size(sizeStr[0].toInt(), sizeStr[1].toInt());

                if (!this->m_windowSize.isValid())
                    this->m_windowSize = size;
            } else if (path == QString("opencv_storage/%1/stages/_/stage_threshold").arg(this->m_name))
                this->m_stages.last().threshold() = haarReader.text().toDouble();
            else if (path == QString("opencv_storage/%1/stages/_/parent").arg(this->m_name))
                this->m_stages.last().parentStage() = haarReader.text().toInt();
            else if (path == QString("opencv_storage/%1/stages/_/next").arg(this->m_name))
                this->m_stages.last().nextStage() = haarReader.text().toInt();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/threshold").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().threshold() = haarReader.text().toDouble();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/left_node").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().leftNode() = haarReader.text().toInt();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/left_val").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().leftVal() = haarReader.text().toDouble();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/right_node").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().rightNode() = haarReader.text().toInt();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/right_val").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().rightVal() = haarReader.text().toDouble();
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/feature/rects/_").arg(this->m_name)) {
                QStringList sizeStr = haarReader.text().toString().simplified().split(" ");

                featureRectList << QRect(sizeStr[0].toInt(), sizeStr[1].toInt(),
                                         sizeStr[2].toInt(), sizeStr[3].toInt());
                featureWeightList << sizeStr[4].toDouble();
            }
            else if (path == QString("opencv_storage/%1/stages/_/trees/_/_/feature/tilted").arg(this->m_name))
                this->m_stages.last().trees().last().features().last().tilted() = haarReader.text().toInt();

            break;
        }

        default:
            break;
        }

        path = pathList.join("/");
    }

    return true;
}

HaarCascade &HaarCascade::operator =(const HaarCascade &other)
{
    if (this != &other) {
        this->m_name = other.m_name;
        this->m_windowSize = other.m_windowSize;
        this->m_stages = other.m_stages;
        this->m_errorString = other.m_errorString;
        this->m_isTree = other.m_isTree;
    }

    return *this;
}

bool HaarCascade::operator ==(const HaarCascade &other) const
{
    if (this->m_name == other.m_name
        && this->m_windowSize == other.m_windowSize
        && this->m_stages == other.m_stages
        && this->m_isTree == other.m_isTree)
        return true;

    return true;
}

bool HaarCascade::operator !=(const HaarCascade &other) const
{
    return !(*this == other);
}

void HaarCascade::setName(const QString &name)
{
    if (this->m_name == name)
        return;

    this->m_name = name;
    emit this->nameChanged(name);
}

void HaarCascade::setWindowSize(const QSize &windowSize)
{
    if (this->m_windowSize == windowSize)
        return;

    this->m_windowSize = windowSize;
    emit this->windowSizeChanged(windowSize);
}

void HaarCascade::setStages(const HaarStageVector &stages)
{
    if (this->m_stages == stages)
        return;

    this->m_stages = stages;
    emit this->stagesChanged(stages);
}

void HaarCascade::resetName()
{
    this->setName(QString());
}

void HaarCascade::resetWindowSize()
{
    this->setWindowSize(QSize());
}

void HaarCascade::resetStages()
{
    this->setStages(HaarStageVector());
}
