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

#ifndef HAARFEATURE_H
#define HAARFEATURE_H

#include <QObject>
#include <QVector>
#include <QRect>

#define HAAR_FEATURE_MAX 3

class HaarFeature;

using RectVector = QVector<QRect>;
using RealVector = QVector<qreal>;
using HaarFeatureVector = QVector<HaarFeature>;

class HaarFeatureHID
{
    public:
        HaarFeatureHID(const HaarFeature &feature,
                       int oWidth,
                       const quint32 *integral,
                       const quint32 *tiltedIntegral,
                       qreal invArea,
                       qreal scale);

        int m_count;
        bool m_tilted;
        qreal m_threshold;
        int m_leftNode;
        qreal m_leftVal;
        int m_rightNode;
        qreal m_rightVal;

        quint32 *m_p0[HAAR_FEATURE_MAX];
        quint32 *m_p1[HAAR_FEATURE_MAX];
        quint32 *m_p2[HAAR_FEATURE_MAX];
        quint32 *m_p3[HAAR_FEATURE_MAX];
        qreal m_weight[HAAR_FEATURE_MAX];

        inline bool goLeft(size_t offset, qreal varianceNormFactor) const
        {
            qreal featureSum = 0;

            for (int i = 0; i < this->m_count; i++)
                featureSum += (this->m_p0[i][offset]
                             - this->m_p1[i][offset]
                             - this->m_p2[i][offset]
                             + this->m_p3[i][offset]) * this->m_weight[i];

            return featureSum < this->m_threshold * varianceNormFactor;
        }
};

class HaarFeature: public QObject
{
    Q_OBJECT

    public:
        HaarFeature(QObject *parent=nullptr);
        HaarFeature(const HaarFeature &other);
        ~HaarFeature() = default;

        Q_INVOKABLE RectVector rects() const;
        Q_INVOKABLE RealVector weight() const;
        Q_INVOKABLE bool tilted() const;
        Q_INVOKABLE bool &tilted();
        Q_INVOKABLE qreal threshold() const;
        Q_INVOKABLE qreal &threshold();
        Q_INVOKABLE int leftNode() const;
        Q_INVOKABLE int &leftNode();
        Q_INVOKABLE qreal leftVal() const;
        Q_INVOKABLE qreal &leftVal();
        Q_INVOKABLE int rightNode() const;
        Q_INVOKABLE int &rightNode();
        Q_INVOKABLE qreal rightVal() const;
        Q_INVOKABLE qreal &rightVal();

        HaarFeature &operator =(const HaarFeature &other);
        bool operator ==(const HaarFeature &other) const;
        bool operator !=(const HaarFeature &other) const;

    private:
        QRect m_rects[HAAR_FEATURE_MAX];
        qreal m_weight[HAAR_FEATURE_MAX];
        int m_count;
        bool m_tilted;
        qreal m_threshold;
        int m_leftNode;
        qreal m_leftVal;
        int m_rightNode;
        qreal m_rightVal;

    signals:
        void rectsChanged(const RectVector &rects);
        void weightChanged(const RealVector &weight);
        void tiltedChanged(bool tilted);
        void thresholdChanged(qreal threshold);
        void leftNodeChanged(int leftNode);
        void leftValChanged(qreal leftVal);
        void rightNodeChanged(int rightNode);
        void rightValChanged(qreal rightVal);

    public slots:
        void setRects(const RectVector &rects);
        void setWeight(const RealVector &weight);
        void setTilted(bool tilted);
        void setThreshold(qreal threshold);
        void setLeftNode(int leftNode);
        void setLeftVal(qreal leftVal);
        void setRightNode(int rightNode);
        void setRightVal(qreal rightVal);
        void resetRects();
        void resetWeight();
        void resetTilted();
        void resetThreshold();
        void resetLeftNode();
        void resetLeftVal();
        void resetRightNode();
        void resetRightVal();

    friend class HaarFeatureHID;
};

#endif // HAARFEATURE_H
