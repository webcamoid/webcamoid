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

#ifndef HAARSTAGE_H
#define HAARSTAGE_H

#include "haartree.h"

class HaarStage;

using HaarStageVector = QVector<HaarStage>;

class HaarStageHID
{
    public:
        HaarStageHID(const HaarStage &stage,
                     int oWidth,
                     const quint32 *integral,
                     const quint32 *tiltedIntegral,
                     qreal invArea,
                     qreal scale);
        HaarStageHID(const HaarStageHID &other) = delete;
        ~HaarStageHID();

        int m_count {0};
        HaarTreeHID **m_trees {nullptr};
        qreal m_threshold {0.0};
        HaarStageHID *m_parentStagePtr {nullptr};
        HaarStageHID *m_nextStagePtr {nullptr};
        HaarStageHID *m_childStagePtr {nullptr};

        inline bool pass(size_t offset, qreal varianceNormFactor) const
        {
            qreal sum = 0;

            for (int i = 0; i < this->m_count; i++)
                sum += this->m_trees[i]->eval(offset, varianceNormFactor);

            return sum >= this->m_threshold;
        }
};

class HaarStagePrivate;

class HaarStage: public QObject
{
    Q_OBJECT

    public:
        HaarStage(QObject *parent=nullptr);
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
        HaarStagePrivate *d;

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
