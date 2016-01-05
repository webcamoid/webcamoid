/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef PROBEELEMENT_H
#define PROBEELEMENT_H

#include <akelement.h>

class ProbeElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(bool log
               READ log
               WRITE setLog
               RESET resetLog
               NOTIFY logChanged)

    public:
        explicit ProbeElement();

        Q_INVOKABLE bool log() const;

    private:
        bool m_log;

    signals:
        void logChanged(bool log);

    public slots:
        void setLog(bool log);
        void resetLog();

        AkPacket iStream(const AkPacket &packet);
};

#endif // PROBEELEMENT_H
