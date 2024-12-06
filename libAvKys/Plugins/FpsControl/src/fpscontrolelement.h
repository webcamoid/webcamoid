/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef FPSCONTROLELEMENT_H
#define FPSCONTROLELEMENT_H

#include <iak/akelement.h>

class FpsControlElementPrivate;
class AkFrac;

class FpsControlElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)
    Q_PROPERTY(bool fillGaps
               READ fillGaps
               WRITE setFillGaps
               RESET resetFillGaps
               NOTIFY fillGapsChanged)

    public:
        FpsControlElement();
        ~FpsControlElement();

        Q_INVOKABLE AkFrac fps() const;
        Q_INVOKABLE bool fillGaps() const;
        Q_INVOKABLE bool discard(const AkVideoPacket &packet);

    private:
        FpsControlElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void fpsChanged(const AkFrac &fps);
        void fillGapsChanged(bool fillGaps);

    public slots:
        void setFps(const AkFrac &fps);
        void setFillGaps(bool fillGaps);
        void resetFps();
        void resetFillGaps();
        void restart();
};

#endif // FPSCONTROLELEMENT_H
