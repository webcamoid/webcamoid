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

#ifndef EmbossELEMENT_H
#define EmbossELEMENT_H

#include <akelement.h>

class EmbossElementPrivate;

class EmbossElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal factor
               READ factor
               WRITE setFactor
               RESET resetFactor
               NOTIFY factorChanged)
    Q_PROPERTY(qreal bias
               READ bias
               WRITE setBias
               RESET resetBias
               NOTIFY biasChanged)

    public:
        EmbossElement();
        ~EmbossElement();

        Q_INVOKABLE qreal factor() const;
        Q_INVOKABLE qreal bias() const;

    private:
        EmbossElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void factorChanged(qreal factor);
        void biasChanged(qreal bias);

    public slots:
        void setFactor(qreal factor);
        void setBias(qreal bias);
        void resetFactor();
        void resetBias();
};

#endif // EmbossELEMENT_H
