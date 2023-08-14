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

#ifndef DENOISEELEMENT_H
#define DENOISEELEMENT_H

#include <akelement.h>

class DenoiseElementPrivate;

class DenoiseElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int radius
               READ radius
               WRITE setRadius
               RESET resetRadius
               NOTIFY radiusChanged)
    Q_PROPERTY(int factor
               READ factor
               WRITE setFactor
               RESET resetFactor
               NOTIFY factorChanged)
    Q_PROPERTY(int mu
               READ mu
               WRITE setMu
               RESET resetMu
               NOTIFY muChanged)
    Q_PROPERTY(qreal sigma
               READ sigma
               WRITE setSigma
               RESET resetSigma
               NOTIFY sigmaChanged)

    public:
        DenoiseElement();
        ~DenoiseElement();

        Q_INVOKABLE int radius() const;
        Q_INVOKABLE int factor() const;
        Q_INVOKABLE int mu() const;
        Q_INVOKABLE qreal sigma() const;

    private:
        DenoiseElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void radiusChanged(int radius);
        void factorChanged(int factor);
        void muChanged(int mu);
        void sigmaChanged(qreal sigma);

    public slots:
        void setRadius(int radius);
        void setFactor(int factor);
        void setMu(int mu);
        void setSigma(qreal sigma);
        void resetRadius();
        void resetFactor();
        void resetMu();
        void resetSigma();
};

#endif // DENOISEELEMENT_H
