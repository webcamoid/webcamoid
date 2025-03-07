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

#ifndef NERVOUSELEMENT_H
#define NERVOUSELEMENT_H

#include <iak/akelement.h>

class NervousElementPrivate;

class NervousElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int nFrames
               READ nFrames
               WRITE setNFrames
               RESET resetNFrames
               NOTIFY nFramesChanged)
    Q_PROPERTY(bool simple
               READ simple
               WRITE setSimple
               RESET resetSimple
               NOTIFY simpleChanged)

    public:
        NervousElement();
        ~NervousElement();

        Q_INVOKABLE int nFrames() const;
        Q_INVOKABLE bool simple() const;

    private:
        NervousElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void nFramesChanged(int nFrames);
        void simpleChanged(bool simple);

    public slots:
        void setNFrames(int nFrames);
        void setSimple(bool simple);
        void resetNFrames();
        void resetSimple();
};

#endif // NERVOUSELEMENT_H
