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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef ACAPSCONVERTELEMENT_H
#define ACAPSCONVERTELEMENT_H

#include <QMutex>
#include <ak.h>

#ifdef USE_GSTREAMER
#include "gstreamer/convertaudio.h"
#else
#include "ffmpeg/convertaudio.h"
#endif

class ACapsConvertElement: public AkElement
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
        AkCaps m_caps;
        ConvertAudio m_convertAudio;
        QMutex m_mutex;

    signals:
        void capsChanged(const QString &caps);

    public slots:
        void setCaps(const QString &caps);
        void resetCaps();

        AkPacket iStream(const AkAudioPacket &packet);
};

#endif // ACAPSCONVERTELEMENT_H
