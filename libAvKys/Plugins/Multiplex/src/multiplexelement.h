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

#ifndef MULTIPLEXELEMENT_H
#define MULTIPLEXELEMENT_H

#include <akelement.h>

class MultiplexElementPrivate;
class AkCaps;

class MultiplexElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(int inputIndex
               READ inputIndex
               WRITE setInputIndex
               RESET resetInputIndex
               NOTIFY inputIndexChanged)
    Q_PROPERTY(int outputIndex
               READ outputIndex
               WRITE setOutputIndex
               RESET resetOutputIndex
               NOTIFY outputIndexChanged)

    public:
        MultiplexElement();
        ~MultiplexElement();

        Q_INVOKABLE AkCaps caps() const;
        Q_INVOKABLE int inputIndex() const;
        Q_INVOKABLE int outputIndex() const;

    private:
        MultiplexElementPrivate *d;

    signals:
        void capsChanged(const AkCaps &caps);
        void inputIndexChanged(int inputIndex);
        void outputIndexChanged(int outputIndex);

    public slots:
        void setCaps(const AkCaps &caps);
        void setInputIndex(int inputIndex);
        void setOutputIndex(int outputIndex);
        void resetCaps();
        void resetInputIndex();
        void resetOutputIndex();

        AkPacket iStream(const AkPacket &packet);
};

#endif // MULTIPLEXELEMENT_H
