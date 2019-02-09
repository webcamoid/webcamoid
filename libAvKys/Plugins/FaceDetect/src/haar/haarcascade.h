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

#ifndef HAARCASCADE_H
#define HAARCASCADE_H

#include <QMutex>

#include "haarstage.h"

class HaarCascade;

class HaarCascadeHID
{
    public:
        HaarCascadeHID(const HaarCascade &cascade,
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
                       QMutex *mutex);
        HaarCascadeHID(const HaarCascadeHID &other) = delete;
        ~HaarCascadeHID();

        static void run(HaarCascadeHID *cascade);

    private:
        int m_count;
        HaarStageHID **m_stages;
        int m_startX;
        int m_endX;
        int m_startY;
        int m_endY;
        int m_windowWidth;
        int m_windowHeight;
        int m_oWidth;
        qreal m_step;
        qreal m_invArea;
        bool m_isTree;
        bool m_cannyPruning;
        const quint32 *m_p[4];
        const quint64 *m_pq[4];
        const quint32 *m_ip[4];
        const quint32 *m_icp[4];
        QList<QRect> *m_roi;
        QMutex *m_mutex;
};

class HaarCascade: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name
               READ name
               WRITE setName
               RESET resetName
               NOTIFY nameChanged)
    Q_PROPERTY(QSize windowSize
               READ windowSize
               WRITE setWindowSize
               RESET resetWindowSize
               NOTIFY windowSizeChanged)
    Q_PROPERTY(HaarStageVector stages
               READ stages
               WRITE setStages
               RESET resetStages
               NOTIFY stagesChanged)
    Q_PROPERTY(QString errorString
               READ errorString
               NOTIFY errorStringChanged)

    public:
        HaarCascade(QObject *parent=nullptr);
        HaarCascade(const HaarCascade &other);
        ~HaarCascade() = default;

        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString &name();
        Q_INVOKABLE QSize windowSize() const;
        Q_INVOKABLE QSize &windowSize();
        Q_INVOKABLE HaarStageVector stages() const;
        Q_INVOKABLE HaarStageVector &stages();
        Q_INVOKABLE QString errorString() const;
        Q_INVOKABLE bool load(const QString &fileName);

        HaarCascade &operator =(const HaarCascade &other);
        bool operator ==(const HaarCascade &other) const;
        bool operator !=(const HaarCascade &other) const;

    private:
        QString m_name;
        QSize m_windowSize;
        HaarStageVector m_stages;
        QString m_errorString;
        bool m_isTree;

    signals:
        void nameChanged(const QString &name);
        void windowSizeChanged(const QSize &windowSize);
        void stagesChanged(const HaarStageVector &stages);
        void errorStringChanged(const QString &errorString);

    public slots:
        void setName(const QString &name);
        void setWindowSize(const QSize &windowSize);
        void setStages(const HaarStageVector &stages);
        void resetName();
        void resetWindowSize();
        void resetStages();

    friend class HaarCascadeHID;
};

#endif // HAARCASCADE_H
