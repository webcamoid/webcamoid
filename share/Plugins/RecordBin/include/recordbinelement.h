/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef RECORDBINELEMENT_H
#define RECORDBINELEMENT_H

#include <QtGui>
#include <gst/gst.h>

#include "element.h"

class RecordBinElement: public Element
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ fileName
                                WRITE setFileName
                                RESET resetFileName)

    Q_PROPERTY(QString videoEncoder READ videoEncoder
                                    WRITE setVideoEncoder
                                    RESET resetVideoEncoder)

    Q_PROPERTY(QString audioEncoder READ audioEncoder
                                    WRITE setAudioEncoder
                                    RESET resetAudioEncoder)

    Q_PROPERTY(QString muxer READ muxer
                             WRITE setMuxer
                             RESET resetMuxer)

    Q_PROPERTY(bool recordAudio READ recordAudio
                                WRITE setRecordAudio
                                RESET resetRecordAudio)

    Q_PROPERTY(QSize frameSize READ frameSize
                               WRITE setFrameSize
                               RESET resetFrameSize)

    public:
        explicit RecordBinElement();
        ~RecordBinElement();

        Q_INVOKABLE QString fileName();
        Q_INVOKABLE QString videoEncoder();
        Q_INVOKABLE QString audioEncoder();
        Q_INVOKABLE QString muxer();
        Q_INVOKABLE bool recordAudio();
        Q_INVOKABLE QSize frameSize();

        Q_INVOKABLE ElementState state();

    private:
        QString m_fileName;
        QString m_videoEncoder;
        QString m_audioEncoder;
        QString m_muxer;
        bool m_recordAudio;
        QSize m_frameSize;
        ElementState m_state;

        QMutex m_mutex;
        int m_callBack;
        GstElement *m_pipeline;
        QImage m_iFrame;

        static void needData(GstElement *appsrc, guint size, gpointer self);

    public slots:
        void setFileName(QString fileName);
        void setVideoEncoder(QString videoEncoder);
        void setAudioEncoder(QString audioEncoder);
        void setMuxer(QString muxer);
        void setRecordAudio(bool recordAudio);
        void setFrameSize(QSize frameSize);
        void resetFileName();
        void resetVideoEncoder();
        void resetAudioEncoder();
        void resetMuxer();
        void resetRecordAudio();
        void resetFrameSize();

        void iStream(const void *data, int datalen, QString dataType);
        void setState(ElementState state);
        void resetState();
};

#endif // RECORDBINELEMENT_H
