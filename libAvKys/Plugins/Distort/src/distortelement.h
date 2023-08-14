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

#ifndef DISTORTELEMENT_H
#define DISTORTELEMENT_H

#include <akelement.h>

class DistortElementPrivate;

class DistortElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(qreal frequency
               READ frequency
               WRITE setFrequency
               RESET resetFrequency
               NOTIFY frequencyChanged)
    Q_PROPERTY(int gridSizeLog
               READ gridSizeLog
               WRITE setGridSizeLog
               RESET resetGridSizeLog
               NOTIFY gridSizeLogChanged)

    public:
        DistortElement();
        ~DistortElement();

        Q_INVOKABLE qreal amplitude() const;
        Q_INVOKABLE qreal frequency() const;
        Q_INVOKABLE int gridSizeLog() const;

    private:
        DistortElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void amplitudeChanged(qreal amplitude);
        void frequencyChanged(qreal frequency);
        void gridSizeLogChanged(int gridSizeLog);

    public slots:
        void setAmplitude(qreal amplitude);
        void setFrequency(qreal frequency);
        void setGridSizeLog(int gridSizeLog);
        void resetAmplitude();
        void resetFrequency();
        void resetGridSizeLog();
};

#endif // DISTORTELEMENT_H
