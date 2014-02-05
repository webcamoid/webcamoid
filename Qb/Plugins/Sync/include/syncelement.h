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

#include "avqueue.h"
#include "clock.h"

typedef QSharedPointer<QTimer> TimerPtr;

class SyncElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString videoTh READ videoTh
                               WRITE setVideoTh
                               RESET resetVideoTh)

    public:
        explicit SyncElement();
        ~SyncElement();

        Q_INVOKABLE QString videoTh() const;

    private:
        bool m_ready;
        bool m_fst;

        bool m_log;

        QString m_videoTh;
        AVQueue m_avqueue;

        Clock m_videoClock;
        Clock m_extrnClock;

        TimerPtr m_videoTimer;
        QbThreadPtr m_videoThread;

        void printLog(const QbPacket &packet, double diff);
        TimerPtr runThread(QThread *thread, const char *method);

    signals:
        void ready(int id);
        void oDiscardFrames(int nFrames);

    public slots:
        void setAudioPts(double pts);
        void setVideoTh(const QString &videoTh);
        void resetVideoTh();
        void releaseAudioFrame(int frameSize);
        void processVideoFrame();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void updateVideoPts(double pts);
};

#endif // SYNCELEMENT_H
