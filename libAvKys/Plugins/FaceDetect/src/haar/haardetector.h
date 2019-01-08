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

#ifndef HAARDETECTOR_H
#define HAARDETECTOR_H

#include <QImage>

class HaarDetectorPrivate;

class HaarDetector: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool equalize
               READ equalize
               WRITE setEqualize
               RESET resetEqualize
               NOTIFY equalizeChanged)
    Q_PROPERTY(int denoiseRadius
               READ denoiseRadius
               WRITE setDenoiseRadius
               RESET resetDenoiseRadius
               NOTIFY denoiseRadiusChanged)
    Q_PROPERTY(int denoiseMu
               READ denoiseMu
               WRITE setDenoiseMu
               RESET resetDenoiseMu
               NOTIFY denoiseMuChanged)
    Q_PROPERTY(int denoiseSigma
               READ denoiseSigma
               WRITE setDenoiseSigma
               RESET resetDenoiseSigma
               NOTIFY denoiseSigmaChanged)
    Q_PROPERTY(bool cannyPruning
               READ cannyPruning
               WRITE setCannyPruning
               RESET resetCannyPruning
               NOTIFY cannyPruningChanged)
    Q_PROPERTY(qreal lowCannyThreshold
               READ lowCannyThreshold
               WRITE setLowCannyThreshold
               RESET resetLowCannyThreshold
               NOTIFY lowCannyThresholdChanged)
    Q_PROPERTY(qreal highCannyThreshold
               READ highCannyThreshold
               WRITE setHighCannyThreshold
               RESET resetHighCannyThreshold
               NOTIFY highCannyThresholdChanged)
    Q_PROPERTY(int minNeighbors
               READ minNeighbors
               WRITE setMinNeighbors
               RESET resetMinNeighbors
               NOTIFY minNeighborsChanged)

    public:
        HaarDetector(QObject *parent=nullptr);
        ~HaarDetector();

        Q_INVOKABLE bool equalize() const;
        Q_INVOKABLE bool &equalize();
        Q_INVOKABLE int denoiseRadius() const;
        Q_INVOKABLE int &denoiseRadius();
        Q_INVOKABLE int denoiseMu() const;
        Q_INVOKABLE int &denoiseMu();
        Q_INVOKABLE int denoiseSigma() const;
        Q_INVOKABLE int &denoiseSigma();
        Q_INVOKABLE bool cannyPruning() const;
        Q_INVOKABLE bool &cannyPruning();
        Q_INVOKABLE qreal lowCannyThreshold() const;
        Q_INVOKABLE qreal &lowCannyThreshold();
        Q_INVOKABLE qreal highCannyThreshold() const;
        Q_INVOKABLE qreal &highCannyThreshold();
        Q_INVOKABLE int minNeighbors() const;
        Q_INVOKABLE int &minNeighbors();
        Q_INVOKABLE bool loadCascade(const QString &fileName);
        Q_INVOKABLE QVector<QRect> detect(const QImage &image,
                                          qreal scaleFactor=1.1,
                                          QSize minObjectSize=QSize(),
                                          QSize maxObjectSize=QSize()) const;

    private:
        HaarDetectorPrivate *d;

    signals:
        void equalizeChanged(bool equalize);
        void denoiseRadiusChanged(int denoiseRadius);
        void denoiseMuChanged(int denoiseMu);
        void denoiseSigmaChanged(int denoiseSigma);
        void cannyPruningChanged(bool cannyPruning);
        void lowCannyThresholdChanged(qreal lowCannyThreshold);
        void highCannyThresholdChanged(qreal highCannyThreshold);
        void minNeighborsChanged(int minNeighbors);

    public slots:
        void setEqualize(bool equalize);
        void setDenoiseRadius(int denoiseRadius);
        void setDenoiseMu(int denoiseMu);
        void setDenoiseSigma(int denoiseSigma);
        void setCannyPruning(bool cannyPruning);
        void setLowCannyThreshold(qreal lowCannyThreshold);
        void setHighCannyThreshold(qreal highCannyThreshold);
        void setMinNeighbors(int minNeighbors);
        void resetEqualize();
        void resetDenoiseRadius();
        void resetDenoiseMu();
        void resetDenoiseSigma();
        void resetCannyPruning();
        void resetLowCannyThreshold();
        void resetHighCannyThreshold();
        void resetMinNeighbors();
};

#endif // HAARDETECTOR_H
