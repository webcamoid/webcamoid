/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef EDGEELEMENT_H
#define EDGEELEMENT_H

#include <ak.h>
#include <akutils.h>

class EdgeElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(bool canny
               READ canny
               WRITE setCanny
               RESET resetCanny
               NOTIFY cannyChanged)
    Q_PROPERTY(int thLow
               READ thLow
               WRITE setThLow
               RESET resetThLow
               NOTIFY thLowChanged)
    Q_PROPERTY(int thHi
               READ thHi
               WRITE setThHi
               RESET resetThHi
               NOTIFY thHiChanged)
    Q_PROPERTY(bool equalize
               READ equalize
               WRITE setEqualize
               RESET resetEqualize
               NOTIFY equalizeChanged)
    Q_PROPERTY(bool invert
               READ invert
               WRITE setInvert
               RESET resetInvert
               NOTIFY invertChanged)

    public:
        explicit EdgeElement();
        ~EdgeElement();

        Q_INVOKABLE bool canny() const;
        Q_INVOKABLE int thLow() const;
        Q_INVOKABLE int thHi() const;
        Q_INVOKABLE bool equalize() const;
        Q_INVOKABLE bool invert() const;

    private:
        bool m_canny;
        int m_thLow;
        int m_thHi;
        bool m_equalize;
        bool m_invert;

        QVector<quint8> equalize(const QImage &image);
        void sobel(int width, int height, const QVector<quint8> &gray,
                   QVector<quint16> &gradient, QVector<quint8> &direction) const;
        QVector<quint16> thinning(int width, int height,
                                  const QVector<quint16> &gradient,
                                  const QVector<quint8> &direction) const;
        QVector<quint8> threshold(int width, int height,
                                  const QVector<quint16> &image,
                                  const QVector<int> &thresholds,
                                  const QVector<int> &map) const;
        void trace(int width, int height, QVector<quint8> &canny,
                   int x, int y) const;
        QVector<quint8> hysteresisThresholding(int width, int height,
                                               const QVector<quint8> &thresholded) const;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void cannyChanged(bool canny);
        void thLowChanged(int thLow);
        void thHiChanged(int thHi);
        void equalizeChanged(bool equalize);
        void invertChanged(bool invert);

    public slots:
        void setCanny(bool canny);
        void setThLow(int thLow);
        void setThHi(int thHi);
        void setEqualize(bool equalize);
        void setInvert(bool invert);
        void resetCanny();
        void resetThLow();
        void resetThHi();
        void resetEqualize();
        void resetInvert();
        AkPacket iStream(const AkPacket &packet);
};

#endif // EDGEELEMENT_H
