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

#ifndef WARPELEMENT_H
#define WARPELEMENT_H

#include <akelement.h>

class WarpElementPrivate;

class WarpElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal ripples
               READ ripples
               WRITE setRipples
               RESET resetRipples
               NOTIFY ripplesChanged)
    Q_PROPERTY(qreal duration
               READ duration
               WRITE setDuration
               RESET resetDuration
               NOTIFY durationChanged)

    public:
        WarpElement();
        ~WarpElement();

        Q_INVOKABLE qreal ripples() const;
        Q_INVOKABLE int duration() const;

    private:
        WarpElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void ripplesChanged(qreal ripples);
        void durationChanged(int duration);

    public slots:
        void setRipples(qreal ripples);
        void setDuration(int duration);
        void resetRipples();
        void resetDuration();
};

#endif // WARPELEMENT_H
