/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef ACAPSCONVERTELEMENT_H
#define ACAPSCONVERTELEMENT_H

#include <qb.h>

#include "ffmpeg/convertaudio.h"

class ACapsConvertElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit ACapsConvertElement();

        Q_INVOKABLE QString caps() const;

    private:
        QbCaps m_caps;
        ConvertAudio m_convertAudio;

    signals:
        void capsChanged(const QString &caps);

    public slots:
        void setCaps(const QString &caps);
        void resetCaps();

        QbPacket iStream(const QbAudioPacket &packet);
};

#endif // ACAPSCONVERTELEMENT_H
