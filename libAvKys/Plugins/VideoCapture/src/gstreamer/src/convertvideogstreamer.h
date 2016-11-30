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

#ifndef CONVERTVIDEOGSTREAMER_H
#define CONVERTVIDEOGSTREAMER_H

#include <QtConcurrent>
#include <ak.h>
#include <akvideopacket.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "convertvideo.h"

class ConvertVideoGStreamer: public ConvertVideo
{
    Q_OBJECT

    public:
        explicit ConvertVideoGStreamer(QObject *parent=NULL);
        ~ConvertVideoGStreamer();

        Q_INVOKABLE void packetEnqueue(const AkPacket &packet);
        Q_INVOKABLE bool init(const AkCaps &caps);
        Q_INVOKABLE void uninit();

    private:
        QThreadPool m_threadPool;
        GstElement *m_pipeline;
        GstElement *m_source;
        GstElement *m_sink;
        GMainLoop *m_mainLoop;
        guint m_busWatchId;
        qint64 m_id;
        qint64 m_ptsDiff;

        GstElement *decoderFromCaps(const GstCaps *caps) const;
        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        static GstFlowReturn videoBufferCallback(GstElement *videoOutput,
                                                 gpointer userData);
};

#endif // CONVERTVIDEOGSTREAMER_H
