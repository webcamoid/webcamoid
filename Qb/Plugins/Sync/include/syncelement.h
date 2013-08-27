/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef SYNCELEMENT_H
#define SYNCELEMENT_H

#include <qb.h>

#include "clock.h"
#include "lock.h"

class SyncElement: public QbElement
{
    Q_OBJECT

    public:
        explicit SyncElement();

    private:
        bool m_ready;
        bool m_fst;

        Clock m_audioClock;
        Clock m_videoClock;
        Clock m_extrnClock;

        QMutex m_audioLock;
        QMutex m_videoLock;
        Lock m_globlLock;

    signals:
        void ready(int id);
        void oDiscardFrames(int nFrames);

    public slots:
        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processAudioFrame(const QbPacket &packet);
        void processVideoFrame(const QbPacket &packet);
};

#endif // SYNCELEMENT_H
