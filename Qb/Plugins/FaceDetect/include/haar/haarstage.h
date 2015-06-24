/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef HAARSTAGE_H
#define HAARSTAGE_H

#include "haartree.h"

class HaarStage;

typedef QVector<HaarStage> HaarStageVector;

class HaarStageHID
{
    public:
        explicit HaarStageHID(const HaarStage &stage,
                              int oWidth,
                              const quint32 *integral,
                              const quint32 *tiltedIntegral,
                              qreal invArea,
                              qreal scale);
        ~HaarStageHID();

        int m_count;
        HaarTreeHID **m_trees;
        qreal m_threshold;
        HaarStageHID *m_parentStagePtr;
        HaarStageHID *m_nextStagePtr;
        HaarStageHID *m_childStagePtr;

        inline bool pass(size_t offset, qreal varianceNormFactor) const
        {
            qreal sum = 0;

            for (int i = 0; i < this->m_count; i++)
                sum += this->m_trees[i]->eval(offset, varianceNormFactor);

            return sum >= this->m_threshold;
        }
};

class HaarStage: public QObject
{
    Q_OBJECT

    public:
        explicit HaarStage(QObject *parent = NULL);
        HaarStage(const HaarStage &other);
        ~HaarStage();

        Q_INVOKABLE HaarTreeVector trees() const;
        Q_INVOKABLE HaarTreeVector &trees();
        Q_INVOKABLE qreal threshold() const;
        Q_INVOKABLE qreal &threshold();
        Q_INVOKABLE int parentStage() const;
        Q_INVOKABLE int &parentStage();
        Q_INVOKABLE int nextStage() const;
        Q_INVOKABLE int &nextStage();
        Q_INVOKABLE int childStage() const;
        Q_INVOKABLE int &childStage();

        HaarStage &operator =(const HaarStage &other);
        bool operator ==(const HaarStage &other) const;
        bool operator !=(const HaarStage &other) const;

    private:
        HaarTreeVector m_trees;
        qreal m_threshold;
        int m_parentStage;
        int m_nextStage;
        int m_childStage;
        HaarStage *m_parentStagePtr;
        HaarStage *m_nextStagePtr;
        HaarStage *m_childStagePtr;

    signals:
        void treesChanged(const HaarTreeVector &trees);
        void thresholdChanged(qreal threshold);
        void parentStageChanged(int parentStage);
        void nextStageChanged(int nextStage);
        void childStageChanged(int childStage);

    public slots:
        void setTrees(const HaarTreeVector &trees);
        void setThreshold(qreal threshold);
        void setParentStage(int parentStage);
        void setNextStage(int nextStage);
        void setChildStage(int childStage);
        void resetTrees();
        void resetThreshold();
        void resetParentStage();
        void resetNextStage();
        void resetChildStage();

    friend class HaarStageHID;
};

#endif // HAARSTAGE_H
