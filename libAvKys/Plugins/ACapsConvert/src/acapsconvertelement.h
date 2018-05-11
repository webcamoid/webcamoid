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

#ifndef ACAPSCONVERTELEMENT_H
#define ACAPSCONVERTELEMENT_H

#include <akelement.h>

class ACapsConvertElementPrivate;
class AkCaps;

class ACapsConvertElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(QString convertLib
               READ convertLib
               WRITE setConvertLib
               RESET resetConvertLib
               NOTIFY convertLibChanged)

    public:
        explicit ACapsConvertElement();
        ~ACapsConvertElement();

        Q_INVOKABLE QString caps() const;
        Q_INVOKABLE QString convertLib() const;

    private:
        ACapsConvertElementPrivate *d;

    signals:
        void capsChanged(const QString &caps);
        void convertLibChanged(const QString &convertLib);

    public slots:
        void setCaps(const QString &caps);
        void setConvertLib(const QString &convertLib);
        void resetCaps();
        void resetConvertLib();

        AkPacket iStream(const AkAudioPacket &packet);
        bool setState(AkElement::ElementState state);

    private slots:
        void convertLibUpdated(const QString &convertLib);
};

#endif // ACAPSCONVERTELEMENT_H
