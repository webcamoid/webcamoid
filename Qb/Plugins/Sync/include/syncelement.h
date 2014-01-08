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

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
}

#include "avqueue.h"
#include "clock.h"

typedef QSharedPointer<SwrContext> SwrContextPtr;
typedef QSharedPointer<QTimer> TimerPtr;

class SyncElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(int outputAudioBufferSize READ outputAudioBufferSize
                                         WRITE setOutputAudioBufferSize
                                         RESET resetOutputAudioBufferSize)

    Q_PROPERTY(QString audioTh READ audioTh
                               WRITE setAudioTh
                               RESET resetAudioTh)

    Q_PROPERTY(QString videoTh READ videoTh
                               WRITE setVideoTh
                               RESET resetVideoTh)

    public:
        explicit SyncElement();
        ~SyncElement();

        Q_INVOKABLE int outputAudioBufferSize() const;
        Q_INVOKABLE QString audioTh() const;
        Q_INVOKABLE QString videoTh() const;

    private:
        bool m_ready;
        bool m_fst;

        bool m_log;

        double m_audioDiffCum;
        double m_audioDiffAvgCoef;
        int m_audioDiffAvgCount;
        int m_outputAudioBufferSize;

        QString m_audioTh;
        QString m_videoTh;

        double m_frameLastPts;
        double m_frameLastDuration;
        double m_frameTimer;
        double m_frameLastDroppedPts;
        double m_remainingTime;

        AVQueue m_avqueue;

        Clock m_audioClock;
        Clock m_videoClock;
        Clock m_extrnClock;

        QbCaps m_curInputCaps;
        SwrContextPtr m_resampleContext;

        TimerPtr m_audioTimer;
        TimerPtr m_videoTimer;

        QbThreadPtr m_audioThread;
        QbThreadPtr m_videoThread;

        static void deleteSwrContext(SwrContext *context);
        QbPacket compensateAudio(const QbPacket &packet, int wantedSamples);
        int synchronizeAudio(double diff, QbPacket packet);
        void printLog(const QbPacket &packet, double diff);
        TimerPtr runThread(QThread *thread, const char *method);
        double computeTargetDelay(double delay, double *diff=NULL);

    signals:
        void ready(int id);
        void oDiscardFrames(int nFrames);

    public slots:
        void setOutputAudioBufferSize(int outputAudioBufferSize);
        void setAudioTh(const QString &audioTh);
        void setVideoTh(const QString &videoTh);
        void resetOutputAudioBufferSize();
        void resetAudioTh();
        void resetVideoTh();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processAudioFrame();
        void processVideoFrame();
        void videoRefresh(double *remainingTime);
        void updateVideoPts(double pts);
};

#endif // SYNCELEMENT_H
